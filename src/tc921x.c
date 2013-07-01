/*
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
 * Toshiba's High Speed PLL for DTS
 *   http://www.chipbook.co.kr/pdf/ic/toshiba/TC9216.pdf
 *
 * TC9216P, TC9217P, TC9217F are a high speed PLL-LSI with built-in 2 modulus
 * prescaler. Each function is controlled through 3 serial bus lines and high
 * performance digital tuning system can be constitued.
 *
 * Each function is controlled by the data setting to a pair of 24-bit
 * registers. Each data of these registers is exchanged with controller side
 * by 3 serial lines of DATA, CLOCK and PERIOD.
 *
 * 8 address bits and 24 data bits, total 32 bits, are transferred thru
 * serial port.
 *
 * Input data is latched to the first and second input registers at the fall
 * of PERIOD signal and each function is activated.
 *
 * Each output data is latched to output register in parallel at the fall
 * timing of the 9th of CLOCK signal and can be received serially over the
 * DATA line. Serial data of DATA, CLOCK and PERIOD is synchronized with
 * crystal oscillation clock and tacken into the internal circuit of LSI.
 * Thus, if crystal oscillator is stopped, serial data can not be input.
 */
/*
 * $Id: tc921x.c,v 1.11 2002/01/19 06:49:16 pva Exp $
 */

#include <unistd.h>

#include "ostypes.h"

#include "radio_drv.h"
#include "tc921x.h"

#define WR_PL_CL_DL(port, p, c, d)	OUTB(port, p * 0 | c * 0 | d * 0)
#define WR_PL_CL_DH(port, p, c, d)	OUTB(port, p * 0 | c * 0 | d * 1)
#define WR_PL_CH_DL(port, p, c, d)	OUTB(port, p * 0 | c * 1 | d * 0)
#define WR_PL_CH_DH(port, p, c, d)	OUTB(port, p * 0 | c * 1 | d * 1)

#define WR_PH_CL_DL(port, p, c, d)	OUTB(port, p * 1 | c * 0 | d * 0)
#define WR_PH_CL_DH(port, p, c, d)	OUTB(port, p * 1 | c * 0 | d * 1)
#define WR_PH_CH_DL(port, p, c, d)	OUTB(port, p * 1 | c * 1 | d * 0)
#define WR_PH_CH_DH(port, p, c, d)	OUTB(port, p * 1 | c * 1 | d * 1)

static void __tc921x_write_burst(u_int8_t, u_int32_t, struct tc921x_t *, int);
static u_int32_t __tc921x_read_burst(u_int8_t, struct tc921x_t *);

u_int16_t
tc921x_encode_freq(u_int16_t freq) {
	u_int16_t ret;

	ret = freq + 1070;

	return ret;
}

u_int16_t
tc921x_decode_freq(u_int16_t reg) {
	u_int16_t freq;

	freq = (reg & TC921X_D0_FREQ_DIVIDER) - 1070;

	return freq;
}

u_int32_t
tc921x_read_addr(struct tc921x_t *c, u_int8_t addr) {
	u_int32_t ret;

	/* Finish previous transmission - PERIOD HIGH, CLOCK HIGH, DATA HIGH */
	WR_PH_CH_DH(c->port, c->period, c->clock, c->data);
	/* Start transmission - PERIOD LOW, CLOCK HIGH, DATA HIGH */
	WR_PL_CH_DH(c->port, c->period, c->clock, c->data);

	/*
	 * Period must be low when the register address transmission starts.
	 * Period must be high when the register data transmission starts.
	 * Do the switch in the middle of the address transmission.
	 */
	__tc921x_write_burst(4, addr, c, 0);
	__tc921x_write_burst(4, addr >> 4, c, 1);

	/* Reading data from the register */
	ret = __tc921x_read_burst(TC921X_REGISTER_LENGTH, c);

	/* End of transmission - PERIOD goes LOW then HIGH */
	WR_PL_CH_DH(c->port, c->period, c->clock, c->data);
	WR_PH_CH_DH(c->port, c->period, c->clock, c->data);

	return ret;
}

void
tc921x_write_addr(struct tc921x_t *c, u_int8_t addr, u_int32_t reg) {
	/* Finish previous transmission - PERIOD HIGH, CLOCK HIGH, DATA HIGH */
	WR_PH_CH_DH(c->port, c->period, c->clock, c->data);
	/* Start transmission - PERIOD LOW, CLOCK HIGH, DATA HIGH */
	WR_PL_CH_DH(c->port, c->period, c->clock, c->data);

	/*
	 * Period must be low when the register address transmission starts.
	 * Period must be high when the register data transmission starts.
	 * Do the switch in the middle of the address transmission.
	 */
	__tc921x_write_burst(4, addr, c, 0);
	__tc921x_write_burst(4, addr >> 4, c, 1);

	/* Writing data to the register */
	__tc921x_write_burst(TC921X_REGISTER_LENGTH, reg, c, 1);

	/* End of transmission - PERIOD goes LOW then HIGH */
	WR_PL_CH_DH(c->port, c->period, c->clock, c->data);
	WR_PH_CH_DH(c->port, c->period, c->clock, c->data);
}

static void
__tc921x_write_burst(u_int8_t length, u_int32_t data, struct tc921x_t *c, int p) {
	u_int8_t i;

	for (i = 0; i < length; i++) {
		if (data & (1 << i)) {
			WR_PH_CL_DH(c->port, c->period * p, c->clock, c->data);
			WR_PH_CH_DH(c->port, c->period * p, c->clock, c->data);
		} else {
			WR_PH_CL_DL(c->port, c->period * p, c->clock, c->data);
			WR_PH_CH_DL(c->port, c->period * p, c->clock, c->data);
		}
	}
}

static u_int32_t
__tc921x_read_burst(u_int8_t length, struct tc921x_t *c) {
	u_int8_t i;
	u_int32_t ret = 0ul;

	for (i = 0; i < length; i++) {
		WR_PH_CL_DH(c->port, c->period, c->clock, c->data);
		WR_PH_CH_DH(c->port, c->period, c->clock, c->data);
		ret |= inb(c->port) & c->data ? (1 << i) : (0 << i);
	}

	return ret;
}
