/*
 * Copyright (c) 2001 Vladimir Popov <jumbo@narod.ru>.
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
 * $Id: gemtek-isa.c,v 1.3 2002/01/14 10:21:34 pva Exp $
 *
 * lowlevel io functions for fmio utility -- Gemtek ISA driver
 */

#include "ostypes.h"

#include "bu2614.h"
#include "radio_drv.h"

#define GTI_CAPS	DRV_INFO_NEEDS_ROOT | DRV_INFO_GETS_SIGNAL | \
			DRV_INFO_VOLUME(1) | DRV_INFO_VOL_SEPARATE

int get_port_gti(u_int32_t);
int free_port_gti(void);
u_int32_t info_port_gti(void);
int find_card_gti(void);
void set_freq_gti(u_int16_t);
void set_vol_gti(int);
int state_gti(void);

u_int32_t gti_ports[] = { 0x20c, 0x30c, 0x24c, 0x34c, 0x248 };
u_int32_t svg_ports[] = { 0x28c };

struct tuner_drv_t gti_drv = {
	"Gemtek ISA", "gti", gti_ports, 5, GTI_CAPS,
	get_port_gti, free_port_gti, info_port_gti, find_card_gti,
	set_freq_gti, NULL, NULL, set_vol_gti, NULL, NULL,
	state_gti
};

struct tuner_drv_t svg_drv = {
	"Sound Vision 16 Gold", "svg", svg_ports, 1, GTI_CAPS,
	get_port_gti, free_port_gti, info_port_gti, find_card_gti,
	set_freq_gti, NULL, NULL, set_vol_gti, NULL, NULL,
	state_gti
};

static struct bu2614_t card = {
	(1 << 2), (1 << 1), (1 << 0), 0
};

/*********************************************************************/

struct tuner_drv_t *
export_gti(void) {
	return &gti_drv;
}

struct tuner_drv_t *
export_svg(void) {
	return &svg_drv;
}

int
find_card_gti(void) {
	int i, a, lasta;

	lasta = inb(card.port);

	/* ISA reads back 0xff when no card here */
	if (lasta == 0xff)
		return -1;

	/*
	 * Gemtek read as 0x3F, 0x1F, 0x37 or 0x17
	 * depending on signal state
	 */
	if ((lasta & 0x17) != 0x17)
		return -1;

	for (i = 1; i < 3; i++) {
		a = inb(card.port + i);
		/* Gemtek read back identical for 4 registers */
		if (lasta != a)
			return -1;
	}

	return 0;
}

int
get_port_gti(u_int32_t port) {
	card.port = port;
	return radio_get_ioperms(card.port, 4);
}

int
free_port_gti(void) {
	return radio_release_ioperms(card.port, 4);
}

u_int32_t
info_port_gti(void) {
	return card.port;
}

void
set_vol_gti(int v) {
	OUTB(card.port, v ? 0x20 : 0x10);
}

void
set_freq_gti(u_int16_t freq) {
	freq  = bu2614_conv_freq(freq);
	freq |= BU2614_BAND_FM | BU2614_REF_FREQ_6P25_KHZ | BU2614_GT_ON;
	bu2614_write(&card, freq);
}

int
state_gti(void) {
	usleep(50);
	return inb(card.port) & 8 ? 0 : DRV_INFO_SIGNAL;
}
