
/***************************************************************************
 * callbacks.c:
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#define CONFIGURATION_FILE "/etc/mfmgr.conf"

gboolean inFocusTouchpad = FALSE;

int RWconfig(int Mode, int value, char * rw)
{
	FILE * f;
	char type[16] = {0, };
	char buf[255] = {0, };
	long positionFile = 0;
	char av = value + '0';
	
	if((f = fopen(CONFIGURATION_FILE, "r+t")) == NULL) {
		printf("File Not Found : %s", CONFIGURATION_FILE);
		return -1;
	}
	
	memset(type, 0, sizeof(type));
	switch (Mode) {
		case 100: //- Fan
			strcpy(type, "FanSpeed");
			break;
		case 200: //- Keypad LED
			strcpy(type, "KeypadLED");
			break;
		case 300: //- TouchPad
			strcpy(type, "TouchPad");
			break;
		default: ;
	}
				
	while(!feof(f)) {
		fgets(buf, sizeof(buf), f);
		if(strncmp(buf, type, strlen(type)) == 0) {
			
			positionFile = ftell(f);
			fseek(f, positionFile-2, SEEK_SET);

			if(rw[0] == 'r')
				fread(&rw[0], 1, 1, f);
			else
			   	fwrite(&av, 1, 1, f);

			break;
		}
	}

	fclose(f);

	return 0;
}

void
on_MFmgr_show                          (GtkWidget       *widget,
                                        gpointer         user_data)
{
	get_active_FanMode();
	get_active_KeyLED();
	get_active_Synaptics();
}

void get_active_FanMode()
{
#if 1
	char fanModeR[] = {'r'};
	int fanMode = 0;

   	RWconfig(100, 1, fanModeR);
	fanMode = atoi(fanModeR);
	//- writeFanSpeed(fanMode);

	switch(fanMode) {
#else
	int fanMode = -1;

	fanMode = iFindFanMode();

	switch(fanMode) {
#endif
		case 0: //- Silent Mode
		   	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobtn_Fan_Silent), TRUE);
			gtk_window_set_focus(MFmgr, radiobtn_Fan_Silent);
			break;
		case 1: //- Normal Mode
		   	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobtn_Fan_Normal), TRUE);
			gtk_window_set_focus(MFmgr, radiobtn_Fan_Normal);
			break;
		case 2: //- Cool Mode
		   	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobtn_Fan_Cool), TRUE);
			gtk_window_set_focus(MFmgr, radiobtn_Fan_Cool);
			break;
		default: ;
	}
}

void get_active_KeyLED()
{
#if 1
	char keyLEDR[] = {'r'};
	int keyLED = 0;

   	RWconfig(200, 1, keyLEDR);
	keyLED = atoi(keyLEDR);
	writeKeypad(keyLED);

	switch(keyLED) {
#else
	int keyPadLEDMode = -1;

	keyPadLEDMode = iFindKeyLEDMode();

	switch(keyPadLEDMode) {
#endif
		case 0:
		   	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobtn_Off), TRUE);
			break;
		case 1:
		   	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobtn_Auto), TRUE);
			break;
		case 2:
		   	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobtn_LED_On), TRUE);
			break;
		default: ;
	}
}

void get_active_Synaptics()
{
#if 1
	int retSystem = 0;
	char touchPadR[] = {'r'};
	
   	RWconfig(300, 1, touchPadR);

	switch(atoi(touchPadR)) {
		case 0: //- disable
			retSystem = system("synclient TouchpadOff=1");
		   	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobtn_touchPad_disable), TRUE);
			break;
		case 1: //- only
			retSystem = system("synclient TouchpadOff=2");
		   	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobtn_touchPad_only), TRUE);
			break;
		case 2: //- enable
			retSystem = system("synclient TouchpadOff=0");
		   	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobtn_touchPad_enable), TRUE);
			break;
		default: ;
	}
#else
	int retSystem = 0;
	retSystem = system("synclient TouchpadOff=0");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobtn_touchPad_enable), TRUE);
#endif
}


void
on_MFmgr_destroy                       (GtkObject       *object,
                                        gpointer         user_data)
{
	int retSystem = 0;
	retSystem = system("killall mfmgr");
}


gboolean
on_radiobtn_Fan_Silent_focus_in_event  (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	writeFanSpeed(0); 
	
	return FALSE;
}


gboolean
on_radiobtn_Fan_Normal_focus_in_event  (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	writeFanSpeed(1); 
	
	return FALSE;
}


gboolean
on_radiobtn_Fan_Cool_focus_in_event    (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	writeFanSpeed(2); 
	
	return FALSE;
}


gboolean
on_radiobtn_LED_On_focus_in_event      (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	writeKeypad(2); 
	
	return FALSE;
}


gboolean
on_radiobtn_Auto_focus_in_event        (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	writeKeypad(1);

	return FALSE;
}


gboolean
on_radiobtn_Off_focus_in_event         (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	writeKeypad(0);
   
	return FALSE;
}


gboolean
on_radiobtn_touchPad_enable_focus_in_event
                                        (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	int retSystem = 0;
	char rwtouchena[] = {'w'};

	retSystem = system("synclient TouchpadOff=0"); 
   	RWconfig(300, 2, rwtouchena);
	
	return FALSE;
}


gboolean
on_radiobtn_touchPad_only_focus_in_event
                                        (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	int retSystem = 0;
   	char rwtouchonly[] = {'w'};

	retSystem = system("synclient TouchpadOff=2");
   	RWconfig(300, 1, rwtouchonly);

	return FALSE;
}


void
on_radiobtn_touchPad_disable_released  (GtkButton       *button,
                                        gpointer         user_data)
{
}

void
on_radiobtn_touchPad_disable_toggled   (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	int retSystem = 0;
	char rwtouchdis[] = {'w'};

	if(inFocusTouchpad) {
		retSystem = system("synclient TouchpadOff=1");
	   	RWconfig(300, 0, rwtouchdis);

		inFocusTouchpad = FALSE;
	}
}


gboolean
on_radiobtn_touchPad_disable_focus_in_event
                                        (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	inFocusTouchpad = TRUE;

	return FALSE;
}


void
on_btn_Calibrate_released              (GtkButton       *button,
                                        gpointer         user_data)
{
	int retSystem = 0;

	retSystem = system("/usr/local/bin/calibrator /dev/input/touchscreen"); 
}


