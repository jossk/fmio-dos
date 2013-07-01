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
 * $Id: radio.h,v 1.22 2001/08/28 12:53:42 pva Exp $
 *
 */

#ifndef RADIO_H__
#define RADIO_H__

#include <sys/types.h>

#include <stdio.h>

#define ERADIO_INVL	-1	/* Invalid/nonexistent driver */
#define ERADIO_HRDW	0xF00F	/* Hardware error during initialization */

#define MIN_FM_FREQ	8750
#define MAX_FM_FREQ	10800

void radio_init(void);	/* Initialize drivers database */
int radio_cleanup(void);

int radio_drv_init(char *); /* Check driver validity */
int radio_drv_free(void);

int radio_get_port(void);
int radio_free_port(void);
int radio_test_port(void);

void radio_set_freq(u_int16_t);

void radio_set_volume(int);
void radio_set_mono(void);

int radio_info_root(void);
char *radio_info_name(void);
u_int32_t radio_info_port(void);
u_int8_t radio_info_maxvol(void);
int radio_info_policy(void);
void radio_info_show(FILE *, char *, u_int32_t);

u_int16_t radio_info_freq(void);
int radio_info_volume(void);

int radio_info_signal(void);
int radio_info_stereo(void);

void radio_detect(void);
void radio_scan(u_int16_t, u_int16_t, u_int32_t);
u_int16_t radio_search(int, u_int16_t);

#ifndef NOMIXER
int radio_mixer_init(void);
int radio_mixer_cleanup(void);
void radio_mixer_mute(void);
int radio_mixer_set_volume(char *);
int radio_mixer_update_volume(char *);
#endif /* !NOMIXER */

#endif /* RADIO_H__ */
