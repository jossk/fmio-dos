/*
 * Copyright (c) 2001, 2002 Vladimir Popov <jumbo@narod.ru>.
 * Copyright (c) 2000, 2001 Wilmer van der Gaast <lintux@lintux.cx>
 * Copyright (c) 1998 Kevin Boone <k.boone@mdx.ac.uk>
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
 * $Id: sf16fmr.c,v 1.21 2002/01/15 11:05:06 pva Exp $
 * lowlevel io functions for fmio utility -- sf16-fmr driver
 */

/*
 * Frequency is controlled by
 *   Toshiba's High Speed PLL for DTS TC9216P
 * Volume is controlled by
 *   Princeton Technology Corp.'s Electronic Volume Controller IC PT2254A.
 */

#include "ostypes.h"

#include "pt2254a.h"
#include "radio_drv.h"
#include "tc921x.h"

#define SF16FMR_CAPS	DRV_INFO_NEEDS_ROOT | DRV_INFO_KNOWS_FREQ | \
			DRV_INFO_VOLUME(15) | DRV_INFO_VOL_SEPARATE | \
			DRV_INFO_MAXVOL_POLICY

#define SF16FMR_FREQ_DATA_ON	(1 << 0)
#define SF16FMR_FREQ_CLOCK_ON	(1 << 1)
#define SF16FMR_FREQ_PERIOD_ON	(1 << 2)

#define SF16FMR_FREQ_STEADY	\
		SF16FMR_FREQ_DATA_ON | \
		SF16FMR_FREQ_CLOCK_ON | \
		SF16FMR_FREQ_PERIOD_ON

#define SF16FMR_VOLU_STROBE_ON	(1 << 3)
#define SF16FMR_VOLU_STROBE_OFF	(0 << 3)
#define SF16FMR_VOLU_CLOCK_ON	(1 << 4)
#define SF16FMR_VOLU_CLOCK_OFF	(0 << 4)
#define SF16FMR_VOLU_DATA_ON	(1 << 5)
#define SF16FMR_VOLU_DATA_OFF	(0 << 5)

int get_port_sf16fmr(u_int32_t);
int free_port_sf16fmr(void);
u_int32_t info_port_sf16fmr(void);
int find_card_sf16fmr(void);
void set_freq_sf16fmr(u_int16_t);
u_int16_t get_freq_sf16fmr(void);
void set_vol_sf16fmr(int);

u_int32_t sfr_ports[] = {0x284, 0x384};

struct tuner_drv_t sf16fmr_drv = {
	"SoundForte RadioLink SF16-FMR",
	"sfr", sfr_ports, 2, SF16FMR_CAPS,
	get_port_sf16fmr, free_port_sf16fmr, info_port_sf16fmr,
	find_card_sf16fmr, set_freq_sf16fmr, get_freq_sf16fmr, NULL,
	set_vol_sf16fmr, NULL, NULL, NULL
};

static struct tc921x_t card = {
	0, SF16FMR_FREQ_PERIOD_ON, SF16FMR_FREQ_CLOCK_ON, SF16FMR_FREQ_DATA_ON
};

static void send_vol_bit(int);


/******************************************************************/

struct tuner_drv_t *
export_sf16fmr(void) {
	return &sf16fmr_drv;
}

int
get_port_sf16fmr(u_int32_t port) {
	card.port = port;
        return radio_get_ioperms(card.port, 1) < 0 ? -1 : 0;
}

int
free_port_sf16fmr(void) {
	return radio_release_ioperms(card.port, 1);
}

u_int32_t
info_port_sf16fmr(void) {
	return card.port;
}

void
set_freq_sf16fmr(u_int16_t frequency) {
	u_int32_t data = 0ul;

	data  = tc921x_encode_freq(frequency);
	data |= TC921X_D0_REF_FREQ_10_KHZ;
	data |= TC921X_D0_PULSE_SWALLOW_FM_MODE;
	data |= TC921X_D0_OSC_7POINT2_MHZ;
	data |= TC921X_D0_OUT_CONTROL_ON;
	tc921x_write_addr(&card, 0xD0, data);

	data |= TC921X_D2_IO_PORT_OUTPUT(4);
	tc921x_write_addr(&card, 0xD2, data);
}

u_int16_t
get_freq_sf16fmr(void) {
	return tc921x_decode_freq(tc921x_read_addr(&card, 0xD1));
}

#if 0
int
state_sf16fmr(void) {
	u_int32_t d;
	int ret = 0;

	do {
		d = tc921x_read_addr(&card, 0xD1);
		warnx("0x%x", d);
	} while (d & TC921X_D1_BUSY);

	if (d & TC921X_D1_UNLOCK)
		ret = 2; /* mono */
	if ((d & TC921X_D1_COUNTER_DATA) == 0)
		ret += 1; /* no signal */

	return ret;
}
#endif /* 0 */

static void
send_vol_bit(int i) {
	unsigned int data;

	data  = SF16FMR_FREQ_STEADY | SF16FMR_VOLU_STROBE_OFF;
	data |= i ? SF16FMR_VOLU_DATA_ON : SF16FMR_VOLU_DATA_OFF;

	OUTB(card.port, data | SF16FMR_VOLU_CLOCK_OFF);
	OUTB(card.port, data | SF16FMR_VOLU_CLOCK_ON);
}

void
set_vol_sf16fmr(int v) {
	u_int32_t reg, vol;
	int i;

	vol = pt2254a_encode_volume(v, 15);
	reg = pt2254a_compose_register(vol, vol, USE_CHANNEL, USE_CHANNEL);

	OUTB(card.port, SF16FMR_FREQ_STEADY | SF16FMR_VOLU_STROBE_OFF);
	
	for (i = 0; i < PT2254A_REGISTER_LENGTH; i++)
		send_vol_bit(reg & (1 << i));

	/* Latch the data */
	OUTB(card.port, SF16FMR_FREQ_STEADY | SF16FMR_VOLU_STROBE_ON);
	OUTB(card.port, SF16FMR_FREQ_STEADY | SF16FMR_VOLU_STROBE_OFF);
}

int
find_card_sf16fmr(void) {
        u_int16_t cur_freq = 0ul;
        u_int16_t test_freq = 0ul;
        int err = -1;

        cur_freq = get_freq_sf16fmr();

        set_freq_sf16fmr(TEST_FREQ);
        test_freq = get_freq_sf16fmr();
        if (test_freq == TEST_FREQ) {
                set_freq_sf16fmr(cur_freq);
                err = 0;
        }

        return err;
}
