/*
 * Copyright (c) 2001 Vladimir Popov <jumbo@narod.ru>.
 * Copyright (c) 1999 Andy Ramensky, andy@clhs.kiev.ua
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
 * $Id: sf16fmd2.c,v 1.16 2002/01/14 10:21:38 pva Exp $
 * ALPHA
 * lowlevel driver io functions for fmio utility -- sf16fmd2 driver
 */

#include "ostypes.h"

#include "radio_drv.h"

#define SF16FMD2_CAPS		DRV_INFO_NEEDS_ROOT | DRV_INFO_MONOSTEREO | \
				DRV_INFO_VOLUME(1) | DRV_INFO_VOL_SEPARATE

int get_port_sf16fmd2(u_int32_t);
int free_port_sf16fmd2(void);
u_int32_t info_port_sf16fmd2(void);
void set_freq_sf16fmd2(u_int16_t);
void mute_sf16fmd2(int);
void mono_sf16fmd2(void);

u_int32_t sf2d_ports[] = { 0x284, 0x384 };

struct tuner_drv_t sf2d_drv = {
	"SoundForte Legacy 128 SF16-FMD2",
	"sf2d", sf2d_ports, 2, SF16FMD2_CAPS,
	get_port_sf16fmd2, free_port_sf16fmd2, info_port_sf16fmd2,
	NULL, set_freq_sf16fmd2, NULL, NULL,
	mute_sf16fmd2, NULL, mono_sf16fmd2, NULL
};

static int stereo = 1;
static u_int32_t radioport = 0;

static void inbits(int);
static void send_zero(int);
static void send_one(int);

/******************************************************************/

struct tuner_drv_t *
export_sf16fmd2(void) {
	return &sf2d_drv;
}

int
get_port_sf16fmd2(u_int32_t port) {
	radioport = port;
	return radio_get_ioperms(radioport, 1);
}

int
free_port_sf16fmd2(void) {
	return radio_release_ioperms(radioport, 1);
}

u_int32_t
info_port_sf16fmd2(void) {
	return radioport;
}

void
set_freq_sf16fmd2(u_int16_t frequency) {
	int c = 0x0f;
	u_int16_t freq = (u_int16_t)
		((float)frequency*0.7985714+871.28571);

	/* Search end - station found */
	send_zero(3);

	/* Search down */
	send_zero(3);

	/* Stereo/Forced Mono */
	if (stereo)
		send_zero(3);
	else
		send_one(3);

	/* FM band */
	send_zero(3);
	send_zero(3);

	/* Band switch */
	send_zero(3);
	send_zero(3);

	/* Locking field strength during search > 30 mkV */
	send_one(3);
	send_zero(3);

	/* Dummy */
	send_zero(3);

	while (c--)
		if (freq & (1 << c))
			send_one(2);
		else
			send_zero(2);

	usleep(AFC_DELAY);
	return;
}

void
mute_sf16fmd2(int v) {
	OUTB(radioport, v ? 0x04 : 0x00);
	inbits(4);
}

static void
inbits(int c) {
	while ( c-- )
		inb(radioport);
}

static void
send_one(int c) {
	OUTB(radioport, 0x01);
	inbits(c);
	OUTB(radioport, 0x03);
	inbits(c);
	OUTB(radioport, 0x01);
	inbits(c);
}

static void
send_zero(int c) {
	OUTB(radioport, 0x00);
	inbits(c);
	OUTB(radioport, 0x02);
	inbits(c);
	OUTB(radioport, 0x00);
	inbits(c);
}

void
mono_sf16fmd2(void) {
	stereo = 0;
}
