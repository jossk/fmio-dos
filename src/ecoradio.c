/*
 * Copyright (c) 2002 Gunther Mayer
 * (c) 1999 Dr. Henrik Seidel <Henrik.Seidel@gmx.de>
 * Copyright (c) 2001, 2002 Vladimir Popov <jumbo@narod.ru>.
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
 * This is a driver for the Ecoradio
 *
 * Based on code by:
 * Henrik Seidel as found in linux-2.4.18 "radio-typhoon"
 *
 * Notes on the hardware
 *
 * The card has auto-stereo according to its manual, although it all
 * sounds mono to me (even with the Win/DOS drivers). Maybe it's my
 * antenna - I really don't know for sure.
 *
 * Frequency control is done digitally.
 *
 * Volume control is done digitally (0..3) on speaker output.
 *  Strange: Line out volume control is inverse ( i.e. 0 is loudest)
 *
 * No real mute; Volume=0 as approximation (on spkr out);
 *
 * Ports 0x316 od 0x336:
 * WRITE
 *  io:   Volume High Bit
 *  io+2: Volume Low Bit
 *  io+4: Freq.
 *  io+6: Freq.
 *  io+8: Freq.
 * READ: all ports read 0xFF !
 * 
 * Note: The manual mentions interrupts (INT3,4,5,7) for the "Autoscanning" feature.
 *       (Proprietary, no info known. Perhaps the "tuned" indicator would raise an irq?)
 */
 
#include "ostypes.h"

#include "radio_drv.h"

#define ER_CAPS		DRV_INFO_NEEDS_ROOT | DRV_INFO_VOLUME(3)

int get_port_ecoradio(u_int32_t);
int free_port_ecoradio(void);
u_int32_t info_port_ecoradio(void);
void set_freq_ecoradio(u_int16_t);
void volume_ecoradio(int);

u_int32_t er_ports[] = { 0x316, 0x336 };

struct tuner_drv_t er_drv = {
	"EcoRadio (JTR-9401)", "er", er_ports, 2, ER_CAPS,
	get_port_ecoradio, free_port_ecoradio, info_port_ecoradio,
	NULL, set_freq_ecoradio, NULL, NULL, volume_ecoradio, NULL,
	NULL, NULL
};

static u_int32_t io;

struct tuner_drv_t *
export_er(void) {
	return &er_drv;
}

int
get_port_ecoradio(u_int32_t port) {
	io = port;
	return radio_get_ioperms(io, 9);
}

int
free_port_ecoradio(void) {
	return radio_release_ioperms(io, 9);
}

u_int32_t
info_port_ecoradio(void) {
	return io;
}

void
set_freq_ecoradio(u_int16_t frequency) {
	u_int32_t outval;
	u_int32_t x;
	/*
	 * The frequency transfer curve is not linear. The best fit I could
	 * get is
	 * outval = -155 + exp((f + 15.55) * 0.057))
	 * where frequency f is in MHz. Since we don't have exp in the kernel,
	 * I approximate this function by a third order polynomial.
	 */
	x = frequency; 
	outval = (x * x + 2500) / 5000;
	outval = (outval * x + 5000) / 10000;
	outval -= (10 * x * x + 10433) / 20866;
	outval += 4 * x - 11505;

	OUTB(io + 4, (outval >> 8) & 0x01);
	OUTB(io + 6, outval >> 9);
	/* freq. change only takes effect when this byte written. */
	OUTB(io + 8, outval & 0xff);
}

void
volume_ecoradio(int vol) {
	if(vol > 3)
		vol = 3;
	if(vol < 0)
		vol = 0;

	OUTB(io, vol / 2);	 /* Set the volume, high bit. */
	OUTB(io + 2, vol % 2);       /* Set the volume, low bit.  */
}
