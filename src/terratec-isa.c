/* Terratec Active Radio ISA
   for fmio
   (c) 09.06.2002 GM
   The user may choose between these 2 licenses at his will:
   Gnu Public License or BSD License
 */
/*

   Volume Control:
   uc is connected to 74hct164
   74hct164 is connected to two 4051bt (right and left audio)

   hef4051bt = 8-channel analogue multiplexer/demultiplexer
   74hct164  = 8-Bit Serial-In/Parallel-Out Shift Register

   Others:
   Octal D Flip-Flop w/Reset (74HCT273)
   74HC/HCT367 are hex non-inverting buffer/line drivers with 3-state outputs
   Quad 2-Input OR Gate (74HCT32).
   Quad Buffer; 3-State (74HCT125)

   RDS:
   SAA6588 

   Radio:
   OM5610 w tea5757

   ISAPNP-ID: TER2111

   Sources: "radio-terratec.c" from linux-2.4.xx, fmio sources

   Todo:
    Fix fmio volume handling.
	- Max Vol. when nothing is given on the commandline
		(so a user hasn't to figure out he needs to set volume first)
*/

#include "ostypes.h"

#include "radio_drv.h"
#include "tea5757.h"

#define TERRATEC_CAPS		DRV_INFO_NEEDS_ROOT | DRV_INFO_HARDW_SRCH | \
				DRV_INFO_MONOSTEREO

/* Tea5757 bus */
int TEA_data = 4;
int TEA_clk  = 8;
int TEA_wren = 0x10;

int get_port_tt(u_int32_t);
int free_port_tt(void);
u_int32_t info_port_tt(void);
int find_card_tt(void);
void set_frequency_tt(u_int16_t);
u_int16_t search_tt(int, u_int16_t);
void set_volume_tt(int);
void mono_tt(void);

u_int32_t tt_port[] = { 0x590 };

struct tuner_drv_t tt_drv = {
	"Terratec", "tt", tt_port, 1, TERRATEC_CAPS,
	get_port_tt, free_port_tt, info_port_tt, find_card_tt,
	set_frequency_tt, NULL, search_tt, set_volume_tt, NULL,
	mono_tt, NULL
};

static void write_shift_register(u_int32_t);
static u_int32_t read_shift_register(void);

static struct tea5757_t card = {
	TEA5757_SEARCH_END, 0, TEA5757_S030, TEA5757_STEREO,
	read_shift_register, write_shift_register
};

/******************************************************************/

struct tuner_drv_t *
export_tt(void) {
	return &tt_drv;
}

int
get_port_tt(u_int32_t port) {
	return radio_get_iopl() < 0 ? -1 : 0;
}

int
find_card_tt(void) {
	OUTB(*tt_port, 0);
	if ((inb(*tt_port) & 0x0f) != 0x08) {
		/* only bits 0-2 are writeable here; bit 3 is always 1 */
		print_wx("Terratec ActiveRadio ISA must be activated by isapnp!");
		return -1;
	}
	return 0;
}

int
free_port_tt(void) {
	return radio_release_iopl();
}

u_int32_t
info_port_tt(void) {
	return *tt_port;
}

void
set_frequency_tt(u_int16_t frequency) {
	card.frequency = frequency;
	card.search = TEA5757_SEARCH_END;
	tea5757_write_shift_register(&card);

	set_volume_tt(7);
}

u_int16_t
search_tt(int dir, u_int16_t freq) {
	card.frequency = freq;
	card.search = dir ? TEA5757_SEARCH_UP : TEA5757_SEARCH_DOWN;
	return tea5757_search(&card);
}

void
set_volume_tt(int volume) {
        int i;

        volume = volume + (volume * 32); /* change both channels */
        for (i = 0; i < 8; i++) {
                if (volume & (0x80 >> i))
                        OUTB(0x80, *tt_port + 1);
                else
			OUTB(0x00, *tt_port + 1);
        }
}

void
mono_tt(void) {
	card.stereo = TEA5757_MONO;
}

/*
 * it is not possible to read back from tea5757
 * with my terratec activeradio isa
 */
static u_int32_t
read_shift_register(void) {
	return 0;
}

static void
write_shift_register(u_int32_t data) {
    int c = 25;

	OUTB(*tt_port, TEA_wren);

	while (c--)
		if (data & (1 << c)) {
			OUTB(*tt_port, TEA_wren | TEA_data);
			OUTB(*tt_port, TEA_wren | TEA_clk | TEA_data);
			OUTB(*tt_port, TEA_wren | TEA_data);
		} else {
			OUTB(*tt_port, TEA_wren);
			OUTB(*tt_port, TEA_wren | TEA_clk);
			OUTB(*tt_port, TEA_wren);
		}
	OUTB(*tt_port, 0);
}
