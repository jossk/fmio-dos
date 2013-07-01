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

#ifndef BU2614_H__
#define BU2614_H__

#define BU2614_REGISTER_LENGTH		32

#define BU2614_FREQ			0xFFFF
#define BU2614_CONF			(0xFFFF << 16)

#define BU2614_OUTPUT(x)		(x << 16)

#define BU2614_CT_OFF			(0 << 23)
#define BU2614_CT_ON			(1 << 23)

#define BU2614_REF_FREQ(x)		(x << 24)
#define   BU2614_REF_FREQ_25_KHZ	BU2614_REF_FREQ(0)
#define   BU2614_REF_FREQ_12P5_KHZ	BU2614_REF_FREQ(1)
#define   BU2614_REF_FREQ_6P25_KHZ	BU2614_REF_FREQ(2)
#define   BU2614_REF_FREQ_5_KHZ		BU2614_REF_FREQ(3)
#define   BU2614_REF_FREQ_3P125_KHZ	BU2614_REF_FREQ(4)
#define   BU2614_REF_FREQ_3_KHZ		BU2614_REF_FREQ(5)
#define   BU2614_REF_FREQ_1_KHZ		BU2614_REF_FREQ(6)
#define   BU2614__PLL_OFF		BU2614_REF_FREQ(7)

#define BU2614_BAND_FM			(0 << 27)
#define BU2614_BAND_AM			(1 << 27)

#define BU2614_PS_OFF			(0 << 28)
#define BU2614_PS_ON			(1 << 28)

#define BU2614_GT_OFF			(0 << 30)
#define BU2614_GT_ON			(1 << 30)

#define BU2614_TS_OFF			(0 << 31)
#define BU2614_TS_ON			(1 << 31)

struct bu2614_t {
	u_int8_t wren;
	u_int8_t data;
	u_int8_t clck;
	u_int32_t port;
};

u_int16_t bu2614_conv_freq(u_int16_t);
u_int16_t bu2614_unconv_freq(u_int32_t);

void bu2614_write(struct bu2614_t *, u_int32_t);

#endif /* BU2614_H__ */
