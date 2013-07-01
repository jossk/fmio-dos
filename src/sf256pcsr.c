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
 * $Id: sf256pcs.c,v 1.8 2002/01/14 10:21:39 pva Exp $
 * TEA5757: http://www.semiconductors.com/acrobat/datasheets/TEA5757_5759_3.pdf
 * lowlevel io functions for fmio utility -- sf256pcs driver
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

#define SF256_CAPS	DRV_INFO_NEEDS_ROOT | DRV_INFO_HARDW_SRCH | \
			DRV_INFO_KNOWS_FREQ | DRV_INFO_MONOSTEREO | \
			DRV_INFO_VOLUME(1) | DRV_INFO_VOL_SEPARATE

/* Exported functions */
int get_port_sf256pcs(u_int32_t);
int free_port_sf256pcs(void);
u_int32_t info_port_sf256pcs(void);
int find_card_sf256pcs(void);
void set_frequency_sf256pcs(u_int16_t);
u_int16_t get_frequency_sf256pcs(void);
u_int16_t search_sf256pcs(int, u_int16_t);
void set_volume_sf256pcs(int);
void mono_sf256pcs(void);

/* Export structure */
static struct tuner_drv_t sf256pcs_drv = {
	"SoundForte Theatre X-treme 5.1 SF256-PCS-R",
	"stx", NULL, 0, SF256_CAPS, get_port_sf256pcs,
	free_port_sf256pcs, info_port_sf256pcs, find_card_sf256pcs,
	set_frequency_sf256pcs, get_frequency_sf256pcs, search_sf256pcs,
	set_volume_sf256pcs, NULL, mono_sf256pcs, NULL
};

/* Internal functions */
static void send_zero(void);
static void send_one(void);
static u_int32_t read_shift_register(void);
static void write_shift_register(u_int32_t);
static u_int32_t read_shift_register(void);

/* Internal variables */
static u_int32_t radioport = 0;

static struct tea5757_t card = {
	TEA5757_SEARCH_END, 0, TEA5757_S030, TEA5757_STEREO,
	read_shift_register, write_shift_register
};

/************* EXPORT ************************************************/
struct tuner_drv_t *
export_sf256pcs(void) {
	return &sf256pcs_drv;
}

/*********************************************************************/

int
get_port_sf256pcs(u_int32_t port) {
	return radio_get_iopl() < 0 ? -1 : 0;
}

int
free_port_sf256pcs(void) {
	return radio_release_iopl();
}

u_int32_t
info_port_sf256pcs(void) {
	return radioport;
}

void
set_volume_sf256pcs(int volu) {
	u_int16_t value = volu ? 0xe004 : 0xe000;

	OUTW(radioport, value);
	usleep(6);
	OUTW(radioport, value);
}

static void
send_zero(void) {
	OUTW(radioport, 0xe000);
	OUTW(radioport, 0xe008);
	OUTW(radioport, 0xe000);
}

static void
send_one(void) {
	OUTW(radioport, 0xe002);
	OUTW(radioport, 0xe00a);
	OUTW(radioport, 0xe002);
}

/*
 * Set frequency and other stuff
 * Basically, this is just writing the 25-bit shift register
 */
void
set_frequency_sf256pcs(u_int16_t freq) {
	card.frequency = freq;
	card.search = TEA5757_SEARCH_END;
	tea5757_write_shift_register(&card);
	return;
}

void
mono_sf256pcs(void) {
	card.stereo = TEA5757_MONO;
}

static void
write_shift_register(u_int32_t data) {
	int c = 25;

	OUTW(radioport, 0xe000);

	while (c--)
		if (data & (1 << c))
			send_one();
		else
			send_zero();

	OUTW(radioport, 0xe004);
}

u_int32_t
read_shift_register(void) {
	u_int32_t res = 0ul;
	int rb;

	/* Read the register */
	rb = 24;
	while (rb--) {
		res <<= 1;
		OUTW(radioport, 0xe206);
		OUTW(radioport, 0xe20e);
		res |= inw(radioport) & 0x02 ? 1 : 0;
	} 

	return (res & (TEA5757_DATA | TEA5757_FREQ));
}

u_int16_t
get_frequency_sf256pcs(void) {
	return tea5757_decode_frequency(tea5757_read_shift_register(&card));
}

int
find_card_sf256pcs(void) {
	u_int32_t cur_freq, test_freq = 0ul;
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
	cur_freq = get_frequency_sf256pcs();

	set_frequency_sf256pcs(TEST_FREQ);
	test_freq = get_frequency_sf256pcs();
	if (test_freq == TEST_FREQ)
		err = 0;
	else
		radioport = 0;

	set_frequency_sf256pcs(cur_freq);

	return err;
}

u_int16_t
search_sf256pcs(int dir, u_int16_t freq) {
	card.frequency = freq;
	card.search = dir ? TEA5757_SEARCH_UP : TEA5757_SEARCH_DOWN;
	return tea5757_search(&card);
}
