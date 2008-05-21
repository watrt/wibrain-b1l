//----------------------------------------------------------------------------------------------------------------
//
//  TECHDINE R4510SP Model
// 
//  Copyright (c) 2007-2008 Wibrain Corporation (charles.park@wibrain.com)
// 
//----------------------------------------------------------------------------------------------------------------
#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/device.h>

#include <linux/stat.h>
#include <linux/proc_fs.h>

#include <asm/io.h>
#include <asm/uaccess.h>

// Interrupt (IO-APIC) control
#include <mach_apic.h>
#include <mach_apicdef.h>

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
//			Write		IDR3 Write(EC Data Register Write)
//						
//	0x3E4		Read		STR3 Read(EC Status Register Read)
//			Write		IDR3 Write(EC Command Register Write)
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
#define	DEBUG_MSG		0

#define	TRUE			1
#define	FALSE			0

#define	DEFAULT_PERIOD		(HZ)		// 1 sec

#define	TS_EVENT_PERIOD		(HZ/20)		// 50 ms

#define	TS_EVENT_ADJUST		(HZ/100)	// 10 ms

#define	READ_IICR_WAIT_TIME	30			// 30 sec

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
#define	OUTBUF_FULL		0x01
#define	INBUF_FULL		0x02

#define	DELAY_TIME		10		/* response wait time (max 10ms) */
//----------------------------------------------------------------------------------------------------------------
// TS Initialize Command 
//----------------------------------------------------------------------------------------------------------------
#define	TS_INIT_CMD		0xA7

//----------------------------------------------------------------------------------------------------------------
// TS Status 
//----------------------------------------------------------------------------------------------------------------
#define	TS_SYNC_BIT		0x80
#define	TS_PEN_DOWN		0x40

#define	TS_ABS_MAX_X		0x3FF
#define	TS_ABS_MAX_Y		0x3FF

#define	TS_ABS_MIN_X		0
#define	TS_ABS_MIN_Y		0

//----------------------------------------------------------------------------------------------------------------
// Function proto type define
//----------------------------------------------------------------------------------------------------------------
static	unsigned char	read_ec_iicr		(void);

static	int		in_buffer_empty_check	(void);
static	int		out_buffer_full_check	(void);
static	int		read_ec_iicr_data_proc	(char *page, char **start, off_t off, int count, int *eof, void *data);

static	int		read_adjust_data_proc	(char *page, char **start, off_t off, int count, int *eof, void *data);
static	int		write_adjust_data_proc	(struct file *file, const char __user *buffer, unsigned long count, void *data);
static	int		read_ec_iicr_data_proc	(char *page, char **start, off_t off, int count, int *eof, void *data);
static	int		read_raw_data_proc	(char *page, char **start, off_t off, int count, int *eof, void *data);
static	int 		init_proc_filesystem	(void);

static void 		set_interrupt_mode	(unsigned char trigger, unsigned char polarity);
static void		wbts_interrupt_init	(void);

// IRQ 10 Handler
static irqreturn_t	wbts_ts_interrupt	(int irq, void *dev_id);

// Timer Interrupt Handler
static void		wbts_ts_event_timer_set	(void);
static void 		wbts_ts_event_interrupt	(unsigned long arg);
static void 		wbts_ec_check_interrupt	(unsigned long arg);

static int 		wbts_open		(struct input_dev *dev);
static void 		wbts_close		(struct input_dev *dev);

static void 		wbts_release_device	(struct device *dev);
static int 		wbts_resume		(struct device *dev);
static int 		wbts_suspend		(struct device *dev, pm_message_t state);

static int __devinit 	wbts_probe		(struct device *pdev);
static int __devexit 	wbts_remove		(struct device *pdev);

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
// platform device driver init
//----------------------------------------------------------------------------------------------------------------
struct platform_device wbts_platform_device_driver = {
	.name		= "wbts",
	.id		= 0,
	.num_resources	= 0,
	.dev	= {
		.release	= wbts_release_device,
	},
};

struct device_driver wbts_device_driver = {
	.owner		= THIS_MODULE,
	.name		= "wbts",
	.bus		= &platform_bus_type,
	.probe		= wbts_probe,
	.remove		= __devexit_p(wbts_remove),
	.suspend	= wbts_suspend,
	.resume		= wbts_resume,
};

//----------------------------------------------------------------------------------------------------------------
// IO-APIC entry struct
//----------------------------------------------------------------------------------------------------------------
struct	io_apic	{
	unsigned int	index;
	unsigned int	unused[3];
	unsigned int	data;
};

union	entry_union	{
	struct	{	u32 w1, w2;	};
	struct	IO_APIC_route_entry	entry;
};

//----------------------------------------------------------------------------------------------------------------
// Module define
//----------------------------------------------------------------------------------------------------------------
module_init(wbts_init);
module_exit(wbts_exit);

//----------------------------------------------------------------------------------------------------------------
// Global variable define
//----------------------------------------------------------------------------------------------------------------
static	struct	input_dev	*wbts;

static 	struct 	timer_list	ts_event_timer, ec_check_timer;

static	DEFINE_SPINLOCK(wbts_lock);

/* TS Data Buffer */
static	unsigned char	ts_raw_data[5] = {0,};

//----------------------------------------------------------------------------------------------------------------
// Proc filesystem define
//----------------------------------------------------------------------------------------------------------------
#define	WBTS_PROC_ROOT_NAME			"touchscreen"
#define	WBTS_PROC_RAW_DATA_NAME			"raw_data"
#define	WBTS_PROC_EC_IICR_DATA_NAME		"ec_iicr"
#define	WBTS_PROC_ADJUST_DATA_NAME		"adjust_data"

//----------------------------------------------------------------------------------------------------------------
static	struct	proc_dir_entry	*wbts_root_fp		= NULL;		// /proc/touchscreen
static	struct	proc_dir_entry	*wbts_raw_data_fp	= NULL;		// /proc/touchscreen/raw_data
static	struct	proc_dir_entry	*wbts_ec_iicr_data_fp	= NULL;		// /proc/touchscreen/ec_iicr
static	struct	proc_dir_entry	*wbts_adjust_data_fp	= NULL;		// /proc/touchscreen/adjust_data

static	char		adjust_data[PAGE_SIZE - 80];
static	unsigned int	ts_event_adjust_value = 0;

static	int		sync_status = 0;
static	int		sync_x = 0;
static	int		sync_y = 0;

static	unsigned int	read_iicr_count = 0;
static	unsigned char	read_iicr = 0;

static	unsigned char	bSuspend = FALSE;

static	unsigned char	bEventTimerExpired = true;

//----------------------------------------------------------------------------------------------------------------
static	int	in_buffer_empty_check(void)
{
	unsigned long 	flags;
	// wait max 10ms
	unsigned char	count = DELAY_TIME, rd_data;
	
	while(count--)	{
		spin_lock_irqsave(&wbts_lock, flags);
		rd_data = inb(EC_0x6064_BASE_ADDR + EC_STR_IDR_OFFSET);
		spin_unlock_irqrestore(&wbts_lock, flags);
		
		if(!(rd_data & INBUF_FULL))	return	1;
		
		mdelay(1);
	}
	
	return	0;
}

//--------------------------------------------------------------------------------------------------------------
static	int	out_buffer_full_check(void)
{
	unsigned long 	flags;
	// wait max 10ms
	unsigned char	count = DELAY_TIME, rd_data;

	while(count--)	{
		spin_lock_irqsave(&wbts_lock, flags);
		rd_data = inb(EC_0x6064_BASE_ADDR + EC_STR_IDR_OFFSET);
		spin_unlock_irqrestore(&wbts_lock, flags);
		
		if(rd_data & OUTBUF_FULL)	return	1;
			
		mdelay(1);
	}
	
	return	0;
}

//--------------------------------------------------------------------------------------------------------------
static	unsigned char	read_ec_iicr(void)
{
	unsigned char	rd_byte = 0x00;
	
	if(in_buffer_empty_check())	outb(0xB9, EC_0x6064_BASE_ADDR + EC_STR_IDR_OFFSET);
	else				printk("InBuffer Full Error!!\n");

	if(in_buffer_empty_check())	outb(0xFF, EC_0x6064_BASE_ADDR);
	else				printk("InBuffer Full Error!!\n");

	if(in_buffer_empty_check())	outb(0xD8, EC_0x6064_BASE_ADDR);
	else				printk("InBuffer Full Error!!\n");

	if(out_buffer_full_check())	rd_byte = inb(EC_0x6064_BASE_ADDR);
	else				printk("Outbuffer Empty Error!!\n");

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
static	int	read_adjust_data_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char	*buf = page;

	buf += sprintf(buf, "Penup-event wait time = (%d) ms\n", 50 + (ts_event_adjust_value * 10));
		
	return	(int)(buf-page);
}

//----------------------------------------------------------------------------------------------------------------
static	int	write_adjust_data_proc(struct file *file, const char __user *buffer, unsigned long count, void *data)
{
	char	*buf = (char *)data;
	int		len;
	
	if(copy_from_user(buf, buffer, count))	return	-EFAULT;
	
	buf[count] = '\0';
	
	len = strlen(buf);
	
	if(buf[len - 1] == '\n')	buf[len-1] = 0;
	
	ts_event_adjust_value = simple_strtoul(buf, NULL, 10);
	
	if(ts_event_adjust_value < 0)		ts_event_adjust_value = 0;
	
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
			wbts_raw_data_fp->data 		= NULL;
			wbts_raw_data_fp->read_proc	= read_raw_data_proc;
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
		
		wbts_ec_iicr_data_fp	= create_proc_entry(WBTS_PROC_EC_IICR_DATA_NAME, S_IFREG | S_IRWXU | S_IRWXO | S_IRWXG, wbts_root_fp);

		if(wbts_ec_iicr_data_fp)	{
			wbts_ec_iicr_data_fp->data		= NULL;
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

		wbts_adjust_data_fp	= create_proc_entry(WBTS_PROC_ADJUST_DATA_NAME, S_IFREG | S_IRWXU | S_IRWXO | S_IRWXG, wbts_root_fp);
		
		if(wbts_adjust_data_fp)	{
			wbts_adjust_data_fp->data	= adjust_data;
			wbts_adjust_data_fp->read_proc	= read_adjust_data_proc;
			wbts_adjust_data_fp->write_proc	= write_adjust_data_proc;

			#if	(DEBUG_MSG)
				printk("wbts adjust filesystem create sucess!(/proc/touchscreen/adjust_data)\n");
			#endif
		}
		else	{
			#if	(DEBUG_MSG)
				printk("wbts adjust filesystem create fail!(/proc/touchscreen/adjust_data)\n");
			#endif
			return	1;
		}
	}
	else	{
		#if	(DEBUG_MSG)
			printk("wbts proc filesystem create fail!(/proc/touchscreen)\n");
		#endif
		return	1;
	}

	return	0;
}

//----------------------------------------------------------------------------------------------------------------
static	void			wbts_ts_event_interrupt(unsigned long arg)
{
	unsigned long 	flags;

	spin_lock_irqsave(&wbts_lock, flags);

	sync_status = FALSE;		// Pen Up

	input_report_key(wbts, BTN_TOUCH, sync_status);
	input_report_abs(wbts, ABS_X, sync_x);			
	input_report_abs(wbts, ABS_Y, sync_y);
	input_sync(wbts);

	bEventTimerExpired	= TRUE;

	spin_unlock_irqrestore(&wbts_lock, flags);

	#if	(DEBUG_MSG)
		printk("timer expired!!touch up!!\n ");
	#endif

}

//----------------------------------------------------------------------------------------------------------------
static	void			wbts_ts_event_timer_set(void)
{
	if(!bEventTimerExpired)
		del_timer_sync(&ts_event_timer);

	init_timer(&ts_event_timer);

	ts_event_timer.data 		= (unsigned long)&ts_event_timer;
	ts_event_timer.function 	= wbts_ts_event_interrupt;
	ts_event_timer.expires		= jiffies + TS_EVENT_PERIOD + (TS_EVENT_ADJUST * ts_event_adjust_value);
	
	add_timer(&ts_event_timer);
	
	bEventTimerExpired	= FALSE;		
}
	
//----------------------------------------------------------------------------------------------------------------
static	irqreturn_t		wbts_ts_interrupt(int irq, void *dev_id)
{
	static	unsigned char	count = 0xFF, rd_data = 0;
			unsigned long 	flags;

	spin_lock_irqsave(&wbts_lock, flags);

	// timer disable
	// timer enable (pen up event) : 5ms
	wbts_ts_event_timer_set();

	if(inb(wbts_io + EC_STR_IDR_OFFSET) & OUTBUF_FULL)		{
		rd_data = inb(wbts_io);
		
		if(rd_data & TS_SYNC_BIT)	{	ts_raw_data[0]     = rd_data;	count = 1;	}
		else				{	ts_raw_data[count] = rd_data;	count++;	}
	
		if(count >= 3)	{
		
			count = 0;
			sync_x = (ts_raw_data[1] & 0x7F) | ((ts_raw_data[0] & 0x38)<<4);
			sync_y = (ts_raw_data[2] & 0x7F) | ((ts_raw_data[0] & 0x07)<<7);

			// swap x pos
			sync_x = 0x3FF - sync_x;

			sync_status = TRUE;		// Pen Down

			input_report_key(wbts, BTN_TOUCH, sync_status);
			input_report_abs(wbts, ABS_X, sync_x);			
			input_report_abs(wbts, ABS_Y, sync_y);
			input_sync(wbts);
		}
	}

	spin_unlock_irqrestore(&wbts_lock, flags);

	return	IRQ_HANDLED;
}

//----------------------------------------------------------------------------------------------------------------
//
// 100ms timer interrupt
//
//----------------------------------------------------------------------------------------------------------------
static	void 		wbts_ec_check_interrupt(unsigned long arg)
{
	struct timer_list *data = (struct timer_list *)arg;

	// EC IICR Read period : 30 sec
	static	unsigned int	ReadIICRWaitCount = 0;

	init_timer(data);

	data->data	= (unsigned long)data;
	data->function	= wbts_ec_check_interrupt;
	data->expires	= jiffies + DEFAULT_PERIOD;

	// EC Battery Status Read
	if(ReadIICRWaitCount > READ_IICR_WAIT_TIME)	{
		ReadIICRWaitCount = 0;
		read_iicr = read_ec_iicr();		read_iicr_count++;
	}
	else	ReadIICRWaitCount++;
	
	if(!bSuspend)		add_timer(data);
}

//----------------------------------------------------------------------------------------------------------------
//
//	IO-APIC Information(IRQ redirection table)
//
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | NR | Log | Phy | Mask | Tirg | IRR | Pol | Stat | Dest | Deli | Vect | IRQ to PIN Map |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 00 | 000 |  00 |  1   |  0   |  0  |  0  |   0  |   0  |  0   |  00  | IRQ0  -> 0:2   |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 01 | 001 |  01 |  0   |  0   |  0  |  0  |   0  |   1  |  1   |  39  | IRQ1  -> 0:1   |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 02 | 001 |  01 |  0   |  0   |  0  |  0  |   0  |   1  |  1   |  31  |                |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 03 | 001 |  01 |  0   |  0   |  0  |  0  |   0  |   1  |  1   |  41  | IRQ3  -> 0:3   |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 04 | 001 |  01 |  0   |  0   |  0  |  0  |   0  |   1  |  1   |  49  | IRQ4  -> 0:4   |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 05 | 001 |  01 |  0   |  0   |  0  |  0  |   0  |   1  |  1   |  51  | IRQ5  -> 0:5   |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 06 | 001 |  01 |  0   |  0   |  0  |  0  |   0  |   1  |  1   |  59  | IRQ6  -> 0:6   |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 07 | 001 |  01 |  0   |  0   |  0  |  0  |   0  |   1  |  1   |  61  | IRQ7  -> 0:7   |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 08 | 001 |  01 |  0   |  0   |  0  |  0  |   0  |   1  |  1   |  69  | IRQ8  -> 0:8   |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 09 | 001 |  01 |  1   |  1   |  0  |  1  |   0  |   1  |  1   |  71  | IRQ9  -> 0:9   |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 0a | 001 |  01 |  0   |  0   |  0  |  1  |   0  |   1  |  1   |  79  | IRQ10 -> 0:10  |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 0b | 001 |  01 |  0   |  0   |  0  |  0  |   0  |   1  |  1   |  81  | IRQ11 -> 0:11  |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 0c | 001 |  01 |  0   |  0   |  0  |  0  |   0  |   1  |  1   |  89  | IRQ12 -> 0:12  |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 0d | 001 |  01 |  0   |  0   |  0  |  0  |   0  |   1  |  1   |  91  | IRQ13 -> 0:13  |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 0e | 001 |  01 |  0   |  0   |  0  |  0  |   0  |   1  |  1   |  99  | IRQ14 -> 0:14  |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 0f | 001 |  01 |  0   |  0   |  0  |  0  |   0  |   1  |  1   |  A1  | IRQ15 -> 0:15  |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+----------------+
//  | 10 | 000 |  00 |  1   |  0   |  0  |  0  |   0  |   0  |  0   |  00  |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+
//  | 11 | 000 |  00 |  1   |  0   |  0  |  0  |   0  |   0  |  0   |  00  |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+
//  | 12 | 000 |  00 |  1   |  0   |  0  |  0  |   0  |   0  |  0   |  00  |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+
//  | 13 | 000 |  00 |  1   |  0   |  0  |  0  |   0  |   0  |  0   |  00  |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+
//  | 14 | 000 |  00 |  1   |  0   |  0  |  0  |   0  |   0  |  0   |  00  |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+
//  | 15 | 000 |  00 |  1   |  0   |  0  |  0  |   0  |   0  |  0   |  00  |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+
//  | 16 | 000 |  00 |  1   |  0   |  0  |  0  |   0  |   0  |  0   |  00  |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+
//  | 17 | 000 |  00 |  1   |  0   |  0  |  0  |   0  |   0  |  0   |  00  |
//  +----+-----+-----+------+------+-----+-----+------+------+------+------+
//
//----------------------------------------------------------------------------------------------------------------
//
// Low   edge trigger : trigger = 0, polarity = 0	-> IO_APIC Default(IRQ 10)
// High  edge trigger : trigger = 0, polarity = 1	-> Touchscreen IRQ mode
// Low  level trigger : trigger = 1, polarity = 0
// High level trigger : trigger = 1, polarity = 1
//
//----------------------------------------------------------------------------------------------------------------
#define		TRIGGER_LEVEL	1
#define		TRIGGER_EDGE	0
#define		POLARITY_HIGH	1
#define		POLARITY_LOW	0

//----------------------------------------------------------------------------------------------------------------
static void set_interrupt_mode(unsigned char trigger, unsigned char polarity)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	struct	io_apic	__iomem *io_apic = (struct	io_apic	__iomem *)0xFFFFC000;	// Gusty
#else
	struct	io_apic	__iomem *io_apic = (struct	io_apic	__iomem *)0xFFFFA000;	// Hardy
#endif
	union	entry_union			eu;
	struct	IO_APIC_route_entry	entry;
	
	memset(&entry, 0x00, sizeof(entry));
	
	// IO-APIC Interrupt Entry (IRQ 10)
	entry.vector				= 0x79;
	entry.delivery_mode			= 1;
	entry.dest_mode				= 1;
	entry.delivery_status			= 0;
	entry.polarity				= polarity;
	entry.irr				= 0;
	entry.trigger				= trigger;
	entry.mask				= 0;
	entry.dest.physical.physical_dest 	= 1;
	entry.dest.logical.logical_dest		= 1;
	
	eu.entry = entry;
	
	// Update IO-APIC entry(IRQ 10)
	// 0x11 + pin*2 : 0x25 (pin == 10)
	// 0x10 + pin*2 : 0x24 (pin == 10)
	writel(0x25, &io_apic->index);	writel(eu.w2, &io_apic->data);	mdelay(1);
	writel(0x24, &io_apic->index);	writel(eu.w1, &io_apic->data);	mdelay(1);
}

//----------------------------------------------------------------------------------------------------------------
static void	wbts_interrupt_init(void)
{
	unsigned long 	flags;
	unsigned char	rd_data;

	spin_lock_irqsave(&wbts_lock, flags);

	#if	(DEBUG_MSG)
		printk("read status before init command = %x\n", inb(wbts_io + EC_STR_IDR_OFFSET));		mdelay(1);
	#endif

//	set_interrupt_mode(TRIGGER_EDGE, POLARITY_HIGH);	mdelay(1);	// High edge trigger interrupt
	set_interrupt_mode(TRIGGER_LEVEL, POLARITY_HIGH);	mdelay(1);	// High edge trigger interrupt

	outb(TS_INIT_CMD, wbts_io + EC_STR_IDR_OFFSET);		mdelay(1);		

	// Interrupt Clear
	while(inb(wbts_io + EC_STR_IDR_OFFSET) & OUTBUF_FULL)	{
		mdelay(1);		rd_data = inb(wbts_io);		mdelay(1);		
	}

	mdelay(1);		
//	read_iicr = read_ec_iicr();

	#if	(DEBUG_MSG)
		printk("read status after init command = %x\n", inb(wbts_io + EC_STR_IDR_OFFSET));
		printk("wbts device driver open!!\n");
	#endif

	bSuspend = FALSE;	bEventTimerExpired = TRUE;
	
	init_timer(&ec_check_timer);

	ec_check_timer.data 		= (unsigned long)&ec_check_timer;
	ec_check_timer.function 	= wbts_ec_check_interrupt;
	ec_check_timer.expires		= jiffies + DEFAULT_PERIOD;
	
	add_timer(&ec_check_timer);

	spin_unlock_irqrestore(&wbts_lock, flags);
}

//----------------------------------------------------------------------------------------------------------------
static int wbts_open(struct input_dev *dev)
{
	#if	(DEBUG_MSG)
		printk("wbts device driver open!!\n");
	#endif

	wbts_interrupt_init();
	
	return 0;
}

//----------------------------------------------------------------------------------------------------------------
static void wbts_close(struct input_dev *dev)
{
	#if	(DEBUG_MSG)
		printk("wbts device driver close!!\n");
	#endif
}

//----------------------------------------------------------------------------------------------------------------
static int wbts_suspend(struct device *dev, pm_message_t state)
{
	#if (DEBUG_MSG)
		printk("wbts_suspend!!!\n");
	#endif
	
	bSuspend = TRUE;
	
	return	0;
}

//----------------------------------------------------------------------------------------------------------------
static int wbts_resume(struct device *dev)
{
	#if (DEBUG_MSG)
		printk("wbts_resume!!!\n");
	#endif

	wbts_interrupt_init();

	return	0;
}

//----------------------------------------------------------------------------------------------------------------
static int __devinit wbts_probe(struct device *pdev)
{
	int err = 0;

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

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	wbts->evbit[0] 	= BIT(EV_KEY) | BIT(EV_ABS);
	wbts->absbit[0] = BIT(ABS_X)  | BIT(ABS_Y);	
	wbts->keybit[LONG(BTN_TOUCH)] = BIT(BTN_TOUCH);
#else
	wbts->evbit[0] 	= BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	wbts->absbit[0] = BIT_MASK(ABS_X)  | BIT_MASK(ABS_Y);	
	wbts->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
#endif

	input_set_abs_params(wbts, ABS_X, TS_ABS_MIN_X, TS_ABS_MAX_X, 0, 0);
	input_set_abs_params(wbts, ABS_Y, TS_ABS_MIN_Y, TS_ABS_MAX_Y, 0, 0);

	if(request_irq(wbts_irq, wbts_ts_interrupt, 0, "wbts", wbts)) {
		printk("wbts : unable to get IRQ !!!\n");
		goto fail1;
	}

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

	printk("wbts device driver install sucess!(kernel interrupt based)\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	printk("wbts.ko device driver version 1.0.2 - 2008.04.20 (Kernel version  < 2.6.24)\n");
#else
	printk("wbts.ko device driver version 1.0.2 - 2008.05.14 (Kernel version >= 2.6.24)\n");
#endif
	return 0;

	fail2:
		free_irq(wbts_irq, wbts);
	 	input_free_device(wbts);

	fail1:	
		release_region(wbts_io, 8);
	
	return err;
}

//----------------------------------------------------------------------------------------------------------------
static int __devexit wbts_remove(struct device *pdev)
{
	input_unregister_device(wbts);

	del_timer(&ec_check_timer);	 	// input_free_device(wbts);
	free_irq(wbts_irq, wbts);	release_region(wbts_io, 8);
	
	remove_proc_entry(WBTS_PROC_ADJUST_DATA_NAME, 	wbts_root_fp);
	remove_proc_entry(WBTS_PROC_EC_IICR_DATA_NAME, 	wbts_root_fp);
	remove_proc_entry(WBTS_PROC_RAW_DATA_NAME,	wbts_root_fp);
	remove_proc_entry(WBTS_PROC_ROOT_NAME, 		0);

	printk("wbts device driver remove sucess!(kernel interrupt based)\n");
	
	return	0;
}

//----------------------------------------------------------------------------------------------------------------
static void wbts_release_device(struct device *dev)
{
	#if (DEBUG_MSG)
		printk("wbts_release_device!!\n");
	#endif
}

//----------------------------------------------------------------------------------------------------------------
static int __init wbts_init(void)
{
	int ret = driver_register(&wbts_device_driver);

	#if (DEBUG_MSG)
		printk("driver_register %d \n", ret);
	#endif

	if(!ret)	{
		ret = platform_device_register(&wbts_platform_device_driver);

		#if (DEBUG_MSG)
			printk("platform_driver_register %d \n", ret);
		#endif

		if(ret)		driver_unregister(&wbts_device_driver);
	}
	return ret;
}

//----------------------------------------------------------------------------------------------------------------
static void __exit wbts_exit(void)
{
	#if	(DEBUG_MSG)
		printk("wbts_exit \n");
	#endif

	platform_device_unregister(&wbts_platform_device_driver);

	driver_unregister(&wbts_device_driver);
}

//----------------------------------------------------------------------------------------------------------------


