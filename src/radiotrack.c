/*
 * Copyright (c) 2000 - 2002 Vladimir Popov <jumbo@narod.ru>.
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
 * $Id: radiotrack.c,v 1.33 2002/01/14 10:21:37 pva Exp $
 * BETA
 */

#include "ostypes.h"

#include "lm700x.h"
#include "radio_drv.h"

#define RT_CAPS		DRV_INFO_NEEDS_ROOT | DRV_INFO_NEEDS_SCAN | \
			DRV_INFO_MONOSTEREO | DRV_INFO_GETS_SIGNAL | \
			DRV_INFO_GETS_STEREO | DRV_INFO_VOL_SEPARATE | \
			DRV_INFO_VOLUME(10)

#define SF16FMI_CAPS	DRV_INFO_NEEDS_ROOT | DRV_INFO_NEEDS_SCAN | \
			DRV_INFO_VOL_SEPARATE | DRV_INFO_VOLUME(1)

#define RADIOTRACK	0
#define SF16_FMI	1
#define UNKNOWN		-1

int get_port_rt(u_int32_t);
int free_port_rt(void);
u_int32_t info_port_rt(void);
void set_freq_rt(u_int16_t);
void set_vol_rt(int);
void mono_rt(void);
int state_rt(void);

u_int32_t rt_ports[] = { 0x20c, 0x30c };
u_int32_t sfi_ports[] = { 0x284, 0x384 };

struct tuner_drv_t rt_drv = {
	"AIMS Lab Radiotrack", "rt", rt_ports, 2, RT_CAPS,
	get_port_rt, free_port_rt, info_port_rt, NULL,
	set_freq_rt, NULL, NULL, set_vol_rt, NULL, mono_rt, state_rt
};

struct tuner_drv_t sfi_drv = {
	"SoundForte RadioX SF16-FMI", "sfi", sfi_ports, 2, SF16FMI_CAPS,
	get_port_rt, free_port_rt, info_port_rt, NULL,
	set_freq_rt, NULL, NULL, set_vol_rt, NULL, NULL, state_rt
};

static u_int32_t radioport = 0;
static int tunertype = RADIOTRACK;
static int stereo = LM700X_STEREO;

/******************************************************************/

struct tuner_drv_t *
export_rt(void) {
	return &rt_drv;
}

struct tuner_drv_t *
export_sfi(void) {
	return &sfi_drv;
}

int
get_port_rt(u_int32_t port) {
	radioport = port;
	switch (port) {
	case 0x20c:
	case 0x30c:
		tunertype = RADIOTRACK;
		break;
	case 0x284:
	case 0x384:
		tunertype = SF16_FMI;
		break;
	default:
		tunertype = UNKNOWN;
		return -1;
	}
	return radio_get_ioperms(radioport, 2);
}

int
free_port_rt(void) {
	return radio_release_ioperms(radioport, 2);
}

u_int32_t
info_port_rt(void) {
	return radioport;
}

void
set_freq_rt(u_int16_t frequency) {
	u_int32_t reg = 0;
	int i;

	if (tunertype == UNKNOWN)
		return;

	reg  = lm700x_encode_freq(frequency, LM700X_REF_050);
	reg |= stereo | LM700X_REF_050 | LM700X_DIVIDER_FM;

	if (tunertype == SF16_FMI)
		OUTB(radioport, 0);

	for (i = 0; i < LM700X_REGISTER_LENGTH; i++)
		if (reg & (1 << i)) {
			OUTB(radioport, 0xd5);
			OUTB(radioport, 0xd7);
		} else {
			OUTB(radioport, 0xd1);
			OUTB(radioport, 0xd3);
		}

	if (tunertype == RADIOTRACK) {
		usleep(1000);
		OUTB(radioport, 0x10);
		usleep(50000);
		OUTB(radioport, 0xd8);
	} else if (tunertype == SF16_FMI) {
		OUTB(radioport, 0x08);
	}
}

void
set_vol_rt(int v) {
	u_int32_t waitdelay;

	if (tunertype == UNKNOWN)
		return;

	if (tunertype == SF16_FMI) {
		OUTB(radioport, v ? 0x08 : 0x00);
		return;
	}

	if (v > 10)
		v = 10;
	if (v < 0)
		v = 0;

	waitdelay = v * 100000;

	/* Mute the card */
	OUTB(radioport, 0x58);
	/* Make sure it's totally down */
	usleep(10 * 100000);
	OUTB(radioport, 0xd8);

	/* Increase volume */
	OUTB(radioport, 0x98);
	usleep(waitdelay);
	OUTB(radioport, 0xd8);
}

int
state_rt(void) {
	int res;

	if (tunertype == UNKNOWN || tunertype == SF16_FMI)
		return 0;

	OUTB(radioport, 0xf8);
	usleep(150000);
	res = (int)inb(radioport);

	if (res == 0xfd)
		return DRV_INFO_STEREO | DRV_INFO_SIGNAL;
	else
		if (res != 0xff)
			return DRV_INFO_SIGNAL;

	return 0;
}

void
mono_rt(void) {
	stereo = LM700X_MONO;
}
