/* Mediaforte SF64-PCE2 
   PCI Sound+Radio card with ES1968 maestro2 soundchip and tea5757 radio
   (c) 06.07.2002 GM
   The user may choose between these 2 licenses at his will:
   Gnu Public License or BSD License

   Sources: sf64pce2.c, radio-maestro.c
 */

#include <errno.h>
#include <stdio.h>

#include "ostypes.h"

#include "radio_drv.h"
#include "tea5757.h"

#ifndef PCI_VENDOR_ID_ESS
#define PCI_VENDOR_ID_ESS	0x125d
#endif

#ifndef PCI_DEVICE_ID_MEASTRO2
#define PCI_DEVICE_ID_MAESTRO2	0x1968
#endif 

#define SF64PCE2_CAPS		DRV_INFO_VOLUME(1) | DRV_INFO_NEEDS_ROOT | \
				DRV_INFO_HARDW_SRCH | DRV_INFO_KNOWS_FREQ | \
				DRV_INFO_MONOSTEREO | DRV_INFO_VOL_SEPARATE

int get_port_sf64pce2(u_int32_t);
int free_port_sf64pce2(void);
u_int32_t info_port_sf64pce2(void);
int find_card_sf64pce2(void);
void set_frequency_sf64pce2(u_int16_t);
u_int16_t get_frequency_sf64pce2(void);
u_int16_t search_sf64pce2(int, u_int16_t);
void mute_sf64pce2(int);
void mono_sf64pce2(void);
int state_sf64pce2(void);

struct tuner_drv_t pce2_drv = {
	"SoundForte Awesome 64R SF64-PCE2", "sae", NULL, 0, SF64PCE2_CAPS,
	get_port_sf64pce2, free_port_sf64pce2, info_port_sf64pce2,
	find_card_sf64pce2, set_frequency_sf64pce2, get_frequency_sf64pce2,
	search_sf64pce2, mute_sf64pce2, NULL, mono_sf64pce2, state_sf64pce2
};

static void write_shift_register(u_int32_t);
static u_int32_t read_shift_register(void);

static u_int32_t radioport = 0;

static struct tea5757_t card = {
	TEA5757_SEARCH_END, 0, TEA5757_S030, TEA5757_STEREO,
	read_shift_register, write_shift_register
};

/*
 * Tea5757 bus is connected to GPIO on ES1968
 * The GPIO data register is located at offset 0x60 of es1968
 * The es1968 additionally has WRITE_MASK register at 0x64 and
 * DIRECTION at 0x68
 */
static int TEA_data = 0x40;  /* connected to GPIO6 */
static int TEA_clk  = 0x80;
static int TEA_wren = 0x100;  /* wren = "WRite ENable" */
static int TEA_most = 0x200; 
/* Note: above bits has the proprietary side-effect of Muting the card!? */

/*********************************************************************/

struct tuner_drv_t *
export_sf64pce2(void) {
	return &pce2_drv;
}

int
get_port_sf64pce2(u_int32_t port) {
	return radio_get_iopl() < 0 ? -1 : 0;
}

int
free_port_sf64pce2(void) {
	return radio_release_iopl();
}

u_int32_t
info_port_sf64pce2(void) {
	return radioport;
}

/* Todo */
void
mute_sf64pce2(int v) {
}

/*
 * Set frequency and other stuff
 * Basically, this is just writing the 25-bit shift register
 */
void
set_frequency_sf64pce2(u_int16_t freq) {
	card.frequency = freq;
	card.search = TEA5757_SEARCH_END;
	tea5757_write_shift_register(&card);
	return;
}

/*
 * Though TEA_most is documented in maestro-radio, the code below
 * does not work. It always indicates: mono, not_tuned with my card.
 */
int
state_sf64pce2(void) {
	int state, is_stereo , is_tuned;

	/******* ES1968 code *********/
	/* enable writes */
	OUTW(radioport + 4, ~(TEA_data | TEA_clk | TEA_wren));
	/* set direction to output */
	OUTW(radioport + 8,  (TEA_data | TEA_clk | TEA_wren));

	/******** TEA5757 code ********/
	/* CLK low: read mono/stereo */
	OUTW(radioport, 0);
	state = inw(radioport);
	if (state & TEA_most)
		is_stereo = 0;
	else
		is_stereo = 1;
	/* CLK hi: read not_tuned/tuned */
	OUTW(radioport, TEA_clk);
	state = inw(radioport);
	if (state & TEA_most)

		is_tuned = 0;
	else
		is_tuned = 1;

	/* SF64-PCE2 code for unmute */
	OUTW(radioport,0);

	/* **************** FMIO API */
	if(is_stereo)
		return 0;
	else
		return 1;
}

void
mono_sf64pce2(void) {
	card.stereo = TEA5757_MONO;
}

static void
write_shift_register(u_int32_t data) {
	int c = 25, bit;

	/* enable writes */
	OUTW(radioport + 4, ~(TEA_data | TEA_clk | TEA_wren));
	/* set direction to output */
	OUTW(radioport + 8,  (TEA_data | TEA_clk | TEA_wren));

	/*
	 * This sequence resets the tea5757 shift register (though this
	 * is not documented in the datasheet) (this is helpful if CLK
	 * was triggered by accident or else...)
	 */
	OUTW(radioport, TEA_wren);
	OUTW(radioport, TEA_wren);
	OUTW(radioport, 0);
	OUTW(radioport, 0);
	OUTW(radioport, TEA_wren);
	OUTW(radioport, TEA_wren);

	while (c--) {
		if (data & (1 << c)) 
			bit = TEA_data;
		else
			bit = 0;
		OUTW(radioport, TEA_wren | bit);
		OUTW(radioport, TEA_wren | bit | TEA_clk);
		OUTW(radioport, TEA_wren | bit);
	}
	OUTW(radioport,0);  /* This is needed to un-mute SF64-PCE2! */
}

u_int32_t
read_shift_register(void) {
	u_int32_t res = 0ul;
	int rb = 24;

	OUTW(radioport + 4, ~(TEA_clk | TEA_wren)); /* enable writes */
	OUTW(radioport + 8,  (TEA_clk | TEA_wren)); /* set direction to output */

	OUTW(radioport, 0x00);  /* WREN low */
	while (rb--) {
		res <<= 1;
		OUTW(radioport, TEA_clk);
		OUTW(radioport, 0x00);
		res |= inw(radioport) & TEA_data ? 1 : 0;
	} 

	return (res & (TEA5757_DATA | TEA5757_FREQ));
}

u_int16_t
get_frequency_sf64pce2(void) {
	return tea5757_decode_frequency(tea5757_read_shift_register(&card));
}

int
find_card_sf64pce2(void) {
	u_int16_t cur_freq, test_freq = 0ul;
	int err = -1;
	struct pci_dev_t pd = {
		PCI_VENDOR_ID_ESS, PCI_DEVICE_ID_MAESTRO2,
		PCI_SUBSYS_ID_ANY, PCI_SUBSYS_ID_ANY, PCI_SUBCLASS_ANY,
		PCI_REVISION_ANY
	};

	radioport = pci_bus_locate(&pd);
	if (radioport == 0) {
		errno = ENXIO;
		return -1;
	}
	radioport += 0x60;

	/* Save old value */
	cur_freq = get_frequency_sf64pce2();

	set_frequency_sf64pce2(TEST_FREQ);
	test_freq = get_frequency_sf64pce2();
	if (test_freq == TEST_FREQ)
		err = 0;
	else
		radioport = 0;

	set_frequency_sf64pce2(cur_freq);

	return err;
}

u_int16_t
search_sf64pce2(int dir, u_int16_t freq) {
	card.frequency = freq;
	card.search = dir ? TEA5757_SEARCH_UP : TEA5757_SEARCH_DOWN;
	return tea5757_search(&card);
}
