#include <stdio.h>
#include <stdlib.h>

static unsigned char SilentMode[11] = {0x32, 0x01, 0x37, 0x01, 0x3B, 0x02, 0x3C, 0x03, 0x07, 0x09, 0x0B};
static unsigned char NormalMode[11] = {0x2D, 0x01, 0x32, 0x01, 0x37, 0x02, 0x38, 0x03, 0x0C, 0x0E, 0x10};
static unsigned char CoooolMode[11] = {0x2D, 0x01, 0x32, 0x01, 0x37, 0x02, 0x38, 0x03, 0x0E, 0x14, 0x1A};

#if 1

#define TRY_MAX_NUM 100000L

#include <asm/io.h>

void checkInBufferFull()
{
	volatile long i = TRY_MAX_NUM;

	while(i-- && (inb(0x64) & 0x02));
}

void checkOutBufferFull()
{
	volatile long i = TRY_MAX_NUM;

	while(i-- && !(inb(0x64) & 0x01));
}

void readFanSpeed(unsigned char * uc)
{
	int i = 0;
	
	if (ioperm(0x60, 6, 1)) {
		perror("ioperm");
		exit(1);
	}

	for(i = 0; i < 4; i++) {
	   	checkInBufferFull();	
		outb(0xB9, 0x64);
	
		checkInBufferFull();	
		outb(0xEC, 0x60);
	    
		checkInBufferFull();	
		outb(0x2A + i, 0x60);

		checkOutBufferFull();	
		uc[i] = inb(0x60);
	}
	    
	if (ioperm(0x60, 6, 0)) {
		perror("ioperm");
		exit(1);
	}
}

void writeFanSpeed(int mode)
{
	int i = 0;
	unsigned char uc[11] = {0, };
	char rwFan[] = {'w'};
  
	if (ioperm(0x60, 12, 1)) {
		perror("ioperm");
		exit(1);
	}

	for(i = 0; i < 11; i++) {
		switch(mode) {
			case 0:
				uc[i] = SilentMode[i];
				break;
			case 1:
				uc[i] = NormalMode[i];
				break;
			case 2:
				uc[i] = CoooolMode[i];
				break;
			default: ;
		}

		checkInBufferFull();
		outb(0xB8, 0x64);

		checkInBufferFull();
		outb(0xEC, 0x60);

		checkInBufferFull();
		outb(0x23 + i, 0x60);

		checkInBufferFull();
		outb(uc[i], 0x60);
	}
	
	if (ioperm(0x60, 12, 0)) {
		perror("ioperm");
		exit(1);
	}

   	RWconfig(100, mode, rwFan);
}

int iFindFanMode()
{
	int i = 0;
	unsigned char c[4] = {0, };
	
	readFanSpeed(c);

	for(i = 0; i < 4; i++)
		if(SilentMode[i+7] != c[i])
			break;
	if(i >= 3)
		return 0;

	for(i = 0; i < 4; i++)
		if(NormalMode[i+7] != c[i])
			break;
	if(i >= 3)
		return 1;

	for(i = 0; i < 4; i++)
		if(CoooolMode[i+7] != c[i])
			break;
	if(i >= 3)
		return 2;

	return -1;
}

unsigned char readKeypad()
{
	unsigned char uc = 0;

	if (ioperm(0x60, 6, 1)) {
		perror("ioperm");
		exit(1);
	}

	checkInBufferFull();
	outb(0xB9, 0x64);

	checkInBufferFull();
	outb(0xEC, 0x60);

	checkInBufferFull();
	outb(0xAC, 0x60);

	checkOutBufferFull();
	uc = inb(0x60);
	
	if (ioperm(0x60, 6, 0)) {
		perror("ioperm");
		exit(1);
	}

	return uc;
}

int iFindKeyLEDMode()
{
	unsigned char uc = 0;

	uc = readKeypad();

	switch(uc & 0xF0) {
		case 0x00:
			return 0;
		case 0x10:
			return 1;
		case 0x30:
			return 2;
		default: ;
	}
	return -1;
}


void writeKeypad(mode)
{
	unsigned char olduc = 0;
	unsigned char uc = 0;
	char rwKeypad[] = {'w'};
	
	uc = readKeypad();
	
	olduc = uc & 0x0F;

	switch(mode) {
		case 0: 
			uc = olduc | 0x00;
			break;
		case 1:
			uc = olduc | 0x10;
			break;
		case 2:
			uc = olduc | 0x30;
			break;
		default: ;
	}
	
	if (ioperm(0x60, 6, 1)) {
		perror("ioperm");
		exit(1);
	}

	checkInBufferFull();
	outb(0xB8, 0x64);

	checkInBufferFull();
	outb(0xEC, 0x60);

	checkInBufferFull();
	outb(0xAC, 0x60);

	checkInBufferFull();
	outb(uc, 0x60);
	
	if (ioperm(0x60, 6, 0)) {
		perror("ioperm");
		exit(1);
	}
   	RWconfig(200, mode, rwKeypad);
}

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEV_PORT "/dev/port"

void checkInBufferFull()
{
	int fd = 0;
	char c = 0;
	volatile long i = TRY_MAX_NUM;

	if((fd = open(DEV_PORT, O_RDWR)) == -1) {
		perror("open()");
		exit(1);
	}

	lseek(fd, 0x64, SEEK_SET);
	
	do {
	   	read(fd, (void *)&c, 1);

	} while(i-- && (c & 0x02));

	close(fd);
}

void checkOutBufferFull()
{
	int fd = 0;
	char c = 0;
	volatile long i = TRY_MAX_NUM;

	if((fd = open(DEV_PORT, O_RDWR)) == -1) {
		perror("open()");
		exit(1);
	}

	lseek(fd, 0x64, SEEK_SET);
	
	do {
	   	read(fd, (void *)&c, 1);

	} while(i-- && !(c & 0x01));

	close(fd);
}

void readFanSpeed(unsigned char * uc)
{
	int fd = 0;
	int i = 0;

	if((fd = open(DEV_PORT, O_RDWR)) == -1) {
		perror("open()");
		exit(1);
	}

	for(i = 0; i < 4; i++) {
	   	lseek(fd, 0x64, SEEK_SET);
	   	write(fd, (void *)0xB9, 1);
	
		lseek(fd, 0x60, SEEK_SET);
	   	write(fd, (void *)0xEC, 1);

		lseek(fd, 0x60, SEEK_SET);
	   	write(fd, (void *)0x2A + i, 1); 
		
		lseek(fd, 0x60, SEEK_SET);
	   	read(fd, (void *)&uc[i], sizeof(uc)); 
		
		g_printf("0x%x\n", uc);
	}

	close(fd);
}

#endif
