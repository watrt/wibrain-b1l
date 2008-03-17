#!/bin/sh

modprobe wbts

FILENM=/proc/bus/input/devices
DEVICENM="TechDien TouchScreen"
ADDNUM=4

NEARBYNUM=`grep -n "${DEVICENM}" ${FILENM} | awk -F':' '{print $1}'`

HANDLERNUM=`expr ${NEARBYNUM} + ${ADDNUM}`

EVENTNUM=`grep -n Handlers ${FILENM} | grep ^${HANDLERNUM} | awk -F'event' '{printf $2}'`

ln -s /dev/input/event${EVENTNUM} /dev/input/touchscreen
