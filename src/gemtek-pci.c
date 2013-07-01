/*
 * Copyright (c) 2001 Vladimir Popov <jumbo@narod.ru>.
 * Copyright (C) 2001 Vladimir Shebordaev <vshebordaev@mail.ru>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * $Id: gemtek-pci.c,v 1.7 2002/01/14 10:21:34 pva Exp $
 * TEA5757: http://www.semiconductors.com/acrobat/datasheets/TEA5757_5759_3.pdf
 * lowlevel io functions for fmio utility -- Gemtek PCI driver
 */

#include "ostypes.h"

#include <errno.h>
#include <stdio.h>

#include "radio_drv.h"
#include "tea5757.h"

#ifndef PCI_VENDOR_ID_GEMTEK
#define PCI_VENDOR_ID_GEMTEK 0x5046
#endif

#ifndef PCI_DEVICE_ID_GEMTEK_PR103
#define PCI_DEVICE_ID_GEMTEK_PR103 0x1001
#endif

#define GTP_WREN_ON	(1 << 2)
#define GTP_WREN_OFF	(0 << 2)
#define GTP_DATA_ON	(1 << 1)
#define GTP_DATA_OFF	(0 << 1)
#define GTP_CLCK_ON	(1 << 0)
#define GTP_CLCK_OFF	(0 << 0)

#define GTP_CAPS	DRV_INFO_NEEDS_ROOT | DRV_INFO_HARDW_SRCH | \
			DRV_INFO_MONOSTEREO | DRV_INFO_GETS_SIGNAL | \
			DRV_INFO_GETS_STEREO | DRV_INFO_VOLUME(1)

int get_port_gtp(u_int32_t);
int free_port_gtp(void);
u_int32_t info_port_gtp(void);
int find_card_gtp(void);
void set_freq_gtp(u_int16_t);
u_int16_t search_gtp(int, u_int16_t);
void mute_gtp(int);
int state_gtp(void);
void mono_gtp(void);

struct tuner_drv_t gtp_drv = {
	"Gemtek PCI", "gtp", NULL, 0, GTP_CAPS,
	get_port_gtp, free_port_gtp, info_port_gtp, find_card_gtp,
	set_freq_gtp, NULL, search_gtp, mute_gtp, NULL,
	mono_gtp, state_gtp
};

struct tuner_drv_t mr_drv = {
	"Guillemot MaxiRadio FM2000", "mr", NULL, 0, GTP_CAPS,
	get_port_gtp, free_port_gtp, info_port_gtp, find_card_gtp,
	set_freq_gtp, NULL, search_gtp, mute_gtp, NULL,
	mono_gtp, state_gtp
};

static u_int32_t read_shift_register(void);
static void write_shift_register(u_int32_t data);
static void send_zero(void);
static void send_one(void);

static u_int32_t radioport = 0;
static struct tea5757_t card = {
	TEA5757_SEARCH_END, 0, TEA5757_S030, TEA5757_STEREO,
	read_shift_register, write_shift_register
};

/*********************************************************************/

struct tuner_drv_t *
export_gtp(void) {
	return &gtp_drv;
}

struct tuner_drv_t *
export_mr(void) {
	return &mr_drv;
}

int
get_port_gtp(u_int32_t port) {
	return radio_get_iopl() < 0 ? -1 : 0;
}

int
find_card_gtp(void) {
	struct pci_dev_t pd = {
		PCI_VENDOR_ID_GEMTEK, PCI_DEVICE_ID_GEMTEK_PR103,
		PCI_SUBSYS_ID_ANY, PCI_SUBSYS_ID_ANY, PCI_SUBCLASS_ANY,
		PCI_REVISION_ANY
	};

	radioport = pci_bus_locate(&pd);
	if (radioport == 0) {
		errno = ENXIO;
		return -1;
	}

	return 0;
}

int
free_port_gtp(void) {
	return radio_release_iopl();
}

u_int32_t
info_port_gtp(void) {
	return radioport;
}

/*
 * Set frequency and other stuff
 * Basically, this is just writing the 25-bit shift register
 */
void
set_freq_gtp(u_int16_t freq) {
	card.frequency = freq;
	card.search = TEA5757_SEARCH_END;
	tea5757_write_shift_register(&card);
	return;
}

u_int16_t
search_gtp(int dir, u_int16_t freq) {
	card.frequency = 0ul;
	card.search = dir ? TEA5757_SEARCH_UP : TEA5757_SEARCH_DOWN;
	tea5757_write_shift_register(&card);
	/* Gemtek PCI is incapable to read the shift register */
	return 0ul;
}

int
state_gtp(void) {
	int ret;

	OUTW(radioport, GTP_DATA_ON | GTP_WREN_OFF | GTP_CLCK_OFF);
	ret  = inw(radioport) & 8 ? DRV_INFO_STEREO : 0; /* STEREO */
	OUTW(radioport, GTP_DATA_ON | GTP_WREN_OFF | GTP_CLCK_ON);
	ret |= inw(radioport) & 8 ? DRV_INFO_SIGNAL : 0; /* SIGNAL */

	OUTW(radioport, 0x10);

	return ret;
}

void
mono_gtp(void) {
	card.stereo = TEA5757_MONO;
}

static void
send_one(void) {
	OUTW(radioport, GTP_WREN_ON | GTP_DATA_ON | GTP_CLCK_OFF);
	OUTW(radioport, GTP_WREN_ON | GTP_DATA_ON | GTP_CLCK_ON);
	OUTW(radioport, GTP_WREN_ON | GTP_DATA_ON | GTP_CLCK_OFF);
}

static void
send_zero(void) {
	OUTW(radioport, GTP_WREN_ON | GTP_DATA_OFF | GTP_CLCK_OFF);
	OUTW(radioport, GTP_WREN_ON | GTP_DATA_OFF | GTP_CLCK_ON);
	OUTW(radioport, GTP_WREN_ON | GTP_DATA_OFF | GTP_CLCK_OFF);
}

void
mute_gtp(int v) {
	/* The only way to unmute the card is to set frequency */
	if (v == 0)
		OUTW(radioport, 0x1f);
}

static void
write_shift_register(u_int32_t data) {
	int c = 25;

	OUTW(radioport, 0x06);

	while ( c-- )
		if (data & (1 << c))
			send_one();
		else
			send_zero();

	OUTW(radioport, 0x10);
}

static u_int32_t
read_shift_register(void) {
	return 0ul;
}
