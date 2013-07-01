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
 * $Id: mixer.c,v 1.22 2002/01/07 14:33:59 pva Exp $
 * mixer.c -- implementation of mixer management routines
 */

#include "ostypes.h"
#include "mixer.h"

#ifndef NOMIXER

#include <sys/ioctl.h>
#if defined (__OpenBSD__) || defined (__NetBSD__)
#include <sys/audioio.h>
#elif defined (linux) || defined (__FreeBSD__)
#include <sys/soundcard.h>
#endif /* __OpenBSD__ || __NetBSD__ */

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef SOUND_MIXER_FIRST
#define SOUND_MIXER_FIRST	0
#endif /* SOUND_MIXER_FIRST */

static int mfd; /* mixer file descriptor */

#if defined (__OpenBSD__) || defined (__NetBSD__)
static mixer_ctrl_t value;
#elif defined (__FreeBSD__) || defined (linux)
static int num_channels;
#endif /* __OpenBSD__ || __NetBSD__ */

static unsigned int volume;

static int
mixer_open(void) {
	char *mixdev = NULL;

	mixdev = getenv("MIXERDEVICE");
	if (mixdev == NULL)
		mixdev = "/dev/mixer";

	mfd = open(mixdev, O_RDWR);
	if (mfd < 0) {
		warn("%s open error", mixdev);
		return -1;
	}

	return 0;
}

static int
mixer_find_master(void) {
#if defined (__OpenBSD__) || (__NetBSD__)
	int ndev;
	mixer_devinfo_t devinfo;

	for (ndev = 0 ; ; ndev++) {
		devinfo.index = ndev;
		if (ioctl(mfd, AUDIO_MIXER_DEVINFO, &devinfo) < 0)
			break;
		/* Look for "outputs.master" */
		if (strncmp(devinfo.label.name, AudioNmaster, 6) == 0) {
			value.dev = ndev;
			value.type = devinfo.type;
			return 0;
		}
	}

	if (ndev == 0)
		warnx("No mixer devices configured");
	else
		warnx("`%s.%s' not found", AudioCoutputs, AudioNmaster);

	return -1;
#elif defined (__FreeBSD__) || defined (linux)
	int devmask;

	if (ioctl(mfd, SOUND_MIXER_READ_DEVMASK, &devmask) < 0) {
		warn("No mixer devices configured");
		return -1;
	}

	if (!(devmask & SOUND_MASK_VOLUME)) {
		warnx("No volume control");
		return -1;
	}

	return 0;
#endif /* __OpenBSD__ || __NetBSD__ */
}

static int
mixer_read_vol(char *q, unsigned int *vol, int channels) {
	int v, v0, v1;
	unsigned char left = *vol & 0xFF;
	unsigned char right = (*vol>>8) & 0xFF;
	char *s;

	if (channels == 1) {
		if (sscanf(q, "%d", &v) == 1) {
			switch (*q) {
				case '+':
				case '-':
					left += v;
					right = left;
					break;
				default:
					left = right = v;
					break;
			}
		} else {
			warnx("Bad number `%s'", q);
			return -1;
		}
	} else {
		if (sscanf(q, "%d,%d", &v0, &v1) == 2) {
			switch (*q) {
				case '+':
				case '-':
					left += v0;
					break;
				default:
					left = v0;
					break;
			}
			s = strchr(q, ',') + 1;
			switch (*s) {
				case '+':
				case '-':
					right += v1;
					break;
				default:
					right = v1;
					break;
			}
		} else if (sscanf(q, "%d", &v) == 1) {
			switch (*q) {
				case '+':
				case '-':
					left += v;
					right += v;
					break;
				default:
					left = v;
					right = v;
					break;
			}
		} else {
			warnx("Bad numbers `%s'", q);
			return -1;
		}
	}

	*vol = (right<<8) | left;
	return 0;
}

static int
mixer_write_vol(unsigned int vol) {
#if defined (__FreeBSD__) || defined (linux)
	if (ioctl(mfd, SOUND_MIXER_WRITE_VOLUME, &vol) < 0) {
#elif defined (__OpenBSD__) || defined (__NetBSD__)
	/* AUDIO_MIXER_LEVEL_LEFT == AUDIO_MIXER_LEVEL_MONO */
	value.un.value.level[AUDIO_MIXER_LEVEL_LEFT] = vol & 0xFF;
	value.un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = (vol>>8) & 0xFF;

	if (ioctl(mfd, AUDIO_MIXER_WRITE, &value) < 0) {
#endif /* __FreeBSD__ || linux */
		warn("Failed to write new volume");
		return -1;
	}

	return 0;
}

int
mixer_init(void) {
#if defined (__FreeBSD__) || defined (linux)
	unsigned int vol;
#endif /* __FreeBSD__ || linux */

	if (mixer_open() < 0)
		return -1;

	if (mixer_find_master() < 0)
		return -1;

#if defined (__OpenBSD__) || defined (__NetBSD__)
	/* Check for stereo/mono */
	value.un.value.num_channels = 2;
	if (ioctl(mfd, AUDIO_MIXER_READ, &value) < 0) {
		value.un.value.num_channels = 1;
		if (ioctl(mfd, AUDIO_MIXER_READ, &value) < 0) {
			warn("AUDIO_MIXER_READ");
			return -1;
		}
	}

	/* Save initial values */
	switch (value.un.value.num_channels) {
		case 2:
			volume = value.un.value.level[AUDIO_MIXER_LEVEL_LEFT];
			volume |= value.un.value.level[AUDIO_MIXER_LEVEL_RIGHT] << 8;
			break;
		case 1:
			volume = value.un.value.level[AUDIO_MIXER_LEVEL_MONO] << 8;
			volume |= value.un.value.level[AUDIO_MIXER_LEVEL_MONO];
			break;
		default:
			warnx("Don't now how to handle %d channels",
					value.un.value.num_channels);
			return -1;
	}
#elif defined (__FreeBSD__) || defined (linux)
	if (ioctl(mfd, SOUND_MIXER_READ_STEREODEVS, &vol) < 0)
		warn("No stereo capable devices");

	if (vol & SOUND_MASK_VOLUME)
		num_channels = 2;
	else
		num_channels = 1;

	if (ioctl(mfd, SOUND_MIXER_READ_VOLUME, &vol) < 0) {
		warn("Volume read error");
		volume = 0;
		return -1;
	}

	volume = vol;
#endif /* __OpenBSD__ || __NetBSD__ */

	/* Mute the thing */
	if (mixer_write_vol(0) < 0)
		return -1;

	return 0;
}

int
mixer_cleanup(void) {

	/* Avoid clicks */
	usleep(20000);

	/* Set new or restore old volume */
	mixer_write_vol(volume);
	
	if (close(mfd) < 0)
		return -1;

	return 0;
}

int
mixer_set_volume(char *vol) {
#if defined (__OpenBSD__) || defined (__NetBSD__)
	return mixer_read_vol(vol, &volume, value.un.value.num_channels);
#elif defined (__FreeBSD__) || defined (linux)
	return mixer_read_vol(vol, &volume, num_channels);
#endif /* __OpenBSD__ || __NetBSD__ */
}

/* FIXME: merge this monster with everything else */
int
mixer_update_device(char *vol) {
	int ndev;
#if defined (__OpenBSD__) || defined (__NetBSD__)
	int found = 0;
	mixer_ctrl_t dev_ctrl;
	mixer_devinfo_t devinfo;
#elif defined (__FreeBSD__) || defined (linux)
	int chan;
	int devmask;
	char *dd[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_NAMES;
#endif /* __OpenBSD__ || __NetBSD__ */
	char dev[16], val[16];
	unsigned int devlen, dev_volume;

	devlen = strcspn(vol, "=");
	dev_volume = strlen(vol)-devlen-1;
	if (dev_volume < 1) {
		warnx("Bad volume %s, see the man page", vol);
		return -1;
	}

	strncpy(dev, vol, devlen);
	dev[devlen] = 0;
	strncpy(val, &vol[devlen+1], dev_volume);
	val[dev_volume] = 0;

#if defined (__OpenBSD__) || (__NetBSD__)
	for (ndev = 0 ; ; ndev++) {
		devinfo.index = ndev;
		if (ioctl(mfd, AUDIO_MIXER_DEVINFO, &devinfo) < 0)
			break;
		/* Look for the device */
		if (strncmp(devinfo.label.name, dev, devlen) == 0) {
			dev_ctrl.dev = ndev;
			dev_ctrl.type = devinfo.type;
			found = 1;
			break;
		}
	}

	if (ndev == 0) {
		warnx("No mixer devices configured");
		return -1;
	} else if (!found) {
		warnx("`%s' not found", dev);
		return -1;
	}

	/* Check for stereo/mono */
	dev_ctrl.un.value.num_channels = 2;
	if (ioctl(mfd, AUDIO_MIXER_READ, &dev_ctrl) < 0) {
		dev_ctrl.un.value.num_channels = 1;
		if (ioctl(mfd, AUDIO_MIXER_READ, &dev_ctrl) < 0) {
			warn("AUDIO_MIXER_READ");
			return -1;
		}
	}

	/* Save initial values */
	switch (dev_ctrl.un.value.num_channels) {
		case 2:
			dev_volume = dev_ctrl.un.value.level[AUDIO_MIXER_LEVEL_LEFT];
			dev_volume |= dev_ctrl.un.value.level[AUDIO_MIXER_LEVEL_RIGHT] << 8;
			break;
		case 1:
			dev_volume = dev_ctrl.un.value.level[AUDIO_MIXER_LEVEL_MONO] << 8;
			dev_volume |= dev_ctrl.un.value.level[AUDIO_MIXER_LEVEL_MONO];
			break;
		default:
			warnx("Don't now how to handle %d channels",
					dev_ctrl.un.value.num_channels);
			return -1;
	}

	mixer_read_vol(val, &dev_volume, dev_ctrl.un.value.num_channels);

	/* AUDIO_MIXER_LEVEL_LEFT == AUDIO_MIXER_LEVEL_MONO */
	dev_ctrl.un.value.level[AUDIO_MIXER_LEVEL_LEFT] = dev_volume & 0xFF;
	dev_ctrl.un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = (dev_volume>>8) & 0xFF;

	if (ioctl(mfd, AUDIO_MIXER_WRITE, &dev_ctrl) < 0) {
#elif defined (__FreeBSD__) || defined (linux)
	if (ioctl(mfd, SOUND_MIXER_READ_DEVMASK, &devmask) < 0) {
		warn("No mixer devices configured");
		return -1;
	}

	for (ndev = SOUND_MIXER_FIRST; ndev < SOUND_MIXER_NRDEVICES; ndev++)
		if (strncasecmp(dd[ndev], dev, devlen) == 0)
			break;

	if (ndev == SOUND_MIXER_NRDEVICES) {
		warnx("`%s' not found", dev);
		return -1;
	}

	if (!(devmask & (1 << ndev))) {
		warnx("No volume control for `%s'", dd[ndev]);
		return -1;
	}

	if (ioctl(mfd, SOUND_MIXER_READ_STEREODEVS, &dev_volume) < 0)
		warn("No stereo capable devices");

	if (dev_volume & (1 << ndev))
		chan = 2;
	else
		chan = 1;

	if (ioctl(mfd, MIXER_READ(ndev), &dev_volume) < 0) {
		warn("Volume read error");
		dev_volume = 0;
		return -1;
	}

	mixer_read_vol(val, &dev_volume, chan);

	if (ioctl(mfd, MIXER_WRITE(ndev), &dev_volume) < 0) {
#endif /* __OpenBSD__ || __NetBSD__ */
		warn("Volume write error");
		return -1;
	}

	return 0;
}
#endif /* !NOMIXER */
