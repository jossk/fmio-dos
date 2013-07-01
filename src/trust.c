/*
 * Copyright (c) 2002 Gunther Mayer
 * Copyright (c) 2001, 2002 Vladimir Popov <jumbo@narod.ru>.
 * Based on code by:
 * Copyright (c) Eric Lammerts, Quay Ly, Donald Song, Jason Lewis, 
 *		Scott McGrath, William McGrath
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

/* Trust Radio Card driver for fmio:
   FCC-ID: I38-MMFM601 (I38 is AZTECH)
   Hardware:
	tsa6060t, tea5712t, tda7318d

   Port: 0x350 fixed (no jumpers)
	WRITE:
	 Bit 0: SDA
	 Bit 1: SCL
	 Bit 2: set mono/stereo
	 Bit 3: set mute/unmute
	READ:A
	 Bit 0: 1=mono 0=stereo
	 Bit 1-7: const. 1

	Does not have a "tuned" indicator?

   Code based on linux-2.4.18/drivers/media/radio/radio-trust.c
 */

#include "ostypes.h"

#include "radio_drv.h"

#include <stdarg.h>

/* i2c addresses */
#define TDA7318_ADDR 0x88
#define TSA6060T_ADDR 0xc4

#define TRUST_CAPS		DRV_INFO_VOLUME(63) | DRV_INFO_MONOSTEREO | \
				DRV_INFO_GETS_STEREO | DRV_INFO_NEEDS_ROOT | \
				DRV_INFO_NEEDS_SCAN | DRV_INFO_MAXVOL_POLICY | \
				DRV_INFO_VOL_SEPARATE

int free_port_trust(void);
int get_port_trust(u_int32_t);
void tr_setvol(int);
void set_freq_trust(u_int16_t);
void mono_trust(void);
u_int32_t info_port_trust(void);
int state_trust(void);

u_int32_t tr_port = 0x350;

struct tuner_drv_t tr_drv = {
	"Trust FM Radio", "tr", &tr_port, 1, TRUST_CAPS,
	get_port_trust, free_port_trust, info_port_trust, NULL,
	set_freq_trust, NULL, NULL, tr_setvol, NULL, mono_trust,
	state_trust
};

static void tr_setmute(int);
static void tr_setbass(int);
static void tr_settreble(int);
static void tsa6060_encode_freq(u_int32_t, int *);
static void write_i2c(int, ...);

static int ioval = 0xf;
static int curvol;
static int curbass;
static int curtreble;
static int curstereo;
static int curmute;

struct tuner_drv_t *
export_tr(void) {
	return &tr_drv;
}

int
free_port_trust(void) {
	return radio_release_ioperms(tr_port, 2);
}

#define TR_DELAY do { inb(tr_port); inb(tr_port); inb(tr_port); } while(0)
#define TR_SET_SCL OUTB(tr_port, ioval |= 2)
#define TR_CLR_SCL OUTB(tr_port, ioval &= 0xfd)
#define TR_SET_SDA OUTB(tr_port, ioval |= 1)
#define TR_CLR_SDA OUTB(tr_port, ioval &= 0xfe)
static void
write_i2c(int n, ...) {
	unsigned char val, mask;
	va_list args;

	va_start(args, n);

	/* start condition */
	TR_SET_SDA;
	TR_SET_SCL;
	TR_DELAY;
	TR_CLR_SDA;
	TR_CLR_SCL;
	TR_DELAY;

	for(; n; n--) {
		val = va_arg(args, unsigned);
		for (mask = 0x80; mask; mask >>= 1) {
			if (val & mask)
				TR_SET_SDA;
			else
				TR_CLR_SDA;
			TR_SET_SCL;
			TR_DELAY;
			TR_CLR_SCL;
			TR_DELAY;
		}
		/* acknowledge bit */
		TR_SET_SDA;
		TR_SET_SCL;
		TR_DELAY;
		TR_CLR_SCL;
		TR_DELAY;
	}

	/* stop condition */
	TR_CLR_SDA;
	TR_DELAY;
	TR_SET_SCL;
	TR_DELAY;
	TR_SET_SDA;
	TR_DELAY;

	va_end(args);
}

/* tda7318 does 0db ... -78.25db */
void
tr_setvol(int vol) {
	if (vol > 63) vol = 63;
	if (vol < 0) vol = 0;
	if (vol == 0)
		tr_setmute(1);
	else
		tr_setmute(0);
	curvol = 63 - vol ;
	write_i2c(2, TDA7318_ADDR, curvol & 0x3f);
}

static int basstreble2chip[15] = {
	0, 1, 2, 3, 4, 5, 6, 7, 14, 13, 12, 11, 10, 9, 8
};

static void
tr_setbass(int bass) {
	curbass = bass / 4370;
	write_i2c(2, TDA7318_ADDR, 0x60 | basstreble2chip[curbass]);
}

static void
tr_settreble(int treble) {
	curtreble = treble / 4370; 
	write_i2c(2, TDA7318_ADDR, 0x70 | basstreble2chip[curtreble]);
}

static void
tr_setstereo(int stereo) {
	curstereo = !!stereo;
	ioval = (ioval & 0xfb) | (!curstereo << 2);
	OUTB(tr_port, ioval);
}

static void
tr_setmute(int mute) {
	curmute = !!mute;
	ioval = (ioval & 0xf7) | (curmute << 3);
	OUTB(tr_port, ioval);
}

int
state_trust(void) {	
	return inb(tr_port) & 1 ? 0 : DRV_INFO_STEREO;
}
	
int
get_port_trust(u_int32_t port) {
	if (radio_get_ioperms(tr_port, 2) < 0)
		return -1;

	write_i2c(2, TDA7318_ADDR, 0x80);       /* speaker att. LF = 0 dB */
	write_i2c(2, TDA7318_ADDR, 0xa0);       /* speaker att. RF = 0 dB */
	write_i2c(2, TDA7318_ADDR, 0xc0);       /* speaker att. LR = 0 dB */
	write_i2c(2, TDA7318_ADDR, 0xe0);       /* speaker att. RR = 0 dB */
	write_i2c(2, TDA7318_ADDR, 0x40);       /* stereo 1 input, gain = 18.75 dB */

	/* tr_setvol(63); */
	tr_setbass(0x8000);
	tr_settreble(0x8000);
	tr_setstereo(1);
	tr_setmute(0);
	return 0;
}

static void
tsa6060_encode_freq(u_int32_t f, int *tsa6060_data) {
	f += 1070; /* add 10.7 MHz IF intermediate frequency */
	tsa6060_data[0] = 1 | (f << 1);  /* Set CP */
	tsa6060_data[1] = f >> 7;
	/* Set 10kHz stepsize, FM Band */
	tsa6060_data[2] = 0x60 | ((f >> 15) & 3);
	tsa6060_data[3] = 0;
}

/* frequency is given in 10kHz units  */
void
set_freq_trust(u_int16_t frequency) {
	int tsa6060_data[4];

	tsa6060_encode_freq(frequency, tsa6060_data);

	write_i2c(5, TSA6060T_ADDR, tsa6060_data[0], 
			tsa6060_data[1],tsa6060_data[2],tsa6060_data[3]);
}

u_int32_t
info_port_trust(void) {
	return tr_port;
}

void
mono_trust(void) {
	tr_setstereo(0); /* set mono */
}
