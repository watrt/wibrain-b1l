//----------------------------------------------------------------------------------------------------------------
//
//  TECHDINE R4510SP Model
// 
//  Copyright (c) 2007-2008 Wibrain Corporation (charles.park@wibrain.com)
// 
//----------------------------------------------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/input.h>

#include <linux/stat.h>
#include <linux/proc_fs.h>

#include <asm/io.h>
#include <asm/uaccess.h>

//----------------------------------------------------------------------------------------------------------------
MODULE_AUTHOR("Wibrain");
MODULE_DESCRIPTION("TECHDIEN touchscreen controller");
MODULE_LICENSE("Dual BSD/GPL");

//----------------------------------------------------------------------------------------------------------------
//
//	EC IO Address MAP
//	
//----------------------------------------------------------------------------------------------------------------
//	ADDRESS		Action		Description
//	0x3E0		Read		ODR3 Read(EC Output Register Read)
//				Write		IDR3 Write(EC Data Register Write)
//						
//	0x3E4		Read		STR3 Read(EC Status Register Read)
//				Write		IDR3 Write(EC Command Register Write)
//
//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
//
//	EC STR Register Description
//
//----------------------------------------------------------------------------------------------------------------
//	BIT0		Output Buffer FULL (EC->HOST)
//	BIT1		Input Buffer FULL	(HOST->EC)
//	BIT3		1 = Command(HOST->EC), 0 = Data(EC->HOST)
//
//----------------------------------------------------------------------------------------------------------------
#define	DEBUG_MSG			0

#define	TRUE				1
#define	FALSE				0

#define	SAMPLE_PERIOD		(HZ/1000)	// 1ms
#define	SLEEP_PERIOD		(HZ/10)		// 100ms

#define	DEFAULT_PERIOD		5			// Default scan period

#define	READ_IICR_WAIT_TIME	30000		// 30 sec
#define	SLEEP_WAIT_TIME		5000		// 5 sec
#define	RELEASE_WAIT_TIME	20			// 20 msec

//----------------------------------------------------------------------------------------------------------------
// Control Address Define
//----------------------------------------------------------------------------------------------------------------
#define	EC_0x6064_BASE_ADDR	0x60
#define	EC_0x3E0_BASE_ADDR	0x3E0
#define	EC_STR_IDR_OFFSET	0x04	/* read = STR, write = IDR(command) */
#define	EC_0x3E0_CTRL_IRQ	10

//----------------------------------------------------------------------------------------------------------------
// Status Register BIT Define 
//----------------------------------------------------------------------------------------------------------------
#define	OUTBUF_FULL			0x01
#define	INBUF_FULL			0x02

#define	DELAY_TIME			1000	/* response wait time */
//----------------------------------------------------------------------------------------------------------------
// TS Initialize Command 
//----------------------------------------------------------------------------------------------------------------
#define	TS_INIT_CMD			0xA7

//----------------------------------------------------------------------------------------------------------------
// TS Status 
//----------------------------------------------------------------------------------------------------------------
#define	TS_SYNC_BIT			0x80
#define	TS_PEN_DOWN			0x40

#define	TS_ABS_MAX_X		0x3FF
#define	TS_ABS_MAX_Y		0x3FF

#define	TS_ABS_MIN_X		0
#define	TS_ABS_MIN_Y		0

//----------------------------------------------------------------------------------------------------------------
// Function proto type define
//----------------------------------------------------------------------------------------------------------------
static	unsigned char	read_ec_iicr		(void);

static	int			in_buffer_empty_check	(void);
static	int			out_buffer_full_check	(void);
static	int			read_ec_iicr_data_proc	(char *page, char **start, off_t off, int count, int *eof, void *data);

static	int			read_ec_iicr_data_proc	(char *page, char **start, off_t off, int count, int *eof, void *data);
static	int			read_raw_data_proc		(char *page, char **start, off_t off, int count, int *eof, void *data);
static	int			read_period_data_proc	(char *page, char **start, off_t off, int count, int *eof, void *data);
static	int			write_period_data_proc	(struct file *file, const char __user *buffer, unsigned long count, void *data);
static	int 		init_proc_filesystem	(void);

static void 		wbts_interrupt	(unsigned long arg);
static int 			wbts_open		(struct input_dev *dev);
static void 		wbts_close		(struct input_dev *dev);
static int __init 	wbts_init		(void);
static void __exit 	wbts_exit		(void);

//----------------------------------------------------------------------------------------------------------------
// Module param define
//----------------------------------------------------------------------------------------------------------------
static	unsigned int	wbts_io = EC_0x3E0_BASE_ADDR;
module_param_named(io, wbts_io, uint, 0);
MODULE_PARM_DESC(io, "I/O base address of EC Controler(TECHDIEN Touchscreen)");

static	unsigned int	wbts_irq = EC_0x3E0_CTRL_IRQ;
module_param_named(irq, wbts_irq, uint, 0);
MODULE_PARM_DESC(irq, "IRQ of EC Controller(TECHDIEN Touchscreen)");

//----------------------------------------------------------------------------------------------------------------
// Module define
//----------------------------------------------------------------------------------------------------------------
module_init(wbts_init);
module_exit(wbts_exit);

//----------------------------------------------------------------------------------------------------------------
// Global variable define
//----------------------------------------------------------------------------------------------------------------
static	struct	input_dev	*wbts;
static 	struct 	timer_list	ts_timer;
static	DEFINE_SPINLOCK(wbts_lock);

/* TS Data Buffer */
static	unsigned char	ts_raw_data[3] = {0,};

//----------------------------------------------------------------------------------------------------------------
// Proc filesystem define
//----------------------------------------------------------------------------------------------------------------
#define	WBTS_PROC_ROOT_NAME				"touchscreen"
#define	WBTS_PROC_RAW_DATA_NAME			"raw_data"
#define	WBTS_PROC_PERIOD_DATA_NAME		"period_data"
#define	WBTS_PROC_EC_IICR_DATA_NAME		"ec_iicr"

//----------------------------------------------------------------------------------------------------------------
static	struct	proc_dir_entry	*wbts_root_fp			= NULL;		// /proc/touchscreen
static	struct	proc_dir_entry	*wbts_raw_data_fp		= NULL;		// /proc/touchscreen/raw_data
static	struct	proc_dir_entry	*wbts_period_data_fp	= NULL;		// /proc/touchscreen/period_data
static	struct	proc_dir_entry	*wbts_ec_iicr_data_fp	= NULL;		// /proc/touchscreen/ec_iicr

static	char	period_data[PAGE_SIZE - 80];

static	int		period_value = DEFAULT_PERIOD;

static	int		sync_status = 0;
static	int		sync_x = 0;
static	int		sync_y = 0;

static	unsigned int	read_iicr_count = 0;
static	unsigned char	read_iicr = 0;

//----------------------------------------------------------------------------------------------------------------
static	int	in_buffer_empty_check(void)
{
	// wait max 10ms
	int		count = DELAY_TIME;
	
	while(1)	{
		if(!(inb(EC_0x6064_BASE_ADDR + EC_STR_IDR_OFFSET) & INBUF_FULL))	return	1;
		
		count --;	udelay(10);
	}
	
	return	0;
}

//--------------------------------------------------------------------------------------------------------------
static	int	out_buffer_full_check(void)
{
	// wait max 10ms
	int		count = DELAY_TIME;
	
	while(1)	{
		if((inb(EC_0x6064_BASE_ADDR + EC_STR_IDR_OFFSET) & OUTBUF_FULL))	return	1;
		
		count --;	udelay(10);
	}
	
	return	0;
}

//--------------------------------------------------------------------------------------------------------------
static	unsigned char	read_ec_iicr(void)
{
	unsigned char	rd_byte = 0x00;
	
	if(in_buffer_empty_check())	outb(0xB9, EC_0x6064_BASE_ADDR + EC_STR_IDR_OFFSET);
	else						printk("InBuffer Full Error!!\n");

	if(in_buffer_empty_check())	outb(0xFF, EC_0x6064_BASE_ADDR);
	else						printk("InBuffer Full Error!!\n");

	if(in_buffer_empty_check())	outb(0xD8, EC_0x6064_BASE_ADDR);
	else						printk("InBuffer Full Error!!\n");

	if(out_buffer_full_check())	rd_byte = inb(EC_0x6064_BASE_ADDR);
	else						printk("Outbuffer Empty Error!!\n");

	return	rd_byte;
}

//--------------------------------------------------------------------------------------------------------------
static	int	read_ec_iicr_data_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char	*buf = page;
	
	buf += sprintf(buf, "EC IICR Register Status [ReadCount = %d, Value = 0x%02X]\n", read_iicr_count, read_iicr);
	
	*eof = 1;
	
	return	(int)(buf-page);
}

//--------------------------------------------------------------------------------------------------------------
static	int	read_raw_data_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char	*buf = page;
	
	buf += sprintf(buf, "status[%d], x[%04d], y[%04d]\n", sync_status, sync_x, sync_y);
	
	*eof = 1;
	
	return	(int)(buf-page);
}

//----------------------------------------------------------------------------------------------------------------
static	int	read_period_data_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char	*buf = page;

	buf += sprintf(buf, "current raw data sampling period = (%d) ms\n", period_value ? period_value : 1);
		
	return	(int)(buf-page);
}

//----------------------------------------------------------------------------------------------------------------
static	int	write_period_data_proc(struct file *file, const char __user *buffer, unsigned long count, void *data)
{
	char	*buf = (char *)data;
	int		len;
	
	if(copy_from_user(buf, buffer, count))	return	-EFAULT;
	
	buf[count] = '\0';
	
	len = strlen(buf);
	
	if(buf[len - 1] == '\n')	buf[len-1] = 0;
	
	period_value = simple_strtoul(buf, NULL, 10);
	
	if(period_value < 0)		period_value = 0;
	
	if(period_value > 10)		period_value = 10;
	
	return	count;
}

//----------------------------------------------------------------------------------------------------------------
static	int init_proc_filesystem(void)
{
	wbts_root_fp = proc_mkdir(WBTS_PROC_ROOT_NAME, 0);
	
	if(wbts_root_fp)	{

		#if	(DEBUG_MSG)
			printk("wbts proc filesystem create sucess!(/proc/touchscreen)\n");
		#endif

		wbts_raw_data_fp = create_proc_entry(WBTS_PROC_RAW_DATA_NAME, S_IFREG | S_IRWXU | S_IRWXO | S_IRWXG, wbts_root_fp);
		
		if(wbts_raw_data_fp)	{
			wbts_raw_data_fp->data 			= NULL;
			wbts_raw_data_fp->read_proc		= read_raw_data_proc;
			wbts_raw_data_fp->write_proc 	= NULL;

			#if	(DEBUG_MSG)
				printk("wbts raw_data filesystem create sucess!(/proc/touchscreen/raw_data)\n");
			#endif
		}
		else	{
			#if	(DEBUG_MSG)
				printk("wbts raw_data filesystem create fail!(/proc/touchscreen/raw_data)\n");
			#endif
			return	1;
		}
		
		wbts_period_data_fp	= create_proc_entry(WBTS_PROC_PERIOD_DATA_NAME, S_IFREG | S_IRWXU | S_IRWXO | S_IRWXG, wbts_root_fp);
		
		if(wbts_period_data_fp)	{
			wbts_period_data_fp->data		= period_data;
			wbts_period_data_fp->read_proc	= read_period_data_proc;
			wbts_period_data_fp->write_proc	= write_period_data_proc;

			#if	(DEBUG_MSG)
				printk("wbts period filesystem create sucess!(/proc/touchscreen/period_data)\n");
			#endif
		}
		else	{
			#if	(DEBUG_MSG)
				printk("wbts period filesystem create fail!(/proc/touchscreen/period_data)\n");
			#endif
			return	1;
		}


		wbts_ec_iicr_data_fp	= create_proc_entry(WBTS_PROC_EC_IICR_DATA_NAME, S_IFREG | S_IRWXU | S_IRWXO | S_IRWXG, wbts_root_fp);

		if(wbts_ec_iicr_data_fp)	{
			wbts_ec_iicr_data_fp->data			= NULL;
			wbts_ec_iicr_data_fp->read_proc		= read_ec_iicr_data_proc;
			wbts_ec_iicr_data_fp->write_proc	= NULL;

			#if	(DEBUG_MSG)
				printk("wbts ec_iicr filesystem create sucess!(/proc/touchscreen/ec_iicr)\n");
			#endif
		}
		else	{
			#if	(DEBUG_MSG)
				printk("wbts ec_iicr filesystem create fail!(/proc/touchscreen/ec_iicr)\n");
			#endif
			return	1;
		}
	}
	else	{
		#if	(DEBUG_MSG)
			printk("wbts period filesystem create fail!(/proc/touchscreen)\n");
		#endif
		return	1;
	}

	return	0;
}

//----------------------------------------------------------------------------------------------------------------
static	void 		wbts_interrupt(unsigned long arg)
{
	static	unsigned char	count = 0xFF, rd_data = 0, bPressed = FALSE, bSleepMode = FALSE;
	static 	unsigned int	x, y;
	
	#if	(DEBUG_MSG)
		static	unsigned char	bOldSleepMode = true;
	#endif
		
	// Button Release
	static	unsigned int	ReleaseWaitCount = RELEASE_WAIT_TIME, ReleaseCount = 0;

	// Sleep Mode support	
	static	unsigned int	SleepWaitTime = SLEEP_WAIT_TIME, SleepCount = 0;

	// EC IICR Read period : 30 sec
	static	unsigned int	ReadIICRWaitTime = READ_IICR_WAIT_TIME, ReadIICRWaitCount = 0;

	struct timer_list *data = (struct timer_list *)arg;

	spin_lock(&wbts_lock);	

	if(inb(wbts_io + EC_STR_IDR_OFFSET) & OUTBUF_FULL)	{

		rd_data = inb(wbts_io);		bSleepMode = FALSE;		SleepCount = 0;
		
		switch(count)	{
			case	0:	if(  rd_data & TS_SYNC_BIT )	{	ts_raw_data[count] = rd_data;	count++;	}		break;
			case	1:	if(!(rd_data & TS_SYNC_BIT))	{	ts_raw_data[count] = rd_data;	count++;	}		break;
			case	2:
				if(!(rd_data & TS_SYNC_BIT))	{
					ts_raw_data[count] = rd_data;		count = 0;

					x = (ts_raw_data[1] & 0x7F) | ((ts_raw_data[0] & 0x38)<<4);
					y = (ts_raw_data[2] & 0x7F) | ((ts_raw_data[0] & 0x07)<<7);
					
					#if	(DEBUG_MSG)
						printk("TS Status = %x, Raw Data x = 0x%x[%d]. y = 0x%x[%d]\n", (ts_raw_data[0] & 0xC0), x, x, y, y);
					#endif

					if(ts_raw_data[0] & TS_PEN_DOWN)	{
						bPressed = TRUE;	sync_status = TRUE;
						input_report_key(wbts, BTN_TOUCH, 1);
					}
					else	{
						bPressed = FALSE;	sync_status = FALSE;
						input_report_key(wbts, BTN_TOUCH, 0);
					}	

					ReleaseCount = 0;	
					sync_x = 0x3FF - x;		sync_y = y;
					input_report_abs(wbts, ABS_X, 0x3FF - x);			
					input_report_abs(wbts, ABS_Y, y);
					input_sync(wbts);
				}
				break;
				
			default:
				if(rd_data & TS_SYNC_BIT)	{	
					count = 0;		ts_raw_data[count] = rd_data;		count++;	
				}
				break;
		}			
	}
	else	{
		// wait event 20ms
		if(bPressed)	{
			if(ReleaseCount > ReleaseWaitCount)	{
				bPressed = FALSE;		sync_status = FALSE;
				sync_x = 0x3FF - x;		sync_y = y;
				
				ReleaseCount = 0;	

				input_report_key(wbts, BTN_TOUCH, 0);
				input_report_abs(wbts, ABS_X, 0x3FF - x);			
				input_report_abs(wbts, ABS_Y, y);
				input_sync(wbts);
			}
			else	ReleaseCount++;
		}
	}

	init_timer(data);

	data->data 		= (unsigned long)data;
	data->function	= wbts_interrupt;

	if(bSleepMode)	{
		data->expires 	 = get_jiffies_64() + SLEEP_PERIOD;		// 100ms interval
		ReleaseWaitCount =  RELEASE_WAIT_TIME;
		ReadIICRWaitTime = (READ_IICR_WAIT_TIME / 100);
		SleepWaitTime 	 =  SLEEP_WAIT_TIME;
	}
	else	{
		if(period_value)	{
			data->expires 	 = get_jiffies_64() + (SAMPLE_PERIOD * period_value);
			ReleaseWaitCount = (RELEASE_WAIT_TIME   / period_value);
			ReadIICRWaitTime = (READ_IICR_WAIT_TIME / period_value);
			SleepWaitTime 	 = (SLEEP_WAIT_TIME     / period_value);
		}
		else	{
			data->expires 	 = get_jiffies_64() + SAMPLE_PERIOD;
			ReleaseWaitCount = RELEASE_WAIT_TIME;
			ReadIICRWaitTime = READ_IICR_WAIT_TIME;
			SleepWaitTime 	 = SLEEP_WAIT_TIME;
		}
	}

	// Sleep Mode Control
	if(SleepCount > SleepWaitTime)	bSleepMode = TRUE;
	else							SleepCount++;

	// EC Battery Status Read
	if(ReadIICRWaitCount > READ_IICR_WAIT_TIME)	{
		ReadIICRWaitCount = 0;
		read_iicr = read_ec_iicr();		read_iicr_count++;
	}
	else	ReadIICRWaitCount++;

	#if	(DEBUG_MSG)
		if(bOldSleepMode != bSleepMode)	{
			bOldSleepMode = bSleepMode;
			
			if(bSleepMode)	printk("wbts current mode = sleep mode !!! \n\r");
			else			printk("wbts current mode = active mode !!! \n\r");
		}
	#endif
	
	add_timer(data);
	spin_unlock(&wbts_lock);
}

//----------------------------------------------------------------------------------------------------------------
static int wbts_open(struct input_dev *dev)
{
	unsigned long flags;

	spin_lock_irqsave(&wbts_lock, flags);

	#if	(DEBUG_MSG)
		printk("read status before init command = %x\n", inb(wbts_io + EC_STR_IDR_OFFSET));
	#endif

	outb(TS_INIT_CMD, wbts_io + EC_STR_IDR_OFFSET);

	read_iicr = read_ec_iicr();

	#if	(DEBUG_MSG)
		printk("read status after init command = %x\n", inb(wbts_io + EC_STR_IDR_OFFSET));
	#endif

	printk("wbts device driver open!!\n");

	spin_unlock_irqrestore(&wbts_lock, flags);

	return 0;
}

//----------------------------------------------------------------------------------------------------------------
static void wbts_close(struct input_dev *dev)
{
	unsigned long flags;

	spin_lock_irqsave(&wbts_lock, flags);
	
	#if	(DEBUG_MSG)
		printk("wbts device driver close!!\n");
	#endif

	spin_unlock_irqrestore(&wbts_lock, flags);
}

//----------------------------------------------------------------------------------------------------------------
static int __init wbts_init(void)
{
	int err;

	if (!request_region(wbts_io, 8, "wbts_io")) {
		printk(KERN_WARNING "wbts_io : unable to get IO region\n");
		return -ENODEV;
	}

	wbts = input_allocate_device();
	
	if (!wbts) {
		printk(KERN_ERR "wbts : not enough memory\n");		err = -ENOMEM;
		goto fail1;
	}

	wbts->name = "TechDien TouchScreen";
	wbts->phys = "isa03e0/input0";
	
	wbts->id.bustype = BUS_ISA;		wbts->id.vendor  = 0x0005;	
	wbts->id.product = 0x0001;		wbts->id.version = 0x0100;

	wbts->open	= wbts_open;
	wbts->close	= wbts_close;

	wbts->evbit[0] 	= BIT(EV_KEY) | BIT(EV_ABS);
	wbts->absbit[0] = BIT(ABS_X)  | BIT(ABS_Y);	
	wbts->keybit[LONG(BTN_TOUCH)] = BIT(BTN_TOUCH);

	input_set_abs_params(wbts, ABS_X, TS_ABS_MIN_X, TS_ABS_MAX_X, 0, 0);
	input_set_abs_params(wbts, ABS_Y, TS_ABS_MIN_Y, TS_ABS_MAX_Y, 0, 0);

	init_timer(&ts_timer);

	ts_timer.data 		= (unsigned long)&ts_timer;
	ts_timer.function 	= wbts_interrupt;
	ts_timer.expires	= get_jiffies_64() + (SAMPLE_PERIOD * DEFAULT_PERIOD);
	
	add_timer(&ts_timer);

	err = input_register_device(wbts);

	if (err)	{
		printk("wbts input register device fail!!\n");		err = -ENODEV;
		goto fail2;
	}

	if(init_proc_filesystem())	{
		printk("wbts proc file system install fail!![/proc/touchscreen]\n");
	}
	else	{
		printk("wbts proc file system install sucess!![/proc/touchscreen]\n");
	}

	printk("wbts device driver install sucess!(kernel timer based)\n");
	printk("wbts.ko device driver version 1.0.1 - 2008.02.28\n");

	return 0;

	fail2:
	 	input_free_device(wbts);

	fail1:	
		release_region(wbts_io, 8);
	
	return err;
}

//----------------------------------------------------------------------------------------------------------------
static void __exit wbts_exit(void)
{
	input_unregister_device(wbts);

	del_timer(&ts_timer);

	release_region(wbts_io, 8);
	
	remove_proc_entry(WBTS_PROC_EC_IICR_DATA_NAME, 	wbts_root_fp);
	remove_proc_entry(WBTS_PROC_PERIOD_DATA_NAME, 	wbts_root_fp);
	remove_proc_entry(WBTS_PROC_RAW_DATA_NAME, 		wbts_root_fp);
	remove_proc_entry(WBTS_PROC_ROOT_NAME, 			0);

	printk("wbts device driver remove sucess!(kernel timer based)\n");

}

//----------------------------------------------------------------------------------------------------------------

