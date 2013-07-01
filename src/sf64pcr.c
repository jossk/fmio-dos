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
 * $Id: sf64pcr.c,v 1.8 2002/01/14 10:21:39 pva Exp $
 * TEA5757: http://www.semiconductors.com/acrobat/datasheets/TEA5757_5759_3.pdf
 * lowlevel io functions for fmio utility -- sf64pcr driver
 */

#include <errno.h>

#include "ostypes.h"

#include "radio_drv.h"
#include "tea5757.h"

#ifndef PCI_VENDOR_ID_FORTEMEDIA
#define PCI_VENDOR_ID_FORTEMEDIA	0x1319
#endif /* PCI_VENDOR_ID_FORTEMEDIA */

#ifndef PCI_DEVICE_ID_FORTEMEDIA_FM801
#define PCI_DEVICE_ID_FORTEMEDIA_FM801	0x0801
#endif /* PCI_VENDOR_ID_FORTEMEDIA_FM801 */

#define SF64PCR_CAPS	DRV_INFO_NEEDS_ROOT  | DRV_INFO_VOLUME(1) | \
			DRV_INFO_KNOWS_FREQ  | DRV_INFO_MONOSTEREO | \
			DRV_INFO_GETS_SIGNAL | DRV_INFO_GETS_STEREO | \
			DRV_INFO_VOL_SEPARATE

/* DRV_INFO_HARDW_SRCH | \ */

int get_port_sf64pcr(u_int32_t);
int free_port_sf64pcr(void);
u_int32_t info_port_sf64pcr(void);
int find_card_sf64pcr(void);
u_int16_t get_frequency_sf64pcr(void);
void set_frequency_sf64pcr(u_int16_t);
void mute_sf64pcr(int v);
void mono_sf64pcr(void);
int state_sf64pcr(void);

struct tuner_drv_t sf64pcr_drv = {
	"SoundForte RadioLink SF64-PCR",
	"sf4r", NULL, 0, SF64PCR_CAPS,
	get_port_sf64pcr, free_port_sf64pcr, info_port_sf64pcr,
	find_card_sf64pcr, set_frequency_sf64pcr, get_frequency_sf64pcr,
	NULL, mute_sf64pcr, NULL, mono_sf64pcr, state_sf64pcr
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
export_sf64pcr(void) {
	return &sf64pcr_drv;
}

int
get_port_sf64pcr(u_int32_t port) {
	return radio_get_iopl() < 0 ? -1 : 0;
}

int
free_port_sf64pcr(void) {
	return radio_release_iopl();
}

u_int32_t
info_port_sf64pcr(void) {
	return radioport;
}

void
mute_sf64pcr(int v) {
	u_int16_t value = v ? 0xf802 : 0xf800;

	OUTW(radioport, value);
	usleep(6);
	OUTW(radioport, value);
}

/*
 * Set frequency and other stuff
 * Basically, this is just writing the 25-bit shift register
 */
void
set_frequency_sf64pcr(u_int16_t freq) {
	card.frequency = freq;
	card.search = TEA5757_SEARCH_END;
	tea5757_write_shift_register(&card);
	return;
}

u_int16_t
get_frequency_sf64pcr(void) {
	return tea5757_decode_frequency(tea5757_read_shift_register(&card));
}

int
state_sf64pcr(void) {
	int ret = read_shift_register();
	return (ret >> 25) & 0x03;
}

void
mono_sf64pcr(void) {
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
	OUTW(radioport, 0xf804);
	OUTW(radioport, 0xf805);
	OUTW(radioport, 0xf804);
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

	OUTW(radioport, 0xf802);
}

static u_int32_t
read_shift_register(void) {
	u_int32_t res = 0ul;
	int rb, ind = 0;

	OUTW(radioport, 0xfc02);
	usleep(4); 

	/* Read the register */
	rb = 23;
	while (rb--) {
		OUTW(radioport, 0xfc03);
		usleep(4);			

		OUTW(radioport, 0xfc02);
		usleep(4);

		res |= inw(radioport) & 0x04 ? 1 : 0;
		res <<= 1;
	}

	OUTW(radioport, 0xfc03);
	usleep(4);			

	rb = inw(radioport);
	ind = rb & 0x08 ? 0 : DRV_INFO_SIGNAL; /* Tuning */

	OUTW(radioport, 0xfc02);

	rb = inw(radioport);
	ind |= rb & 0x08 ? 0 : DRV_INFO_STEREO; /* Mono */
	res |= rb & 0x04 ? 1 : 0;

	return (res & (TEA5757_DATA | TEA5757_FREQ)) | (ind << 25);
}

int
find_card_sf64pcr(void) {
	u_int16_t cur_freq, test_freq = 0ul;
	int err = -1;

	struct pci_dev_t pd = {
		PCI_VENDOR_ID_FORTEMEDIA, PCI_DEVICE_ID_FORTEMEDIA_FM801,
		PCI_SUBSYS_ID_ANY, PCI_SUBSYS_ID_ANY, PCI_SUBCLASS_ANY,
		PCI_REVISION_ANY
	};

	radioport = pci_bus_locate(&pd);
	if (radioport == 0) {
		errno = ENXIO;
		return -1;
	}
	radioport += 0x52;

	/* Save old value */
	cur_freq = get_frequency_sf64pcr();

	set_frequency_sf64pcr(TEST_FREQ);
	test_freq = get_frequency_sf64pcr();
	if (test_freq == TEST_FREQ)
		err = 0;
	else
		radioport = 0;

	set_frequency_sf64pcr(cur_freq);

	return err;
}
