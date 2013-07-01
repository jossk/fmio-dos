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
 * $Id: aztech.c,v 1.40 2002/01/14 10:21:32 pva Exp $
 * lowlevel io functions for fmio utility -- aztech driver
 */

#include "ostypes.h"

#include "lm700x.h"
#include "radio_drv.h"

#define AZTECH_CAPS	DRV_INFO_NEEDS_ROOT | DRV_INFO_NEEDS_SCAN | \
			DRV_INFO_MONOSTEREO | DRV_INFO_GETS_SIGNAL | \
			DRV_INFO_GETS_STEREO | DRV_INFO_VOLUME(3) | \
			DRV_INFO_MAXVOL_POLICY

#define AZTECH_STEREO	(1 << 0)
#define AZTECH_SIGNAL	(1 << 0)

int get_port_aztech(u_int32_t);
int free_port_aztech(void);
u_int32_t info_port_aztech(void);
void set_freq_aztech(u_int16_t);
int state_aztech(void);
void mono_aztech(void);
void set_vol_aztech(int);

u_int32_t az_ports[] = {0x350, 0x358};

struct tuner_drv_t aztech_drv = {
	"Aztech/PackardBell", "az", az_ports, 2, AZTECH_CAPS, 
	get_port_aztech, free_port_aztech, info_port_aztech, NULL,
	set_freq_aztech, NULL, NULL, set_vol_aztech, NULL,
	mono_aztech, state_aztech
};

static void send_zero(void);
static void send_one(void);

static int stereo = LM700X_STEREO; /* Use stereo by default */
static int vol = 0;
static u_int32_t radioport = 0;

/******************************************************************/

struct tuner_drv_t *
export_aztech(void) {
	return &aztech_drv;
}

int
get_port_aztech(u_int32_t port) {
	radioport = port;
	return radio_get_ioperms(radioport, 1);
}

int
free_port_aztech(void) {
	return radio_release_ioperms(radioport, 1);
}

u_int32_t
info_port_aztech(void) {
	return radioport;
}

void
set_freq_aztech(u_int16_t frequency) {
	int i;
	u_int32_t reg;

	reg  = lm700x_encode_freq(frequency, LM700X_REF_050);
	reg |= stereo | LM700X_REF_050 | LM700X_DIVIDER_FM;

	for (i = 0; i < LM700X_REGISTER_LENGTH; i++)
		if (reg & (1 << i))
			send_one();
		else
			send_zero();

	OUTB(radioport, 0x80+0x40+vol); /* Hey, we're done */

	return;
}

/*
 * Volume is adjusted by manipulating bits 00000x0x,
 * that is, the only allowed values are 101(5), 100(4), 001(1), 000(0)
 */
void
set_vol_aztech(int v) {
	if (v < 0)
		v = 0;
	if (v > 3)
		v = 3;

	switch (v) {
	case 0:
	case 1:
		vol = v;
		break;
	case 2:
		vol = 4;
		break;
	case 3:
		vol = 5;
		break;
	}

	OUTB(radioport, vol);
}

int
state_aztech(void) {
	int res, ret = 0;
	
	res  = inb(radioport) & 3;
	ret |= res & AZTECH_STEREO ? 0 : DRV_INFO_STEREO;
	ret |= res & AZTECH_SIGNAL ? 0 : DRV_INFO_SIGNAL;

	return ret;
}

void
mono_aztech(void) {
	stereo = LM700X_MONO;
}

static void
send_zero(void) {
	OUTB(radioport, 0x02+vol);
	OUTB(radioport, 0x40+0x02+vol);
}

static void
send_one(void) {
	OUTB(radioport, 0x80+0x02+vol);
	OUTB(radioport, 0x80+0x40+0x02+vol);
}
