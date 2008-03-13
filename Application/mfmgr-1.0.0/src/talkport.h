
/***************************************************************************
 * talkport.h:
 *
 * This file is part of mfmgr Application
 *
 * Copyright (C) 2007-2008 Wibrain, inc.
 ***************************************************************************/
/***************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * See the file COPYING for more information.
 ***************************************************************************/

#include <gtk/gtk.h>

void checkInBufferFull();
void checkOutBufferFull();
void readFanSpeed(unsigned char *);
void writeFanSpeed(int);
int iFindFanMode();
unsigned char readKeypad();
