#!/usr/bin/make -f
# -*- mode: makefile; coding: utf-8 -*-

# Makefile for building squashfs out-of-tree
# CFLAGS  += -DCONFIG_WBTS=1
KDIR    ?= /lib/modules/`uname -r`/build
# MODPATH ?= /lib/modules/`uname -r`/kernel/drivers/input/touchscreen

modules:

install: modules
clean:
	find . \( -name '*.ko' -o -name '*.o' -o -name '.tmp_versions' -o -name '*~' -o -name '.*.cmd' \
		-o -name '*.mod.c' -o -name '*.tar.bz2' -o -name '*.rej' -o -name '*.orig' \)\
		-print | xargs rm -Rf
