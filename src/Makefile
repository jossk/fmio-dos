#
# Copyright (c) 2000 - 2002 Vladimir Popov. All rights reserved.
#
# $Id: Makefile,v 1.56 2002/01/18 10:55:47 pva Exp $
#
# Makefile for fmio -- small utility to manipulate FM radio card
#
CC?=cc
CFLAGS?=-O2 -Wall

# don't use mixer stuff
CFLAGS+= -DNOMIXER

CURDIR!= pwd

LIBRADIODIR= $(CURDIR)
INCRADIODIR= $(CURDIR)

CFLAGS+= -I$(INCRADIODIR)

HDRS= bu2614.h lm700x.h pt2254a.h radio.h radio_drv.h tc921x.h tea5757.h
ALLHDRS= $(HDRS) export.h mixer.h ostypes.h pci.h
OBJS= access.o bu2614.o lm700x.o mixer.o pci.o pt2254a.o radio.o tc921x.o \
	tea5757.o
DRVS= aztech.o bktr.o bmc-hma.o bsdradio.o ecoradio.o \
	gemtek-isa.o gemtek-pci.o radiotrack.o radiotrackII.o \
	sf16fmd2.o sf16fmr.o sf16fmr2.o sf64pce2.o sf64pcr.o sf256pcpr.o \
	sf256pcsr.o spase.o terratec-isa.o trust.o xtreme.o zoltrix.o

FMIOOBJ= fmio.o
FMIO= fmio
MANPAGE= fmio.1
CATPAGE= fmio.0

REMOVABLE= $(FMIOOBJ) $(FMIO) $(OBJS) $(DRVS) libradio.a *core

PREFIX?= /usr/local
LIBDIR?= $(PREFIX)/lib
INCDIR?= $(PREFIX)/include
BINDIR?= $(PREFIX)/bin
MANDIR?= $(PREFIX)/man/cat1

BINOWN?= root
BINGRP?= bin
BINMODE?= 4555

MANOWN?= root
MANGRP?= wheel
MANMODE?= 444

LIBOWN?= root
LIBGRP?= wheel
LIBMODE?= 444

INSTALL_PROGRAM_DIR?= install -d -o $(BINOWN) -g $(BINGRP)
INSTALL_MAN_DIR?= install -d -o $(MANOWN) -g $(MANGRP)
INSTALL_PROGRAM?= install -c -s -o $(BINOWN) -g $(BINGRP) -m $(BINMODE)
INSTALL_MAN?= install -c -o $(MANOWN) -g $(MANGRP) -m $(MANMODE)

INSTALL_LIB_DIR?= install -d -o $(LIBOWN) -g $(LIBGRP)
INSTALL_LIB_FILE?= install -c -o $(LIBOWN) -g $(LIBGRP) -m $(LIBMODE)

all: lib fmio man

lib: libradio.a

fmio: libradio.a $(FMIOOBJ)
	$(CC) -o $@ $(FMIOOBJ) -L$(LIBRADIODIR) -lradio

man: $(CATPAGE)

install: lib fmio man
	$(INSTALL_PROGRAM_DIR) $(BINDIR)
	$(INSTALL_MAN_DIR) $(MANDIR)
	$(INSTALL_PROGRAM) fmio $(BINDIR)/fmio
	$(INSTALL_MAN) fmio.0 $(MANDIR)/fmio.0

deinstall:
	rm -f $(PREFIX)/bin/fmio
	rm -f $(PREFIX)/man/cat1/fmio.0

install_lib: lib
	$(INSTALL_LIB_DIR) $(LIBDIR)
	$(INSTALL_LIB_DIR) $(INCDIR)
	$(INSTALL_LIB_FILE) libradio.a $(LIBDIR)/libradio.a
	$(INSTALL_LIB_FILE) $(HDRS) $(INCDIR)

deinstall_lib:
	rm -f $(LIBDIR)/libradio.a
	@cd $(INCDIR) && rm -f $(HDRS)

clean:
	rm -f $(REMOVABLE)

distclean:
	rm -f $(REMOVABLE) $(CATPAGE)

libradio.a: $(ALLHDRS) $(OBJS) $(DRVS)
	rm -f $@
	ar cru $@ $(OBJS) $(DRVS)
	ranlib $@

fmio.0: $(MANPAGE)
	@echo "groff -Tascii -mandoc $(MANPAGE) > $@"
	@groff -Tascii -mandoc $(MANPAGE) > $@ || rm -f $@

.SUFFIXES: .c.o .1.0

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<
