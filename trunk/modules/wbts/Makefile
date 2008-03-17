#!/usr/bin/make -f
# -*- mode: makefile; coding: utf-8 -*-

# Makefile for building squashfs out-of-tree
CFLAGS  += -DCONFIG_WBTS=1
KDIR    ?= /lib/modules/`uname -r`/build
MODPATH ?= /lib/modules/`uname -r`/kernel/drivers/input/touchscreen

modules:
	make -C $(KDIR) SUBDIRS=`pwd`/linux-2.6 CONFIG_WBTS=m \
		$(DBG_CONFIG) CC="${CROSS_COMPILE}gcc" modules

install: modules
	install -d $(MODPATH)
	install -m 644 -c `pwd`/linux-2.6/wbts.ko $(MODPATH)
	/sbin/depmod -a

clean:
	find . \( -name '*.ko' -o -name '*.o' -o -name '.tmp_versions' -o -name '*~' -o -name '.*.cmd' \
		-o -name '*.mod.c' -o -name '*.tar.bz2' -o -name '*.rej' -o -name '*.orig' \)\
		-print | xargs rm -Rf
