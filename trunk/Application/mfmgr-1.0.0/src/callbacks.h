
/***************************************************************************
 * callbacks.h:
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
#include "talkport.h"

int RWconfig(int, int, char *);

void
on_MFmgr_show                          (GtkWidget       *widget,
                                        gpointer         user_data);

void get_active_FanMode();

void get_active_KeyLED();

void get_active_Synaptics();

void
on_MFmgr_destroy                       (GtkObject       *object,
                                        gpointer         user_data);

gboolean
on_radiobtn_Fan_Silent_focus_in_event  (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_radiobtn_Fan_Normal_focus_in_event  (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_radiobtn_Fan_Cool_focus_in_event    (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_radiobtn_LED_On_focus_in_event      (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_radiobtn_Auto_focus_in_event        (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_radiobtn_Off_focus_in_event         (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_radiobtn_touchPad_enable_focus_in_event
                                        (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_radiobtn_touchPad_disable_focus_in_event
                                        (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_radiobtn_touchPad_only_focus_in_event
                                        (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

void
on_radiobtn_touchPad_disable_released  (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_radiobtn_Fan_Silent_focus_in_event  (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_radiobtn_Fan_Normal_focus_in_event  (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_radiobtn_Fan_Cool_focus_in_event    (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

void
on_radiobtn_Fan_Cool_released          (GtkButton       *button,
                                        gpointer         user_data);

void
on_btn_Calibrate_released              (GtkButton       *button,
                                        gpointer         user_data);

void
on_radiobtn_touchPad_disable_toggled   (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

gboolean
on_radiobtn_touchPad_disable_focus_in_event
                                        (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);
