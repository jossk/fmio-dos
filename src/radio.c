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
 * $Id: radio.c,v 1.79 2002/01/18 10:55:50 pva Exp $
 *
 * radio.c -- implementation of interface to libradio
 *
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ostypes.h"

#include "export.h"
#ifndef NOMIXER
#include "mixer.h"
#endif /* !NOMIXER */
#include "radio.h"
#include "radio_drv.h"

#define MMAX(a,b)	((a) >= (b) ? (a) : (b))

EXPORT_FUNC export_db[] = {
	export_aztech,		/* Aztech/PackardBell */
#ifdef USE_BKTR
	export_bktr,		/* Brooktree BT848/BT878 or Video4Linux */
#endif /* USE_BKTR */
	export_bmc,		/* BMC fcc-id HMA00-0076 */
#ifdef BSDRADIO
	export_bsdradio,	/* OpenBSD & NetBSD FM Radio */
#endif /* BSDRADIO */
	export_er,		/* EcoRadio (JTR-9401) */
	export_gti,		/* Gemtek ISA */
	export_gtp,		/* Gemtek PCI */
	export_mr,		/* Guillemot MaxiRadio FM2000 */
	export_rt,		/* AIMS Lab Radiotrack */
	export_rtii,		/* AIMS Lab Radiotrack II */
	export_sf16fmd2,	/* SoundForte Legacy 128 SF16-FMD2 */
	export_sf16fmr,		/* SoundForte RadioLink SF16-FMR */
	export_sf16fmr2,	/* SoundForte RadioLink SF16-FMR2 */
	export_sf64pce2,	/* SoundForte Awesome 64R SF64-PCE2 */
	export_sf64pcr,		/* SoundForte RadioLink SF64-PCR */
	export_sf256pcpr,	/* SoundForte Quad X-treme SF256-PCP-R */
	export_sf256pcs,	/* SoundForte Theatre X-treme 5.1 SF256-PCS-R */
	export_sfi,		/* SoundForte RadioX SF16-FMI */
	export_sp,		/* Spase PC-Radio */
	export_svg,		/* Sound Vision 16 Gold */
	export_tr,		/* Trust */
	export_tt,		/* Terratec */
#ifdef BSDBKTR
	export_xtreme,		/* AIMS Lab Highway Xtreme */
#endif /* BSDBKTR */
	export_zx		/* Zoltrix RadioPlus */
};

struct tuner_drv_t **drv_db;

extern char *pn;
static int driver = ERADIO_INVL;
static int variant = ERADIO_INVL;
static int complain = 1;

int check_drv(struct tuner_drv_t *, char *);
int test_port(struct tuner_drv_t *, u_int32_t);
void draw_stick(int);
void range(u_int16_t, u_int16_t *, u_int16_t *, u_int16_t);
u_int16_t search_up_generic(struct tuner_drv_t *, u_int16_t);
u_int16_t search_down_generic(struct tuner_drv_t *, u_int16_t);

/*
 * Create driver database
 */
void
radio_init(void) {
	int i, drivers = sizeof(export_db) / sizeof(export_db[0]);
	
	drv_db = malloc(drivers * sizeof(struct tuner_drv_t *));
	if (drv_db == NULL)
		return;

	/* Initialize the driver database */
	for (i = 0; i < drivers; i++)
		drv_db[i] = export_db[i]();
}

int
radio_drv_init(char *name) {
	int i, drivers = sizeof(export_db) / sizeof(export_db[0]);

	for (i = 0; i < drivers; i++) {
		variant = check_drv(drv_db[i], name);
		if (variant != ERADIO_INVL) {
			driver = i;
			break;
		}
	}

	return driver == ERADIO_INVL ? ERADIO_INVL : 0;
}

int
radio_drv_free(void) {
	driver = ERADIO_INVL;
	variant = ERADIO_INVL;
	return 0;
}

int
radio_cleanup(void) {
	free(drv_db);
	return 0;
}

int
radio_get_port(void) {
	struct tuner_drv_t *drv;
	u_int32_t port;

	if (driver == ERADIO_INVL)
		return ERADIO_INVL;

	drv = drv_db[driver];

	port = drv->ports == NULL ? 0 : drv->ports[variant];

	return drv->get_port(port);
}

int
radio_free_port(void) {
	return driver == ERADIO_INVL ? 0 : drv_db[driver]->free_port();
}

int
radio_test_port(void) {
	if (driver == ERADIO_INVL)
		return ERADIO_INVL;

	if (drv_db[driver]->find_card == NULL)
		return 1;

	return drv_db[driver]->find_card() == 0 ? 1 : 0;
}

void
radio_set_freq(u_int16_t freq) {
	if (driver != ERADIO_INVL)
		if (drv_db[driver]->set_freq != NULL)
			drv_db[driver]->set_freq(freq);
}

void
radio_set_volume(int vol) {
	if (driver != ERADIO_INVL)
		if (drv_db[driver]->set_volu != NULL)
			drv_db[driver]->set_volu(vol);
}

void
radio_set_mono(void) {
	if (driver == ERADIO_INVL)
		return;

	if (drv_db[driver]->set_mono != NULL)
		drv_db[driver]->set_mono();
}

int
radio_info_volume(void) {
	if (driver == ERADIO_INVL)
		return ERADIO_INVL;

	return drv_db[driver]->get_volu == NULL ?
		 0 : drv_db[driver]->get_volu();
}

int
radio_info_signal(void) {
	int ret = ERADIO_INVL;

	if (driver == ERADIO_INVL)
		return ret;

	if (drv_db[driver]->caps & DRV_INFO_GETS_SIGNAL)
		if (drv_db[driver]->get_state != NULL)
			ret = drv_db[driver]->get_state() & DRV_INFO_SIGNAL ?
				1 : 0;

	return ret;
}

int
radio_info_stereo(void) {
	int ret = ERADIO_INVL;

	if (driver == ERADIO_INVL)
		return ret;

	if (drv_db[driver]->caps & DRV_INFO_GETS_STEREO)
		if (drv_db[driver]->get_state != NULL)
			ret = drv_db[driver]->get_state() & DRV_INFO_STEREO ?
				1 : 0;

	return ret;
}

int
radio_info_root(void) {
	if (driver == ERADIO_INVL)
		return ERADIO_INVL;

	return drv_db[driver]->caps & DRV_INFO_NEEDS_ROOT ? 1 : 0;
}

u_int32_t
radio_info_port(void) {
	if (driver == ERADIO_INVL)
		return 0ul;

	return drv_db[driver]->info_port == NULL ?
		0ul : drv_db[driver]->info_port();
}

u_int16_t
radio_info_freq(void) {
	if (driver == ERADIO_INVL)
		return ERADIO_INVL;

	return drv_db[driver]->get_freq == NULL ?
		0ul : drv_db[driver]->get_freq();
}

char *
radio_info_name(void) {
	return driver == ERADIO_INVL ? NULL : drv_db[driver]->name;
}

u_int8_t
radio_info_maxvol(void) {
	u_int8_t ret;

	if (driver == ERADIO_INVL)
		return 0;

	ret = DRV_INFO_VOLUME(drv_db[driver]->caps);

	return ret == 0 ? 1 : ret;
}

int
radio_info_policy(void) {
	int ret;

	if (driver == ERADIO_INVL)
		return ERADIO_INVL;

	ret = drv_db[driver]->caps & DRV_INFO_MAXVOL_POLICY ? 1 : 0;
	ret |= drv_db[driver]->caps & DRV_INFO_VOL_SEPARATE ? 2 : 0;

	return ret;
}

void
radio_info_show(FILE *out, char *name, u_int32_t port) {
	fprintf(out, "%s", name);
	if (port)
		fprintf(out, ", port 0x%x", port);
	fprintf(out, "\n");
}

void
radio_detect(void) {
	int i, vars, drivers;
	struct tuner_drv_t *drv;
	u_int32_t port;

	puts("Probing ports, please wait...");

	drivers = sizeof(export_db) / sizeof(export_db[0]);

	complain = 0;
	for (i = 0; i < drivers; i++) {
		drv = drv_db[i];
		vars = drv->ports == NULL ? 1 : drv->portsno;
		while (vars--) {
			port = drv->ports == NULL ? 0 : drv->ports[vars];
			if (test_port(drv, port)) /* Card found */
				radio_info_show(stdout, drv->name, port);
		}
	}
	complain = 1;

	puts("done.");
}

void
radio_scan(u_int16_t s, u_int16_t e, u_int32_t cycle) {
	u_int16_t ff;
	int signal = 0;
	u_int32_t i;

	if (driver == ERADIO_INVL)
		return;

	if ((drv_db[driver]->caps & DRV_INFO_GETS_SIGNAL) == 0 &&
			(drv_db[driver]->caps & DRV_INFO_GETS_STEREO) == 0) {
		print_wx("This driver does not detect signal state");
		return;
	}
	if (drv_db[driver]->set_freq == NULL || drv_db[driver]->get_state == NULL)
		return;

	range(MIN_FM_FREQ, &s, &e, MAX_FM_FREQ);

	if (e == MIN_FM_FREQ)
		e = MAX_FM_FREQ;

	for (ff = s; ff < e; ff++) {
		signal = 0;
		drv_db[driver]->set_freq(ff);
		for (i = 0; i < cycle; i++)
			signal += drv_db[driver]->get_state();
		printf("%.2f => %d\n", (float)ff/100, signal);
	}
}

u_int16_t
radio_search(int dir, u_int16_t freq) {
	if (driver == ERADIO_INVL)
		return 0u;

	if (drv_db[driver]->search == NULL) {
		if (drv_db[driver]->get_state == NULL)
			print_wx("Driver does not support search");
		else if (dir)
			return search_up_generic(drv_db[driver], freq);
		else
			return search_down_generic(drv_db[driver], freq);
	} else return drv_db[driver]->search(dir, freq);

	return 0u;
}

/* MIXER STUFF */
#ifndef NOMIXER
void
radio_mixer_mute(void) {
	        mixer_set_volume("0,0");
}

int
radio_mixer_set_volume(char *vol) {
	        return mixer_set_volume(vol);
}

int
radio_mixer_update_volume(char *vol) {
	        return mixer_update_device(vol);
}

int
radio_mixer_init(void) {
	        return mixer_init();
}

int
radio_mixer_cleanup(void) {
	        return mixer_cleanup();
}
#endif /* !NOMIXER */

/* INTERNAL STUFF */
int
test_port(struct tuner_drv_t *drv, u_int32_t port) {
	int res = -1;
	u_int16_t i = MAX_FM_FREQ;
	static int c;

	if (drv == NULL)
		return 0;

	if (drv->get_port)
		if (drv->get_port(port) < 0)
			return 0;

	if (drv->find_card) {
		res = drv->find_card();
		draw_stick(c++);
	} else if (drv->caps & DRV_INFO_NEEDS_SCAN)
		if ((drv->caps & DRV_INFO_GETS_SIGNAL) || (drv->caps & DRV_INFO_GETS_STEREO))
			while ((i > MIN_FM_FREQ) && (res < 10)) {
				drv->set_freq(i);
				res += drv->get_state();
				i -= 10;
				draw_stick(c++);
			}

	if (drv->free_port)
		drv->free_port();

	return res < 0 ? 0 : 1;
}

int
check_drv(struct tuner_drv_t *drv, char *name) {
	int drvlen, namelen, i;

	if (name == NULL || *name == '\0')
		return ERADIO_INVL;

	drvlen = strlen(drv->drv);
	namelen = strlen(name);
	if (drvlen > namelen)
		return ERADIO_INVL;

	if (strncasecmp(name, drv->drv, drvlen) == 0) {
		if (drv->portsno > 1) {
			i = strtoul(name + drvlen, (char **)NULL, 10);
			if (i > 0 && i <= drv->portsno)
				return i - 1;
		} else {
			if (namelen == drvlen)
				return 0;
		}
	}

	return ERADIO_INVL;
}

void
print_w(const char *fmt, ...) {
	va_list ap;

	if (complain == 0)
		return;

	va_start(ap, fmt);

	fprintf(stderr, "%s: ", pn);
	if (fmt != NULL) {
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, ": ");
	}
	fprintf(stderr, "%s\n", strerror(errno));

	va_end(ap);
}

void
print_wx(const char *fmt, ...) {
	va_list ap;

	if (complain == 0)
		return;

	va_start(ap, fmt);

	fprintf(stderr, "%s: ", pn);
	if (fmt != NULL)
		vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");

	va_end(ap);
}

void
draw_stick(int count) {
	switch ( count & 0x03 ) {
	case 0: write(1, "|", 1); break;
	case 1: write(1, "/", 1); break;
	case 2: write(1, "-", 1); break;
	case 3: write(1, "\\", 1); break;
	}
	write(1, "\r", 1);
}

void
range(u_int16_t lf, u_int16_t *lpf, u_int16_t *hpf, u_int16_t hf) {
	/* forcing into range */
	if ( *lpf < lf ) *lpf = lf;
	if ( *lpf > hf ) *lpf = hf;

	if ( *hpf < lf ) *hpf = lf;
	if ( *hpf > hf ) *hpf = hf;

	/* swapping values */
	if ( *hpf < *lpf ) {
		u_int16_t buf = *lpf;
		*lpf = *hpf;
		*hpf = buf;
	}

	return;
}

u_int16_t
search_down_generic(struct tuner_drv_t *drv, u_int16_t freq) {
	int max = 0;
	int platoe_start = 0;
	int platoe_count = 0;
	u_int16_t f = freq;

	freq++;

	while (freq > MIN_FM_FREQ) {
		int c = SEARCH_PROBE;
		int s = 0;

		drv->set_freq(--freq);

		while (c--)
			s += drv->get_state();

		/* FIXME: more precise approximation */
		if (s > max) {
			max = s;
			platoe_start = 1;
		} else if (s == max) {
			if (platoe_start)
				platoe_count++;
		} else if (s < max) {
			if (platoe_start) {
				if (platoe_count > SEARCH_LENGTH) {
					freq += platoe_count / 3;
					break;
				}
			} else {
				platoe_start = 0;
				platoe_count = 0;
				max = s;
			}
		}
	}

	if (freq > MIN_FM_FREQ) {
		drv->set_freq(freq);
		return freq;
	}

	drv->set_freq(f);
	return f;
}

u_int16_t
search_up_generic(struct tuner_drv_t *drv, u_int16_t freq) {
	int max = 0;
	int platoe_start = 0;
	int platoe_count = 0;
	u_int16_t f = freq;

	freq--;

	while (freq < MAX_FM_FREQ) {
		int c = SEARCH_PROBE;
		int s = 0;

		drv->set_freq(++freq);

		while (c--)
			s += drv->get_state();

		/* FIXME: more precise approximation */
		if (s > max) {
			max = s;
			platoe_start = 1;
		} else if (s == max) {
			if (platoe_start)
				platoe_count++;
		} else if (s < max) {
			if (platoe_start) {
				if (platoe_count > SEARCH_LENGTH) {
					freq -= 2 * platoe_count / 3;
					break;
				}
			} else {
				platoe_start = 0;
				platoe_count = 0;
				max = s;
			}
		}
	}

	if (freq < MAX_FM_FREQ) {
		drv->set_freq(freq);
		return freq;
	}

	drv->set_freq(f);
	return f;
}
