/*
 * Copyright (c) 2002 Vladimir Popov <jumbo@narod.ru>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * $Id$
 */

#ifndef RADIO_DRV_H__
#define RADIO_DRV_H__

#ifndef __DOS__

#include <sys/param.h>
#include <sys/ioctl.h>

#endif

#ifdef linux
#include <sys/io.h>
#include <linux/videodev.h>
#elif defined __FreeBSD__
#include <machine/ioctl_bt848.h>
#include <fcntl.h>
#include <machine/cpufunc.h>
#elif defined __QNXNTO__
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <strings.h>
#elif defined __OpenBSD__ || defined __NetBSD__
#if defined __NetBSD__ 
#include <dev/ic/bt8xx.h>
#elif defined __OpenBSD__
#if OpenBSD < 200105
#include <machine/ioctl_bt848.h>
#else
#include <dev/ic/bt8xx.h>
#endif /* OpenBSD < 200105 */
#endif /* __NetBSD__ */
#include <machine/pio.h>
#include <machine/sysarch.h>
#endif /* linux || __FreeBSD__ || __QNXNTO__ || __OpenBSD__ || __NetBSD__ */

#ifndef __DOS__

#include <err.h>
#include <unistd.h>

#else

#include <stdlib.h>
#include <conio.h>
#include <io.h>
#include <i86.h>

#endif /* #ifdef __DOS__ */

#if defined __FreeBSD__ || defined __OpenBSD__ || defined __NetBSD__
#define BSDBKTR
#endif /* __FreeBSD__ || __OpenBSD__ || __NetBSD__ */
#if defined __OpenBSD__ || defined __NetBSD__
#if (OpenBSD > 200201) || ((NetBSD > 199904) && (__NetBSD_Version__ > 105270000))
#define BSDRADIO
#endif /* (OpenBSD > 200201) || (NetBSD > 199904 && __NetBSD_Version__ > 105270000) */
#endif /* __OpenBSD__ || __NetBSD__ */

#if defined BSDBKTR || defined linux
#define USE_BKTR
#endif

#define AFC_DELAY	300000

#define SEARCH_PROBE	15
#define SEARCH_LENGTH	19

#define TEST_FREQ	10630

#define SYMLINK_DEPTH	10

#ifdef linux
#define OUTL(a, b)	outl(b, a)
#define OUTW(a, b)	outw(b, a)
#define OUTB(a, b)	outb(b, a)
#elif defined __QNXNTO__
#define OUTL(a, b)	out32(a, b)
#define OUTW(a, b)	out16(a, b)
#define OUTB(a, b)	out8(a, b)
#define inl(a)		in32(a)
#define inw(a)		in16(a)
#define inb(a)		in8(a)
#elif defined(__DOS__)
#define OUTL(a, b)	outpd(a, b)
#define OUTW(a, b)	outpw(a, b)
#define OUTB(a, b)	outp(a, b)
#define inl(a)		inpd(a)
#define inw(a)		inpw(a)
#define inb(a)		inp(a)
#else
#define OUTL(a, b)	outl(a, b)
#define OUTW(a, b)	outw(a, b)
#define OUTB(a, b)	outb(a, b)
#endif /* linux */

struct tuner_drv_t {
	char *name;	/* Full card name */
	char *drv;	/* Shord driver name */

	u_int32_t *ports;	/* Ports used by card */
	int portsno;		/* Number of ports */

	u_int32_t caps; /* Driver capabilities */
#define DRV_INFO_MAX_VOLUME	0xFF		/* Maximal volume level */
#define   DRV_INFO_VOLUME(x)	((x) & DRV_INFO_MAX_VOLUME)
#define DRV_INFO_NEEDS_ROOT	(1 << 8)	/* Requires root privileges */
#define DRV_INFO_NEEDS_SCAN	(1 << 9)	/* Uses scan for detection */
#define DRV_INFO_HARDW_SRCH	(1 << 10)	/* Hardware search */
#define DRV_INFO_KNOWS_FREQ	(1 << 11)	/* Knows current frequency */
#define DRV_INFO_KNOWS_VOLU	(1 << 12)	/* Knows current volume */
#define DRV_INFO_MONOSTEREO	(1 << 13)	/* Can switch mono/stereo */
#define DRV_INFO_GETS_SIGNAL	(1 << 14)	/* Knows current signal */
#define DRV_INFO_GETS_STEREO	(1 << 15)	/* Knows current stereo */
#define DRV_INFO_MAXVOL_POLICY	(1 << 16)	/* Volume management policy
						   - maximal value first */
#define DRV_INFO_VOL_SEPARATE	(1 << 17)	/* Volume may be managed
						   separately from frequency */

	int (*get_port)(u_int32_t);	/* Get port access */
	int (*free_port)(void);		/* Release port */
	u_int32_t (*info_port)(void);	/* Report port */
	int (*find_card)(void);		/* Find the card */

	void (*set_freq)(u_int16_t);	/* Set frequency */
	u_int16_t (*get_freq)(void);	/* Get frequency */

	u_int16_t (*search)(int, u_int16_t);	/* Hardware search up/down */

	void (*set_volu)(int);		/* Set volume */
	int (*get_volu)(void);		/* Get volume */

	void (*set_mono)(void);		/* Set output to mono */

	int (*get_state)(void);		/* Get signal/stereo status */
#define DRV_INFO_SIGNAL	(1 << 0)
#define DRV_INFO_STEREO	(1 << 1)
};

typedef struct tuner_drv_t *(*EXPORT_FUNC)(void);

struct pci_dev_t {
	u_int16_t vid; /* vendor id */
	u_int16_t did; /* device id */
	u_int16_t subvid; /* subsystem vendor id */
	u_int16_t subdid; /* subsystem id */
#define PCI_SUBSYS_ID_ANY		0xffff
	u_int8_t subclass;
#define PCI_SUBCLASS_ANY		0xff
	u_int8_t rev; /* revision id */
#define PCI_REVISION_ANY		0xff
};

#define PCI_SUBCLASS_MULTIMEDIA_AUDIO	0x01

int radio_get_iopl(void);
int radio_release_iopl(void);
int radio_get_ioperms(u_int32_t, int);
int radio_release_ioperms(u_int32_t, int);

int radio_device_get(const char *, const char *, int);
int radio_device_release(int, const char *);

u_int16_t pci_bus_locate(struct pci_dev_t *);

void print_w(const char *, ...);
void print_wx(const char *, ...);

#endif /* RADIO_DRV_H__ */
