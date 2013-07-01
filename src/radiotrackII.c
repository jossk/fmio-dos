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
 * $Id: rtii.c,v 1.27 2002/01/14 10:21:37 pva Exp $
 * lowlevel driver io functions for fmio utility -- rtii driver
 */

#include "ostypes.h"

#include "radio_drv.h"
#include "tea5757.h"

#define RTII_CAPS	DRV_INFO_NEEDS_ROOT  | DRV_INFO_HARDW_SRCH | \
			DRV_INFO_KNOWS_FREQ  | DRV_INFO_MONOSTEREO | \
			DRV_INFO_GETS_SIGNAL | DRV_INFO_GETS_STEREO | \
			DRV_INFO_VOLUME(1) | DRV_INFO_VOL_SEPARATE

int get_port_rtii(u_int32_t);
int free_port_rtii(void);
u_int32_t info_port_rtii(void);
void set_freq_rtii(u_int16_t);
u_int16_t get_freq_rtii(void);
u_int16_t search_rtii(int, u_int16_t);
void mute_rtii(int);
int state_rtii(void);
void mono_rtii(void);

u_int32_t rtii_ports[] = { 0x20c, 0x30c };

struct tuner_drv_t rtii_drv = {
	"AIMS Lab Radiotrack II", "rtii", rtii_ports, 2, RTII_CAPS,
	get_port_rtii, free_port_rtii, info_port_rtii, NULL,
	set_freq_rtii, get_freq_rtii, search_rtii,
	mute_rtii, NULL, mono_rtii, state_rtii
};

static void send_zero(void);
static void send_one(void);
static void write_shift_register(u_int32_t);
static u_int32_t read_shift_register(void);

static struct tea5757_t card = {
	TEA5757_SEARCH_END, 0, TEA5757_S030, TEA5757_STEREO,
	read_shift_register, write_shift_register
};
static u_int32_t radioport = 0;

/******************************************************************/

struct tuner_drv_t *
export_rtii(void) {
	return &rtii_drv;
}

int
get_port_rtii(u_int32_t port) {
	radioport = port;
	return radio_get_ioperms(radioport, 1);
}

int
free_port_rtii(void) {
	return radio_release_ioperms(radioport, 1);
}

u_int32_t
info_port_rtii(void) {
	return radioport;
}

void
set_freq_rtii(u_int16_t frequency) {
	card.frequency = frequency;
	card.search = TEA5757_SEARCH_END;
	tea5757_write_shift_register(&card);
}

u_int16_t
get_freq_rtii(void) {
	return tea5757_decode_frequency(tea5757_read_shift_register(&card));
}

void
mute_rtii(int v) {
	OUTB(radioport, v == 0 ? 0x01 : 0x00);
}

int
state_rtii(void) {
	int res = inb(radioport);

	if (res == 0xfd)
		return (DRV_INFO_SIGNAL | DRV_INFO_STEREO);
	else
		if (res != 0xff)
			return (DRV_INFO_SIGNAL);

	return 0;
}

u_int16_t
search_rtii(int dir, u_int16_t freq) {
	card.frequency = freq;
	card.search = dir ? TEA5757_SEARCH_UP : TEA5757_SEARCH_DOWN;
	return tea5757_search(&card);
}

void
mono_rtii(void) {
	card.stereo = TEA5757_MONO;
}

static void
send_zero(void) {
	OUTB(radioport, 0x01);
	OUTB(radioport, 0x03);
	OUTB(radioport, 0x01);
}

static void
send_one(void) {
	OUTB(radioport, 0x05);
	OUTB(radioport, 0x07);
	OUTB(radioport, 0x05);
}

static void
write_shift_register(u_int32_t data) {
	int c = 25;

	OUTB(radioport, 0xc8);
	OUTB(radioport, 0xc9);
	OUTB(radioport, 0xc9);

	while (c--)
		if (data & (1 << c))
			send_one();
		else
			send_zero();

	OUTB(radioport, 0xc8);
}

static u_int32_t
read_shift_register(void) {
	u_int32_t reg = 0;
	int c = 25;
	int rb;
	
	OUTB(radioport, 0x06);

	while (c--) {
		OUTB(radioport, 0x04);
		OUTB(radioport, 0x06);
		rb = inb(radioport);
		reg |= rb & 0x04 ? 1 : 0;
		reg <<= 1;
	}

	reg >>= 1;

	return reg;
}
