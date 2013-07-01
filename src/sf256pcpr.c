/*
 * Copyright (c) 2002 Vladimir Popov <jumbo@narod.ru>.
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
 * $Id: sf256pcpr.c,v 1.8 2002/01/14 10:21:39 pva Exp $
 * TEA5757: http://www.semiconductors.com/acrobat/datasheets/TEA5757_5759_3.pdf
 * lowlevel io functions for fmio utility -- sf256pcpr driver
 */

#include <errno.h>

#include "ostypes.h"

#include "radio_drv.h"
#include "tea5757.h"

#define SF256PCPR_CAPS	DRV_INFO_NEEDS_ROOT | DRV_INFO_HARDW_SRCH | \
			DRV_INFO_KNOWS_FREQ | DRV_INFO_MONOSTEREO | \
			DRV_INFO_GETS_SIGNAL | DRV_INFO_VOLUME(1) | \
			DRV_INFO_VOL_SEPARATE

void set_volume_sf256pcpr(int);
int get_port_sf256pcpr(u_int32_t);
int free_port_sf256pcpr(void);
u_int32_t info_port_sf256pcpr(void);
int find_card_sf256pcpr(void);
void set_frequency_sf256pcpr(u_int16_t);
u_int16_t get_frequency_sf256pcpr(void);
u_int16_t search_sf256pcpr(int, u_int16_t);
int state_sf256pcpr(void);
void mono_sf256pcpr(void);

struct tuner_drv_t sf256pcpr_drv = {
	"SoundForte Quad X-treme SF256-PCP-R",
	"sqx", NULL, 0, SF256PCPR_CAPS,
	get_port_sf256pcpr, free_port_sf256pcpr, info_port_sf256pcpr,
	find_card_sf256pcpr, set_frequency_sf256pcpr,
	get_frequency_sf256pcpr, search_sf256pcpr,
	set_volume_sf256pcpr, NULL, mono_sf256pcpr, state_sf256pcpr
};

static void send_zero(void);
static void send_one(void);
static u_int32_t read_shift_register(void);
static void write_shift_register(u_int32_t);
static u_int32_t read_shift_register(void);

static u_int32_t radioport = 0;
static struct tea5757_t card = {
	TEA5757_SEARCH_END, 0, TEA5757_S030, TEA5757_STEREO,
	read_shift_register, write_shift_register
};

/*********************************************************************/

struct tuner_drv_t *
export_sf256pcpr(void) {
	return &sf256pcpr_drv;
}

int
get_port_sf256pcpr(u_int32_t port) {
	return radio_get_iopl() < 0 ? -1 : 0;
}

int
free_port_sf256pcpr(void) {
	return radio_release_iopl();
}

u_int32_t
info_port_sf256pcpr(void) {
	return radioport;
}

void
set_volume_sf256pcpr(int volu) {
	u_int16_t value = volu ? 0xf804 : 0xf800;

	OUTW(radioport, value);
	usleep(6);
	OUTW(radioport, value);
}

u_int16_t
search_sf256pcpr(int dir, u_int16_t freq) {
	card.frequency = freq;
	card.search = dir ? TEA5757_SEARCH_UP : TEA5757_SEARCH_DOWN;
	return tea5757_search(&card);
}

u_int16_t
get_frequency_sf256pcpr(void) {
	return tea5757_decode_frequency(tea5757_read_shift_register(&card));
}

/*
 * Set frequency and other stuff
 * Basically, this is just writing the 25-bit shift register
 */
void
set_frequency_sf256pcpr(u_int16_t freq) {
	card.frequency = freq;
	card.search = TEA5757_SEARCH_END;
	tea5757_write_shift_register(&card);
	return;
}

int
state_sf256pcpr(void) {
	/* Funny, one mksec less and this won't work */
	usleep(120001);

	/* stereo : mono or no signal */
	return inw(radioport - 0x2c) == 4 ? DRV_INFO_SIGNAL : 0;
}

void
mono_sf256pcpr(void) {
	card.stereo = TEA5757_MONO;
}

static void
send_zero(void) {
	OUTW(radioport, 0xf800);
	OUTW(radioport, 0xf801);
	OUTW(radioport, 0xf800);
}

static void
send_one(void) {
	OUTW(radioport, 0xf802);
	OUTW(radioport, 0xf803);
	OUTW(radioport, 0xf802);
}

static void
write_shift_register(u_int32_t data) {
	int c = 25;

	OUTW(radioport, 0xf800);

	while (c--)
		if (data & (1 << c))
			send_one();
		else
			send_zero();

	OUTW(radioport, 0xf804);
}

static u_int32_t
read_shift_register(void) {
	u_int32_t res = 0ul;
	int rb;

	/* Read the register */
	OUTW(radioport, 0xfa06);
	rb = 24;
	while (rb--) {
		res <<= 1;
		OUTW(radioport, 0xfa07);
		OUTW(radioport, 0xfa06);
		res |= inw(radioport) & 0x02 ? 1 : 0;
	} 

	return (res & (TEA5757_DATA | TEA5757_FREQ));
}


int
find_card_sf256pcpr(void) {
	u_int16_t cur_freq, test_freq = 0ul;
	int err = -1;

	struct pci_dev_t pd = {
		0x1319 /* vendor id */, 0x0801 /* device id */,
		0x1319 /* subsystem vendor id */, 0x1319 /* subsystem id */,
		PCI_SUBCLASS_MULTIMEDIA_AUDIO, 0xb2
	};

	radioport = pci_bus_locate(&pd);
	if (radioport == 0) {
		errno = ENXIO;
		return -1;
	}
	radioport += 0x52;

	/* Save old value */
	cur_freq = get_frequency_sf256pcpr();
	set_frequency_sf256pcpr(TEST_FREQ);
	test_freq = get_frequency_sf256pcpr();
	if (test_freq == TEST_FREQ)
		err = 0;
	else
		radioport = 0;

	set_frequency_sf256pcpr(cur_freq);

	return err;
}
