#!/bin/sh

XORG=/etc/X11/xorg.conf

MINXOK=`grep Option ${XORG} | grep -c \"MinX\"`
MINYOK=`grep Option ${XORG} | grep -c \"MinY\"`
MAXXOK=`grep Option ${XORG} | grep -c \"MaxX\"`
MAXYOK=`grep Option ${XORG} | grep -c \"MaxY\"`

if [ $MINXOK -ge 1 ]
then
	ed ${XORG} << EOF
/"MinX"
d
i
	Option		"MinX"		"$1"
.
w
q
EOF

else
	ed ${XORG} << EOF
/Driver.*"evtouch"
/EndSection
-
a
	Option		"MinX"		"$1"
.
w
q
EOF

fi

if [ $MINYOK -ge 1 ]
then
	ed ${XORG} << EOF
/"MinY"
d
i
	Option		"MinY"		"$2"
.
w
q
EOF

else

	ed ${XORG} << EOF
/Driver.*"evtouch"
/EndSection
-
a
	Option		"MinY"		"$2"
.
w
q
EOF

fi

if [ $MAXXOK -ge 1 ]
then
	ed ${XORG} << EOF
/"MaxX"
d
i
	Option		"MaxX"		"$3"
.
w
q
EOF

else

	ed ${XORG} << EOF
/Driver.*"evtouch"
/EndSection
-
a
	Option		"MaxX"		"$3"
.
w
q
EOF

fi

if [ $MAXYOK -ge 1 ]
then
	ed ${XORG} << EOF
/"MaxY"
d
i
	Option		"MaxY"		"$4"
.
w
q
EOF

else

	ed ${XORG} << EOF
/Driver.*"evtouch"
/EndSection
-
a
	Option		"MaxY"		"$4"
.
w
q
EOF

fi

