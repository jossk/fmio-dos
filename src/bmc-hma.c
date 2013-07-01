/*
 * Copyright (c) 2002 Gunther Mayer
 * Copyright (c 2001, 2002 Vladimir Popov <jumbo@narod.ru>.
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

/* BMC/ FCC ID HMA00-0076 Radio Card driver
 *
 * The PCB has a print "BMC" and "FCC ID HMA00-0076" on it.
 * The card was sold e.g. as some "Typhoon Radio Card" (Note: Typhoon used at least
 *  4 different cards as OEM!)
 *
 * IO is jumper selectable 0x20f or 0x30f
 *
 * Frequency is controlled by
 *   Toshiba's High Speed PLL for DTS TC9216P
 *    3wire protocol pins are connected to Bit 0-2
 * Volume is controlled by
 *   Setting Bits 3-6 (Bit6=MSB)
 *    Note: Volumerange is 0-15; though 12-15 usable, lower than 12 is low volume 
 * Mono/Stereo is controlled by
 *   Bit 7: 1=Stereo 0=Mono
 */

#include "ostypes.h"

#include "radio_drv.h"
#include "tc921x.h"

#define BMC_CAPS		DRV_INFO_NEEDS_ROOT | DRV_INFO_KNOWS_FREQ | \
				DRV_INFO_MONOSTEREO | DRV_INFO_VOLUME(15) | \
				DRV_INFO_MAXVOL_POLICY | DRV_INFO_VOL_SEPARATE

#define bmc_FREQ_DATA_ON	(1 << 0)
#define bmc_FREQ_CLOCK_ON	(1 << 1)
#define bmc_FREQ_PERIOD_ON	(1 << 2)

#define bmc_VOLUME_MASK		(0x78)  /* Bit 3-6 */

#define BMC_STEREO		(1 << 7) /* 0x80 for Stereo, else: Mono */
#define BMC_MONO		0

#define bmc_FREQ_STEADY	\
		bmc_FREQ_DATA_ON | \
		bmc_FREQ_CLOCK_ON | \
		bmc_FREQ_PERIOD_ON

#define USE_CHANNEL	1

int get_port_bmc(u_int32_t);
int free_port_bmc(void);
u_int32_t info_port_bmc(void);
int find_card_bmc(void);
void set_freq_bmc(u_int16_t);
u_int16_t get_freq_bmc(void);
void set_vol_bmc(int);
void set_mono_bmc(void);

u_int32_t bmc_ports[] = { 0x20f, 0x30f };

struct tuner_drv_t bmc_drv = {
	"BMC fcc-id HMA00-0076", "bmc", bmc_ports, 2, BMC_CAPS,
	get_port_bmc, free_port_bmc, info_port_bmc, find_card_bmc,
	set_freq_bmc, get_freq_bmc, NULL,
	set_vol_bmc, NULL, set_mono_bmc, NULL
};

static struct tc921x_t card = {
	0, bmc_FREQ_PERIOD_ON, bmc_FREQ_CLOCK_ON, bmc_FREQ_DATA_ON
};

static int stereo = BMC_STEREO;

/******************************************************************/

struct tuner_drv_t *
export_bmc(void) {
	return &bmc_drv;
}

int
get_port_bmc(u_int32_t port) {
	card.port = port;
        return radio_get_ioperms(card.port, 1) < 0 ? -1 : 0;
}

int
free_port_bmc(void) {
	return radio_release_ioperms(card.port, 1);
}

u_int32_t
info_port_bmc(void) {
	return card.port;
}

void
set_freq_bmc(u_int16_t frequency) {
	u_int32_t data = 0ul;

	data |= tc921x_encode_freq(frequency);
	data |= TC921X_D0_REF_FREQ_10_KHZ;
	data |= TC921X_D0_PULSE_SWALLOW_FM_MODE;
	data |= TC921X_D0_OSC_7POINT2_MHZ;
	data |= TC921X_D0_OUT_CONTROL_ON;
	tc921x_write_addr(&card, 0xD0, data);

	data  = TC921X_D2_IO_PORT_OUTPUT(4);
	tc921x_write_addr(&card, 0xD2, data);
}

u_int16_t
get_freq_bmc(void) {
	return tc921x_decode_freq(tc921x_read_addr(&card, 0xD1));
}

void
set_vol_bmc(int v) {
	int i;

	i = (v & 0x0f) << 3; /* set volume bits */
	/* leave 3wire tc9216 bits HI, else undef state */
	OUTB(card.port, i | stereo | 0x07);
}

int
find_card_bmc(void) {
        u_int16_t cur_freq = 0ul;
        u_int16_t test_freq = 0ul;
        int err = -1;

        cur_freq = get_freq_bmc();

        set_freq_bmc(TEST_FREQ);
        test_freq = get_freq_bmc();
        if (test_freq == TEST_FREQ) {
                set_freq_bmc(cur_freq);
                err = 0;
        }

        return err;
}

void
set_mono_bmc(void) {
	stereo = BMC_MONO;
}
