#!/bin/sh

CONFIGFILE=/etc/mfmgr.conf

if [ -e ${CONFIGFILE} ]
then
	FANMODE=`grep FanSpeed ${CONFIGFILE} | awk -F '=' '{print $2}'`
	KEYMODE=`grep KeypadLED ${CONFIGFILE} | awk -F '=' '{print $2}'`
	TPADMODE=`grep TouchPad ${CONFIGFILE} | awk -F '=' '{print $2}'` 

	/usr/local/bin/mfmgr ${FANMODE} ${KEYMODE} ${TPADMODE}
else
	echo "error : ${CONFIGFILE} not found"
fi
