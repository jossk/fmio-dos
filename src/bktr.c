/*
 * Copyright (c) 2000 - 2002 Vladimir Popov <jumbo@narod.ru>.
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
 * $Id: bktr.c,v 1.38 2002/01/18 10:55:47 pva Exp $
 *  Driver for cards support for which is compiled into kernel
 */

#include "ostypes.h"

#include <err.h>
#include <fcntl.h>	/* O_RDONLY */

#include "radio_drv.h"

#ifdef USE_BKTR
#define BKTR_CAPS		DRV_INFO_KNOWS_FREQ | DRV_INFO_KNOWS_VOLU | \
				DRV_INFO_MONOSTEREO | DRV_INFO_GETS_SIGNAL | \
				DRV_INFO_GETS_STEREO

int get_port_bktr(u_int32_t);
int free_port_bktr(void);
u_int32_t info_port_bktr(void);
int find_card_bktr(void);
void set_freq_bktr(u_int16_t);
u_int16_t get_freq_bktr(void);
void set_vol_bktr(int);
int get_vol_bktr(void);
void mono_bktr(void);
int state_bktr(void);

struct tuner_drv_t bktr_drv = {
#ifdef BSDBKTR
	"Brooktree BT848/BT878 Driver", "bktr", NULL, 0,
	BKTR_CAPS | DRV_INFO_VOLUME(1),
#elif defined linux
	"Video4Linux Driver", "v4l", NULL, 0,
	BKTR_CAPS | DRV_INFO_VOL_SEPARATE | DRV_INFO_VOLUME(100),
#endif
	get_port_bktr, free_port_bktr, info_port_bktr, find_card_bktr,
	set_freq_bktr, get_freq_bktr, NULL,
	set_vol_bktr, get_vol_bktr, mono_bktr, state_bktr
};

static int fd = -1;

#ifdef linux
static double get_freq_fact(int);
static void linux_mute(void);
static int tuner_ord = 0;
static int stereo = 1; /* Use stereo by default */
#elif defined BSDBKTR
#define BKTR_STEREO	0
#define BKTR_MONO	1
static int bktr_setstereo(int);
#endif

extern const char *tuner_device_1;
extern const char *tuner_device_2;

/******************************************************************/

struct tuner_drv_t *
export_bktr(void) {
	return &bktr_drv;
}

int
get_port_bktr(u_int32_t port) {
	fd = radio_device_get(tuner_device_1, tuner_device_2, O_RDONLY);
	return fd < 0 ? -1 : 0;
}

int
find_card_bktr(void) {
#ifdef BSDBKTR
	int intern = 3;
#elif defined linux
	struct video_tuner t;
#endif /* __FreeBSD__ || __OpenBSD__ || __NetBSD__ */

	/* Check for working driver */
#ifdef BSDBKTR
	if (ioctl(fd, TVTUNER_SETCHNL, &intern) < 0) {
		warn("TVTUNER_SETCHNL");
		return -1;
	}
	if (ioctl(fd, TVTUNER_SETTYPE, &intern) < 0) {
		warn("TVTUNER_SETTYPE");
		return -1;
	}
	intern = AUDIO_INTERN;
	if (ioctl(fd, BT848_SAUDIO, &intern) < 0) {
		warn("set BT848_SAUDIO to AUDIO_INTERN");
		return -1;
	}
	if (bktr_setstereo(BKTR_STEREO) < 0) {
#elif defined linux
	/* FIXME: it can be not the first tuner */
	t.tuner = tuner_ord;

	if (ioctl(fd, VIDIOCGTUNER, &t) < 0) {
		warn("VIDIOCGTUNER");
#endif
		return -1;
	}
	return 0;
}

int
free_port_bktr(void) {
	return radio_device_release(fd, tuner_device_1);
}

u_int32_t
info_port_bktr(void) {
	return 0ul;
}

void
set_freq_bktr(u_int16_t frequency) {
#ifdef BSDBKTR
	unsigned long freq = frequency;

	if (ioctl(fd, RADIO_SETFREQ, &freq) < 0)
#elif defined linux
	unsigned long freq = (unsigned long)(frequency * get_freq_fact(fd));

	if (ioctl(fd, VIDIOCSFREQ, &freq) < 0)
#endif
		warn("set frequency error");
}

void
set_vol_bktr(int v) {
#ifdef BSDBKTR
	int intern = v ? AUDIO_UNMUTE : AUDIO_MUTE;

	if (ioctl(fd, BT848_SAUDIO, &intern) < 0)
		warn("%s error", v ? "unmute" : "mute");
#elif defined linux
	struct video_audio va;

	if (v > 10)
		v = 10;
	if (v < 0)
		v = 0;

	if (v == 0) {
		linux_mute();
		return;
	}

	va.flags = VIDEO_AUDIO_VOLUME;
	if (stereo)
		va.mode = VIDEO_SOUND_STEREO;
	else
		va.mode = VIDEO_SOUND_MONO;
	va.audio = 0;
	va.volume = v * (65535 / 10);

	if (ioctl(fd, VIDIOCSAUDIO, &va) < 0)
		warn("set volume error");
#endif /* BSDBKTR */
}

int
state_bktr(void) {
#ifdef BSDBKTR
	int ret = 0;
	int ss;

	if (ioctl(fd, TVTUNER_GETSTATUS, &ss) < 0) {
		warn("TVTUNER_GETSTATUS");
		return 0;
	}

	ret |= ss & 0x10 ? DRV_INFO_STEREO : 0;
	ret |= ss & 0x07 ? DRV_INFO_SIGNAL : 0;

	return ret;
#elif defined linux
	struct video_tuner t;

	t.tuner = tuner_ord;

	if (ioctl(fd, VIDIOCGTUNER, &t) < 0) {
		warn("VIDIOCGTUNER");
		return 0;
	}

	if (t.flags & VIDEO_TUNER_STEREO_ON)
		return (DRV_INFO_SIGNAL | DRV_INFO_STEREO); /* STEREO */
	if (t.signal > 49061) return DRV_INFO_STEREO; /* almost stereo */
	if (t.signal > 32678) return DRV_INFO_SIGNAL; /* mono */

	return 0;
#endif
}

void
mono_bktr(void) {
#ifdef BSDBKTR
	if (bktr_setstereo(BKTR_MONO) < 0)
#elif defined linux
	struct video_audio va;

	va.audio = 0;
	va.mode = VIDEO_SOUND_MONO;
	stereo = 0;

	if (ioctl(fd, VIDIOCSAUDIO, &va) < 0)
#endif
		warn("set mono error");
}

u_int16_t
get_freq_bktr(void) {
	unsigned long freq;
#ifdef BSDBKTR
	if (ioctl(fd, RADIO_GETFREQ, &freq) < 0)
		warn("RADIO_GETFREQ");

	return (u_int16_t)freq;
#else
	float fact = get_freq_fact(fd);

	if (ioctl(fd, VIDIOCGFREQ, &freq) < 0)
		warn("VIDIOCGFREQ");

	if (fact == 160.)
		return (u_int16_t)(1 + freq/160);

	return (u_int16_t)(freq/fact);
#endif
}

int
get_vol_bktr(void) {
#ifdef BSDBKTR
	int intern = 0;

	if (ioctl(fd, BT848_GAUDIO, &intern) < 0)
		warn("BT848_GAUDIO");

	return intern & 2 ? 1 : 0;
#else
	struct video_audio va;

	va.audio = 0;

	if (ioctl(fd, VIDIOCGAUDIO, &va) < 0)
		warn("VIDIOCGAUDIO");

	return 1 + va.volume * 10 / 65535;
#endif
}

#ifdef linux
static double
get_freq_fact(int fd) {
	struct video_tuner t;
	t.tuner = tuner_ord;
	if (ioctl(fd, VIDIOCGTUNER, &t) == -1 || (t.flags & VIDEO_TUNER_LOW) == 0)
		return .16;

	return 160.;
}

static void
linux_mute(void) {
	struct video_audio va;

	va.audio = 0;
	va.flags = VIDEO_AUDIO_MUTE;
	va.volume = 0;

	if (ioctl(fd, VIDIOCSAUDIO, &va) < 0)
		warn("mute error");
}
#elif defined BSDBKTR
static int
bktr_setstereo(int st) {
	int intern = 0;

	if (ioctl(fd, RADIO_GETMODE, &intern) < 0) {
		warn("RADIO_GETMODE");
		return -1;
	}

	if (st == BKTR_STEREO)
		intern &= ~RADIO_MONO;
	else
		intern |= RADIO_MONO;

	if (ioctl(fd, RADIO_SETMODE, &intern) < 0) {
		warn("RADIO_SETMODE");
		return -1;
	}

	return 0;
}
#endif /* linux */
#endif /* USE_BKTR */
