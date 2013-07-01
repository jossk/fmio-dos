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
 * $Id: tea5757.c,v 1.10 2001/10/04 16:13:23 pva Exp $
 * implementation of routines for TEA5757 chip
 */

#include <unistd.h>

#include "ostypes.h"

#include "radio_drv.h"
#include "tea5757.h"

void
tea5757_write_shift_register(struct tea5757_t *card) {
	u_int32_t reg = 0ul;

	if (card->frequency) {
		reg = card->frequency;
		reg += 1070;
		reg /= 1.25;
	} else {
		reg = TEA5757_SEARCH_START | card->search;
	}

	reg |= card->stereo;
	reg |= card->sensitivity;

	card->write(reg);

	return;
}

u_int32_t
tea5757_read_shift_register(struct tea5757_t *card) {
	u_int32_t reg;

	usleep(TEA5757_ACQUISITION_DELAY);
	reg = card->read();

	return reg;
}

u_int32_t
tea5757_search(struct tea5757_t *card) {
	u_int32_t tmp = card->search;
	unsigned int co = 0;

	card->search = TEA5757_SEARCH_END;
	tea5757_write_shift_register(card);

	usleep(TEA5757_ACQUISITION_DELAY);
	card->frequency = 0;
	card->search = tmp;
	tmp = card->frequency;
	tea5757_write_shift_register(card);

	card->frequency = tmp;

	do {
		usleep(TEA5757_WAIT_DELAY);
		tmp = card->read(); 
	} while ((tmp & TEA5757_FREQ) == 0 && ++co < 200);

	if (co > 199) {
		card->search = TEA5757_SEARCH_END;
		tea5757_write_shift_register(card);
	}

	return co < 200 ? tea5757_decode_frequency(tmp) : card->frequency;
}

u_int32_t
tea5757_decode_frequency(u_int32_t data) {
	u_int32_t freq = data & TEA5757_FREQ;

	freq *= 1.25;
	freq -= 1070;

	return freq;
}
