#include <gtk/gtk.h>

void checkInBufferFull();
void checkOutBufferFull();
void readFanSpeed(unsigned char *);
void writeFanSpeed(int);
int iFindFanMode();
unsigned char readKeypad();
