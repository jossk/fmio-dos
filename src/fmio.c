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
 * $Id: fmio.c,v 1.71 2002/01/12 09:48:46 pva Exp $
 * core file for fmio utility
 */

#ifndef __DOS__
#include <err.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __DOS__
#include <stdint.h>
typedef uint32_t	u_int32_t;
typedef uint16_t	u_int16_t;
typedef uint8_t		u_int8_t;
#endif /* __DOS__ */

#ifdef __QNXNTO__
#include <inttypes.h>
typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
#define NOMIXER
#endif /* __QNXNTO__ */

#include "radio.h"

#include "config.h"

/* Actions */
/* major */
#define NONE	0x0000
#define SCAN	0x0100
#define DETE	0x0200
#define SRCH	0x0400
/* minor */
#define MINOR	0x00FF
#define STAT	0x0001
#define TUNE	0x0002
#define VOLU	0x0004
#define MONO	0x0008
#define INFO	0x0010

char *pn = NULL;

void die(int);
void usage(void);
void invalid_driver_error(char *);
int gouser(void);
int goroot(void);

int
main(int argc, char **argv) {
	int optchar;
	int search = 0;
	char *drv = NULL;
	u_int16_t freq = DEF_FREQ;
	int volu = 0;
	u_int16_t action = NONE;
	u_int16_t lower = 0, higher = 0;
	u_int32_t cycle = 1;
#ifndef NOMIXER
	int mixer = 0;
	char *master_volume = NULL;
	char *other_volume = NULL;
#endif /* !NOMIXER */

	if (gouser() < 0)
		return -1;

	pn = strrchr(argv[0], '/');
	if (pn == NULL)
		pn = argv[0];
	else
		pn++;

	radio_init();

	/* 
	 * Call radio_drv_init() before usage(),
	 * or default driver will be: NULL, 0x0
	 */
	drv = getenv("FMTUNER");
	if (drv == NULL || *drv == '\0')
		drv = DEF_DRV;

	if (radio_drv_init(drv) == ERADIO_INVL)
		invalid_driver_error(drv);

	if (argc < 2)
		usage();

	/* Argh... options */
#ifndef NOMIXER
	while ((optchar = getopt(argc, argv, "c:Dd:f:h:il:mSsv:W:X:x:")) != -1) {
#else
	while ((optchar = getopt(argc, argv, "c:Dd:f:h:il:mSsv:W:X:x:")) != -1) {
#endif /* !NOMIXER */
		switch (optchar) {
		case 'c': /* number of probes for each scanned frequency */
			if ((cycle = strtol(optarg, (char **)NULL, 10)) == 0)
				cycle = 1;
			break;
		case 'D':
			action = DETE;
			break;
		case 'd':
			radio_drv_free();
			if (radio_drv_init(optarg) == ERADIO_INVL)
				invalid_driver_error(optarg);
			break;
		case 'f':
			freq = 100.0 * strtod(optarg, (char **)NULL);
			if (freq == 0)
				freq = DEF_FREQ;
			action |= TUNE;
			break;
		case 'h':
			higher = atof(optarg) * 100;
			break;
		case 'i':
			action |= INFO;
			break;
		case 'l':
			lower = atof(optarg) * 100;
			break;
		case 'm':
			action |= MONO;
			break;
		case 'S':
			action = SCAN;
			break;
		case 's':
			action |= STAT;
			break;
		case 'v':
			volu = strtoul(optarg, (char **)NULL, 10);
			action |= VOLU;
			break;
		case 'W':
			action = SRCH;
			search = atof(optarg) * 100;
			break;
#ifndef NOMIXER
		case 'X': /* set outputs.master */
			master_volume = optarg;
			break;
		case 'x':
			other_volume = optarg;
			break;
#endif /* !NOMIXER */
		default:
			usage();
		}
	}

	/* Minor actions have more priority */
	if (action & MINOR) action &= MINOR;

#if 0
	/* Drop privs for drivers that don't need root */
	if ((action & ~MINOR) != DETE)
		if (!radio_info_root())
			setuid(getuid());
#endif

	/* Enabling communication with the radio port */
	if (radio_info_root())
		if (goroot() < 0)
			die(1);

	if (radio_get_port() < 0)
		die(1);

	/* Test for card presense */
	if (radio_test_port() != 1) {
		fprintf(stderr, "%s: card not found: ", pn);
		radio_info_show(stderr, radio_info_name(), radio_info_port());
		radio_free_port();
		die(1);
	}

	if (radio_info_root())
		if (gouser() < 0)
			die(1);

	/*
	 * During time consuming actions fmio can get
	 * some signal. Define action for such emergency.
	 * Though it's more suitable to define these under SCAN or DETE,
	 * it won't hurt if they'll be here.
	 */
	signal(SIGINT , die);
#ifndef __DOS__
	signal(SIGHUP , die);
#endif
	signal(SIGTERM, die);

	switch (action & ~MINOR) {
	case NONE:
#ifndef NOMIXER
		mixer = radio_mixer_init() < 0 ? 0 : 1;
#endif /* !NOMIXER */
		if (radio_info_root())
			if (goroot() < 0)
				die(1);
		if (action & MONO)
			radio_set_mono();
		switch (radio_info_policy()) {
		case 0:
			if (action & TUNE)
				radio_set_volume(action & VOLU ? volu : 1);
			break;
		case 1:
			if (action & TUNE)
				radio_set_volume(action & VOLU ?
						volu : radio_info_maxvol());
			break;
		}
		if (action & TUNE)
			radio_set_freq(freq);
		if (action & VOLU)
			radio_set_volume(volu);
		if (action & STAT) {
			int st = radio_info_stereo();
			int si = radio_info_signal();
			if (st != ERADIO_INVL)
				printf("%s", st ? "stereo" : "mono");
			if (st != ERADIO_INVL && si != ERADIO_INVL)
				printf(" : ");
			if (si != ERADIO_INVL)
				printf("%s", si ? "signal" : "noise");
			if (st != ERADIO_INVL || si != ERADIO_INVL)
				printf("\n");
		}
		if (action & INFO) {
			u_int16_t f = radio_info_freq();
			int v = radio_info_volume();

			printf("Driver: ");
			radio_info_show(stdout, radio_info_name(), radio_info_port());
			if (f)
				printf("Frequency: %.2f MHz\n", (float) f / 100);
			if (v)
				printf("Volume: %u\n", v);
			v = radio_info_signal();
			if (v != ERADIO_INVL)
				printf("Signal: %s\n", v ? "on" : "off");
			v = radio_info_stereo();
			if (v != ERADIO_INVL)
				printf("Stereo: %s\n", v ? "on" : "off");
		}
		if (radio_info_root())
			gouser();
#ifndef NOMIXER
		if (mixer)
			if (other_volume)
				radio_mixer_update_volume(other_volume);

		/*
		 * Now outputs.master=0,0
		 * Update saved values
		 */
		if (mixer) {
			if (master_volume)
				radio_mixer_set_volume(master_volume);
			radio_mixer_cleanup();
		}
#endif /* !NOMIXER */
		break;
	case DETE:
		if (goroot() < 0)
			die(1);
		radio_detect();
		if (gouser() < 0)
			die(1);
		break;
	case SCAN:
		if (radio_info_root())
			if (goroot() < 0)
				die(1);
		radio_scan(lower, higher, cycle);
		if (radio_info_root())
			if (gouser() < 0)
				die(1);
		break;
	case SRCH:
#ifndef NOMIXER
		mixer = radio_mixer_init() < 0 ? 0 : 1;
#endif /* !NOMIXER */
		if (radio_info_root())
			if (goroot() < 0)
				die(1);
		if (search < 0)
			freq = radio_search(0, -1 * search);
		else
			freq = radio_search(1, search);
		if (radio_info_root())
			if (gouser() < 0)
				die(1);
		if (freq)
			printf("%s: %.2f MHz\n", pn, (float)freq / 100);
#ifndef NOMIXER
		if (mixer)
			radio_mixer_cleanup();
#endif /* !NOMIXER */
		break;
	default:
		break;
	}

	if (radio_info_root())
		if (goroot() < 0)
			die(1);
	radio_free_port();
	if (radio_info_root())
		gouser();
	radio_drv_free();
	radio_cleanup();

	return 0;
}

/*
 * Dump the usage string to screen
 */
void
usage(void) {
	const char usage_string[] =
#ifdef NOMIXER
		"Usage:  %s [-d driver] [-f frequency] [-i] [-m] [-s] [-v volume]\n"
#else
		"Usage:  %s [-d drv] [-f freq] [-i] [-m] [-s] [-v vol] [-X vol] [-x vol]\n"
#endif /* NOMIXER */
		"\t%s [-d driver] -S [-l begin] [-h end] [-c count]\n"
		"\t%s [-d driver] -W frequency\n"
		"\t%s -D - detect driver\n\n"

		"\t-f frequency in Mhz, -f 98.0 for example\n"
		"\t-i information\n"
		"\t-m mono\n"
		"\t-s stat\n"
		"\t-v volume, -v 0 set tuner off\n"
		"\t-S scan -l start frequency, -h end frequency\n"
		"\t-c number of probes for each scanned frequency\n"
		"\t-W search\n"
	;
	printf("%s version %s\n", pn, VERSION);
	printf("Default driver: ");
	radio_info_show(stdout, radio_info_name(), radio_info_port());
	printf(usage_string, pn, pn, pn, pn);

	die(0);
}

void
die(int sig) {
	radio_drv_free();
	radio_cleanup();
	exit(sig);
}

/*
 * Complain about invalid driver and init with default driver
 */
void
invalid_driver_error(char *invl_drv) {
#ifdef __DOS__
	printf("Invalid driver `%s', using default `%s'", invl_drv, DEF_DRV);
#else
	warnx("Invalid driver `%s', using default `%s'", invl_drv, DEF_DRV);
#endif
	radio_drv_free();
	radio_drv_init(DEF_DRV);
}

int
gouser(void) {
#ifndef __DOS__
	if (seteuid(getuid()) < 0) {
		warn("set effective user %d privs error", getuid());
		return -1;
	}
#endif
	return 0;
}

int
goroot(void) {
#ifndef __DOS__
	if (seteuid(0) < 0) {
		warn("set effective root privs error");
		return -1;
	}
#endif
	return 0;
}
