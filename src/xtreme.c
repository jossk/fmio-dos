/*
 * Copyright (c) 2001, 2002 Vladimir Popov <jumbo@narod.ru>
 * All rights reserved.
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
 * This driver is used to manage AIMS Lab Highway Xtreme Tuner cards
 * with BSD bktr device driver (you need to compile your kernel with
 * `option BT848_GPIO_ACCESS')
 *
 * bktr detects it as Video Highway Xtreme, Philips FR1236 SECAM FM tuner
 *
 * The card management protocol was looked up in a program written by
 * Flemming Jacobsen 971212, fj@login.dknet.dk
 */

#include <err.h>
#include <fcntl.h>	/* O_RDONLY */

#include "ostypes.h"

#include "radio_drv.h"
#include "tea5757.h"

#ifdef BSDBKTR

#define XTREME_CAPS		DRV_INFO_HARDW_SRCH | DRV_INFO_MONOSTEREO | \
				DRV_INFO_GETS_SIGNAL | DRV_INFO_GETS_STEREO | \
				DRV_INFO_VOLUME(1) | DRV_INFO_VOL_SEPARATE

const char *set_data_err = "BT848_GPIO_SET_DATA";
const char *set_en_err = "BT848_GPIO_SET_EN";
extern char *tuner_device_1;
extern char *tuner_device_2;

int get_port_xtreme(u_int32_t);
int free_port_xtreme(void);
int find_card_xtreme(void);
void set_freq_xtreme(u_int16_t);
u_int16_t search_xtreme(int, u_int16_t);
void mute_xtreme(int);
void mono_xtreme(void);
int state_xtreme(void);

struct tuner_drv_t xtreme_drv = {
	"AIMS Lab Highway Xtreme", "hx", NULL, 0, XTREME_CAPS,
	get_port_xtreme, free_port_xtreme, NULL, find_card_xtreme,
	set_freq_xtreme, NULL, search_xtreme, mute_xtreme, NULL,
	mono_xtreme, state_xtreme
};

static void send_bit(int, int);
static u_int32_t read_shift_register(void);
static void write_shift_register(u_int32_t);

static struct tea5757_t card = {
	TEA5757_SEARCH_END, 0, TEA5757_S030, TEA5757_STEREO,
	read_shift_register, write_shift_register
};

static unsigned int gpio_data = 0;
static int fd = -1;

struct tuner_drv_t *
export_xtreme(void) {
	return &xtreme_drv;
}

static void
send_bit(int pos, int val) {
	gpio_data &= ~(1 << pos);
	gpio_data |= (val << pos);

	if (ioctl(fd, BT848_GPIO_SET_DATA, &gpio_data) < 0)
		warn(set_data_err);
}

int
get_port_xtreme(u_int32_t radioport) {
	fd = radio_device_get(tuner_device_1, tuner_device_2, O_RDONLY);
	return fd < 0 ? -1 : 0;
}

int
free_port_xtreme(void) {
	return radio_device_release(fd, tuner_device_1);
}

int
find_card_xtreme(void) {
	int intern = AUDIO_INTERN;

	/* Check for working driver */
	if (ioctl(fd, BT848_SAUDIO, &intern) < 0 )
		return -1;
	if (ioctl(fd, BT848_GPIO_GET_DATA, &gpio_data) < 0)
		return -1;
	return 0;
}

void
mute_xtreme(int v) {
	gpio_data &= ~0x7;
	gpio_data |= v ? 0x02 : 0x01;
	if (ioctl(fd, BT848_GPIO_SET_DATA, &gpio_data) < 0)
		warn(set_data_err);
}

int
state_xtreme(void) {
	int ss;

	if (ioctl(fd, TVTUNER_GETSTATUS, &ss) < 0) return 3;

	ss &= 0x07;

	if (ss > 0x06) return (DRV_INFO_SIGNAL | DRV_INFO_STEREO);
	if (ss > 0x03) return DRV_INFO_STEREO;
	if (ss > 0x01) return DRV_INFO_SIGNAL;

	return 0;
}

static void
write_shift_register(u_int32_t reg) {
	int i;

	send_bit(5, 0);
	send_bit(5, 1);

	i = 0x3f;
	if (ioctl(fd, BT848_GPIO_SET_EN, &i) < 0)
		warn(set_en_err);

	i = 25;
	while (i--) {
		if (reg & (1 << i))
			send_bit(4, 1);
		else
			send_bit(4, 0);

		send_bit(3, 0);
		send_bit(3, 1);
		send_bit(3, 0);
	}

	i = 0x2f;
	if (ioctl(fd, BT848_GPIO_SET_EN, &i) < 0)
		warn(set_en_err);

	send_bit(5, 0);

	mute_xtreme(1);
}

void
mono_xtreme(void) {
	card.stereo = TEA5757_MONO;
}

void
set_freq_xtreme(u_int16_t freq) {
	card.frequency = freq;
	card.search = TEA5757_SEARCH_END;
	tea5757_write_shift_register(&card);
	return;
}

static u_int32_t
read_shift_register(void) {
	/* FIXME: stub */
	return 0ul;
}

u_int16_t
search_xtreme(int dir, u_int16_t freq) {
	card.frequency = freq;
	card.search = dir ? TEA5757_SEARCH_UP : TEA5757_SEARCH_DOWN;
	return tea5757_search(&card);
}
#endif /* BSDBKTR */
