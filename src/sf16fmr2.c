/*
 * Copyright (c) 2001, 2002 Vladimir Popov <jumbo@narod.ru>.
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
 * $Id: sf16fmr2.c,v 1.38 2002/01/18 10:55:51 pva Exp $
 * lowlevel driver io functions for fmio utility -- sf16fmr2 driver
 */

#include "ostypes.h"

#include "pt2254a.h"
#include "radio_drv.h"
#include "tea5757.h"

#define SF16FMR2_NOAMP	0
#define SF16FMR2_AMP	1

#define SF16FMR2_CAPS	DRV_INFO_NEEDS_ROOT  | DRV_INFO_HARDW_SRCH | \
			DRV_INFO_KNOWS_FREQ  | DRV_INFO_MONOSTEREO | \
			DRV_INFO_GETS_SIGNAL | DRV_INFO_GETS_STEREO | \
			DRV_INFO_MAXVOL_POLICY | DRV_INFO_VOL_SEPARATE

#define SF16FMR2_VOLU_STROBE_ON  (0 << 2)
#define SF16FMR2_VOLU_STROBE_OFF (1 << 2)
#define SF16FMR2_VOLU_CLOCK_ON   (1 << 5)
#define SF16FMR2_VOLU_CLOCK_OFF  (0 << 5)
#define SF16FMR2_VOLU_DATA_ON    (1 << 6)
#define SF16FMR2_VOLU_DATA_OFF   (0 << 6)

int get_port_sf16fmr2(u_int32_t);
int free_port_sf16fmr2(void);
u_int32_t info_port_sf16fmr2(void);
int find_card_sf16fmr2(void);
void set_frequency_sf16fmr2(u_int16_t);
u_int16_t get_frequency_sf16fmr2(void);
u_int16_t search_sf16fmr2(int, u_int16_t);
void set_vol_sf16fmr2(int);
int state_sf16fmr2(void);
void mono_sf16fmr2(void);

static void write_shift_register(u_int32_t);
static u_int32_t read_shift_register(void);
static void set_volume(int);
static void send_vol_bit(int);
static void mute_sf16fmr2(int);

static u_int32_t radioport = 0x384;
static int type = SF16FMR2_NOAMP;

static struct tea5757_t card = {
	TEA5757_SEARCH_END, 0, TEA5757_S030, TEA5757_STEREO,
	read_shift_register, write_shift_register
};

struct tuner_drv_t sf16fmr2_drv = {
	"SoundForte RadioLink SF16-FMR2",
	"sf2r", &radioport, 1, SF16FMR2_CAPS | DRV_INFO_VOLUME(15),
	get_port_sf16fmr2, free_port_sf16fmr2, info_port_sf16fmr2,
	find_card_sf16fmr2, set_frequency_sf16fmr2,
	get_frequency_sf16fmr2, search_sf16fmr2,
	set_vol_sf16fmr2, NULL, mono_sf16fmr2, state_sf16fmr2
};

/******************************************************************/

struct tuner_drv_t *
export_sf16fmr2(void) {
	return &sf16fmr2_drv;
}

int
get_port_sf16fmr2(u_int32_t port) {
	return radio_get_ioperms(radioport, 1) < 0 ? -1 : 0;
}

int
free_port_sf16fmr2(void) {
	return radio_release_ioperms(radioport, 1);
}

u_int32_t
info_port_sf16fmr2(void) {
	return radioport;
}

void
set_frequency_sf16fmr2(u_int16_t frequency) {
	card.frequency = frequency;
	card.search = TEA5757_SEARCH_END;
	tea5757_write_shift_register(&card);
	return;
}

u_int16_t
search_sf16fmr2(int dir, u_int16_t freq) {
	card.frequency = freq;
	card.search = dir ? TEA5757_SEARCH_UP : TEA5757_SEARCH_DOWN;
	return tea5757_search(&card);
}

void
set_vol_sf16fmr2(int v) {
	if (v > 15)
		v = 15;
	if (v < 0)
		v = 0;

	mute_sf16fmr2(v);

	if (type != SF16FMR2_NOAMP)
		set_volume(v);
}

int
state_sf16fmr2(void) {
	u_int32_t res = tea5757_read_shift_register(&card);
	int ret = 0;

	if (res & (1 << 26))
		ret |= DRV_INFO_SIGNAL;
	if (res & (1 << 25))
		ret |= DRV_INFO_STEREO;

	return ret;
}

void
mono_sf16fmr2(void) {
	card.stereo = TEA5757_MONO;
}

u_int16_t
get_frequency_sf16fmr2(void) {
	return tea5757_decode_frequency(tea5757_read_shift_register(&card));
}

static void
mute_sf16fmr2(int v) {
	OUTB(radioport, v ? 0x04 : 0x00);
}

static u_int32_t
read_shift_register(void) {
	int rb, res;
	int state;

	OUTB(radioport, 0x05);

	OUTB(radioport, 0x07);
	rb = inb(radioport);
	state = rb & 0x80 ? 0x04 : 0; /* Amplifier present/not present */
	type = rb & 0x80 ? SF16FMR2_AMP : SF16FMR2_NOAMP;
	state |= rb & 0x08 ? 0 : 0x02; /* Tuned/Not tuned */

	OUTB(radioport, 0x05);
	rb = inb(radioport);
	state |= rb & 0x08 ? 0 : 0x01; /* Mono/Stereo */
	res = rb & 0x01;

	/* Read the rest of the register */
	rb = 23;
	while (rb--) {
		res <<= 1;
		OUTB(radioport, 0x07);
		OUTB(radioport, 0x05);
		res |= inb(radioport) & 0x01;
	}

	return res | (state << 25);
}

static void
write_shift_register(u_int32_t data) {
	int c = 25;

	OUTB(radioport, 0x00);

	while (c--)
		if (data & (1 << c)) {
			OUTB(radioport, 0x01);
			OUTB(radioport, 0x03);
		} else {
			OUTB(radioport, 0x00);
			OUTB(radioport, 0x02);
		}

	OUTB(radioport, 0x00);
	OUTB(radioport, 0x04);
}

int
find_card_sf16fmr2(void) {
	u_int16_t cur_freq = 0ul;
	u_int16_t test_freq = 0ul;
	int err = -1;

	cur_freq = get_frequency_sf16fmr2();

	set_frequency_sf16fmr2(TEST_FREQ);
	test_freq = get_frequency_sf16fmr2();
	if (test_freq == TEST_FREQ) {
		set_frequency_sf16fmr2(cur_freq);
		err = 0;
	}

	return err;
}

void
set_volume(int v) {
	u_int32_t reg, vol;
	int i;

	vol = pt2254a_encode_volume(v, 15);
	reg = pt2254a_compose_register(vol, vol, USE_CHANNEL, USE_CHANNEL);

	OUTB(radioport, SF16FMR2_VOLU_STROBE_OFF);

	for (i = 0; i < PT2254A_REGISTER_LENGTH; i++)
		send_vol_bit(reg & (1 << i));

	/* Latch the data */
	OUTB(radioport, SF16FMR2_VOLU_STROBE_ON);
	OUTB(radioport, SF16FMR2_VOLU_STROBE_OFF);
	OUTB(radioport, 0x10 | SF16FMR2_VOLU_STROBE_OFF);
}

static void
send_vol_bit(int i) {
	unsigned int data;

	data  = SF16FMR2_VOLU_STROBE_OFF;
	data |= i ? SF16FMR2_VOLU_DATA_ON : SF16FMR2_VOLU_DATA_OFF;

	OUTB(radioport, data | SF16FMR2_VOLU_CLOCK_OFF);
	OUTB(radioport, data | SF16FMR2_VOLU_CLOCK_ON);
}
