#include <stdio.h>
#include <stdlib.h>

static unsigned char SilentMode[11] = {0x32, 0x01, 0x37, 0x01, 0x3B, 0x02, 0x3C, 0x03, 0x07, 0x09, 0x0B};
static unsigned char NormalMode[11] = {0x2D, 0x01, 0x32, 0x01, 0x37, 0x02, 0x38, 0x03, 0x0C, 0x0E, 0x10};
static unsigned char CoooolMode[11] = {0x2D, 0x01, 0x32, 0x01, 0x37, 0x02, 0x38, 0x03, 0x0E, 0x14, 0x1A};

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
