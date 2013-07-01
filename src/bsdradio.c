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
 * $Id: bsdradio.c,v 1.2 2002/01/17 06:04:07 pva Exp $
 * Driver for OpenBSD and NetBSD radio driver
 */

#include "ostypes.h"
#include "radio_drv.h"

#ifdef BSDRADIO

#include <sys/radioio.h>

#include <fcntl.h>	/* O_RDWR */

#define BSDRADIO_CAPS	DRV_INFO_KNOWS_FREQ | DRV_INFO_KNOWS_VOLU | \
			DRV_INFO_VOLUME(255) | DRV_INFO_MAXVOL_POLICY | \
			DRV_INFO_VOL_SEPARATE

int get_port_bsdradio(u_int32_t);
int free_port_bsdradio(void);
int find_card_bsdradio(void);
int get_volume_bsdradio(void);
u_int16_t get_frequency_bsdradio(void);
void mono_bsdradio(void);
int state_bsdradio(void);
void set_volume_bsdradio(int);
void set_frequency_bsdradio(u_int16_t);

static int do_rinfo(int, int, struct radio_info *);

#define SET_INFO	0
#define GET_INFO	1

struct tuner_drv_t bsdradio_drv = {
	"OpenBSD and NetBSD FM Radio", "br", NULL, 0, BSDRADIO_CAPS,
	get_port_bsdradio, free_port_bsdradio, NULL, find_card_bsdradio,
	set_frequency_bsdradio, get_frequency_bsdradio, NULL,
	set_volume_bsdradio, get_volume_bsdradio, mono_bsdradio,
	state_bsdradio
};

static int fd = -1;
static struct radio_info ri;

extern const char *radio_device_1;
extern const char *radio_device_2;

const char *riocsinfo = "RIOCSINFO";
const char *riocginfo = "RIOCGINFO";

/******************************************************************/

struct tuner_drv_t *
export_bsdradio(void) {
	return &bsdradio_drv;
}

int
find_card_bsdradio(void) {
	return do_rinfo(fd, GET_INFO, &ri);
}

int
get_port_bsdradio(u_int32_t port) {
	fd = radio_device_get(radio_device_1, radio_device_2, O_RDWR);
	return fd < 0 ? -1 : 0;
}

int
free_port_bsdradio(void) {
	return radio_device_release(fd, radio_device_1);
}

void
set_frequency_bsdradio(u_int16_t frequency) {
	if (do_rinfo(fd, GET_INFO, &ri) < 0)
		return;
	ri.freq = frequency * 10;
	do_rinfo(fd, SET_INFO, &ri);
}

void
set_volume_bsdradio(int vol) {
	if (do_rinfo(fd, GET_INFO, &ri) < 0)
		return;
	ri.volume = vol;
	ri.mute = vol ? 0 : 1;
	do_rinfo(fd, SET_INFO, &ri);
}

int
state_bsdradio(void) {
	int result = 0;

	if (do_rinfo(fd, GET_INFO, &ri) < 0)
		return 0;

	if (ri.caps & RADIO_CAPS_DETECT_SIGNAL)
		if (ri.info & RADIO_INFO_SIGNAL)
			result |= DRV_INFO_SIGNAL;
	if (ri.caps & RADIO_CAPS_DETECT_STEREO)
		if (ri.info & RADIO_INFO_STEREO)
			result |= DRV_INFO_STEREO;

	return result;
}

void
mono_bsdradio(void) {
	ri.stereo = 0;
	do_rinfo(fd, SET_INFO, &ri);
}

u_int16_t
get_frequency_bsdradio(void) {
	return do_rinfo(fd, GET_INFO, &ri) < 0 ? -1 : ri.freq / 10;
}

int
get_volume_bsdradio(void) {
	return do_rinfo(fd, GET_INFO, &ri) < 0 ? -1 : ri.volume;
}

static int
do_rinfo(int rfd, int dir, struct radio_info *r) {
	if (ioctl(rfd, dir == SET_INFO ? RIOCSINFO : RIOCGINFO, r) < 0) {
		print_w(dir == SET_INFO ? riocsinfo : riocginfo);
		return -1;
	}
	return 0;
}
#endif /* BSDRADIO */
