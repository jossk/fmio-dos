/*
 * Copyright (c) 2000, 2002 Vladimir Popov <jumbo@narod.ru>.
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
 * ALPHA
 * zoltrix driver
 */

#include "ostypes.h"

#include "radio_drv.h"

#define LWRITE(a)	usleep(0); OUTB(radioport, a)

#define ZOLTRIX_CAPS		DRV_INFO_NEEDS_ROOT | DRV_INFO_NEEDS_SCAN | \
				DRV_INFO_MONOSTEREO | DRV_INFO_GETS_SIGNAL | \
				DRV_INFO_GETS_STEREO | DRV_INFO_VOLUME(16) | \
				DRV_INFO_MAXVOL_POLICY

int get_port_zoltrix(u_int32_t);
int free_port_zoltrix(void);
u_int32_t info_port_zoltrix(void);
void set_freq_zoltrix(u_int16_t);
void set_vol_zoltrix(int);
int state_zoltrix(void);
void mono_zoltrix(void);

u_int32_t zoltrix_ports[] = { 0x20c, 0x30c };

struct tuner_drv_t zx_drv = {
	"Zoltrix RadioPlus", "zx", zoltrix_ports, 2, ZOLTRIX_CAPS,
	get_port_zoltrix, free_port_zoltrix, info_port_zoltrix, NULL,
	set_freq_zoltrix, NULL, NULL, set_vol_zoltrix, NULL, mono_zoltrix,
	state_zoltrix
};

static int stereo = 0; /* Use stereo by default */
static int vol = 0;
static u_int32_t radioport = 0;

/******************************************************************/

struct tuner_drv_t *
export_zx(void) {
	return &zx_drv;
}

int
get_port_zoltrix(u_int32_t port) {
	radioport = port;
	return radio_get_ioperms(radioport, 4);
}

int
free_port_zoltrix(void) {
	return radio_release_ioperms(radioport, 4);
}

u_int32_t
info_port_zoltrix(void) {
	return radioport;
}

void
set_freq_zoltrix(u_int16_t frequency) {
	/* tunes the radio to the desired frequency */
	unsigned long long bitmask, f;
	int i;
	float freq = frequency/100;

	f = (unsigned long long)(((float)(freq-88.0))*200.0)+0x4d1c;
	i = 45;
	bitmask = 0xc480402c10080000ull;
	bitmask = (bitmask^((f&0xff)<<47)^((f&0xff00)<<30)^(stereo<<31));

	LWRITE(0x0);
	LWRITE(0x0);
	inb(radioport+3);

	LWRITE(0x40);
	LWRITE(0xc0);
	while (i--) {
		if ((bitmask & 0x8000000000000000ull) != 0) {
			LWRITE(0x80);
			LWRITE(0x00);
			LWRITE(0x80);
		} else {
			LWRITE(0xc0);
			LWRITE(0x40);
			LWRITE(0xc0);
		}
		bitmask *= 2;
	}

	/* Termination sequence */
	LWRITE(0x80);
	LWRITE(0xc0);
	LWRITE(0x40);
	usleep(20000);
	if (vol) { LWRITE(vol); }
	usleep(10000);
	inb(radioport+2);

	return;
}

void
set_vol_zoltrix(int v) {

	if (v > 16)
		v = 16;
	if (v < 0)
		v = 0;

	vol = v;

	OUTB(radioport, vol);
	usleep(10000);
	OUTB(radioport, vol);
	inb(vol == 0 ? radioport + 3 : radioport + 2);
}

int
state_zoltrix(void) {
	int a, b;

	OUTB(radioport, 0);
	OUTB(radioport, vol);
	usleep(10000);

	a = inb(radioport);
	usleep(1000);
	b = inb(radioport);
	
	if (a == b) {
		switch (a) {
			case 0xcf: return (DRV_INFO_SIGNAL | DRV_INFO_STEREO);
			case 0xdf: return DRV_INFO_STEREO;
			case 0xef: return DRV_INFO_SIGNAL;
			default: return 0;
		}
	}

	return 0;
}

void
mono_zoltrix(void) {
	stereo = 1;
}
