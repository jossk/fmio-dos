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
 * $Id$
 */

#ifndef EXPORT_H__
#define EXPORT_H__

#include "radio_drv.h"

struct tuner_drv_t *export_aztech(void);
#ifdef USE_BKTR
struct tuner_drv_t *export_bktr(void);
#endif /* USE_BKTR */
struct tuner_drv_t *export_bmc(void);
#ifdef BSDRADIO
struct tuner_drv_t *export_bsdradio(void);
#endif /* BSDRADIO */
struct tuner_drv_t *export_er(void);
struct tuner_drv_t *export_gti(void);
struct tuner_drv_t *export_gtp(void);
struct tuner_drv_t *export_mr(void);
struct tuner_drv_t *export_rt(void);
struct tuner_drv_t *export_rtii(void);
struct tuner_drv_t *export_sf16fmd2(void);
struct tuner_drv_t *export_sf16fmr(void);
struct tuner_drv_t *export_sf16fmr2(void);
struct tuner_drv_t *export_sf64pce2(void);
struct tuner_drv_t *export_sf64pcr(void);
struct tuner_drv_t *export_sf256pcpr(void);
struct tuner_drv_t *export_sf256pcs(void);
struct tuner_drv_t *export_sfi(void);
struct tuner_drv_t *export_sp(void);
struct tuner_drv_t *export_svg(void);
struct tuner_drv_t *export_tr(void);
struct tuner_drv_t *export_tt(void);
#ifdef BSDBKTR
struct tuner_drv_t *export_xtreme(void);
#endif /* BSDBKTR */
struct tuner_drv_t *export_zx(void);

#endif /* EXPORT_H__ */
