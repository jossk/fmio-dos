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
 *   The BU2614 PLL frequency sinthesizers work up through the FM band.
 *   Featuring low radioation noise, low power dissipation, and highly
 *   sensitive built-in RF amps, the support an IF count function.
 *
 */
/*
 * $Id$
 */

#include "ostypes.h"

#include "bu2614.h"
#include "radio_drv.h"

u_int16_t
bu2614_conv_freq(u_int16_t freq) {
	return (freq + 1052)/1.2779553;
}

u_int16_t
bu2614_unconv_freq(u_int32_t reg) {
	return (((reg & BU2614_FREQ) * 1.2779553) - 1051);
}

void
bu2614_write(struct bu2614_t *c, u_int32_t reg) {
	u_int8_t i;

	/*
	 * Prepare for data transmission
	 *  CLOCK HIGH
	 *  DATA HIGH
	 *  CHIP-ENABLE LOW then HIGH
	 *  after 15 mksec CLOCK may go LOW
	 */
	OUTB(c->port, c->wren * 0 | c->clck * 1 | c->data * 1);
	OUTB(c->port, c->wren * 1 | c->clck * 1 | c->data * 1);
	usleep(15);

	for (i = 0; i < BU2614_REGISTER_LENGTH; i++) {
		if (reg & (1 << i)) {
			OUTB(c->port, c->wren * 1 | c->clck * 0 | c->data * 1);
			usleep(1);
			OUTB(c->port, c->wren * 1 | c->clck * 1 | c->data * 1);
			usleep(1);
		} else {
			OUTB(c->port, c->wren * 1 | c->clck * 0 | c->data * 0);
			usleep(1);
			OUTB(c->port, c->wren * 1 | c->clck * 1 | c->data * 0);
			usleep(1);
		}
	}

	/* Finish transmission */
	OUTB(c->port, c->wren * 0 | c->clck * 1 | c->data * 1);
}
