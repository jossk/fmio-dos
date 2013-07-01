/*
 * Copyright (c) 2002 Gunther Mayer
 * Copyright (c) 1998,1999 by Michiel Ronsse <ronsse@elis.rug.ac.be> 
 * Copyright (c) 2001, 2002 Vladimir Popov <jumbo@narod.ru>.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  This is a driver for the Spase PCRadio-003 and Spase PCRadio-002 card.
 *  Based (Layer1-3) on code by:
 *  Michiel Ronsse (linux-2.x "radio-spase.c" standalone v4l module)
 *
 *  The card uses three I2C controlled chips:
 * 
 *   TSA 6057 I2C radio tuning PLL frequency synthesizer		 (synth[])
 *   TEA 6100 I2C FM/IF system and microcomputer-based tuning interface  (fmif[])
 *   TEA6310T I2C sound fader control with tone and volume control       (sound[])
 *
 *  Valid addresses are 0x1B0, 0x1F0, 0x278, 0x378, 0x2F8 and 0x3BC
 */

#ifdef linux
#include <stdlib.h>	/* abs */
#endif /* linux */

#include "ostypes.h"

#include "radio_drv.h"

typedef signed char i2cdata_t[8];
i2cdata_t  fmif, synth, sounda;
#define SOUND_ADDRESS      0x80
#define SYNTH_ADDRESS      0xC4
#define FMIF_WRITE_ADDRESS 0xC2
#define FMIF_READ_ADDRESS  0xC3

#define FALSE 0
#define TRUE  1

#define STEREO 0x60
#define MONO   0x64

#define MUTE   0x80
#define NOMUTE 0x00

#define SPASE_CAPS		DRV_INFO_VOLUME(63) | DRV_INFO_MAXVOL_POLICY | \
				DRV_INFO_VOL_SEPARATE | DRV_INFO_NEEDS_ROOT | \
				DRV_INFO_MONOSTEREO

int grab_port_spase(u_int32_t);
int release_port_spase(void);
u_int32_t info_port_spase(void);
int find_card_spase(void);
void set_freq_spase(u_int16_t);
void set_vol_spase(int);
int state_spase(void);
void mono_spase(void);

u_int32_t sp_ports[] = { 0x1b0, 0x1f0, 0x278, 0x378, 0x2f8, 0x3bc };

struct tuner_drv_t sp_drv = {
	"Spase PC-Radio", "sp", sp_ports, 6, SPASE_CAPS,
	grab_port_spase, release_port_spase, info_port_spase,
	find_card_spase, set_freq_spase, NULL, NULL, set_vol_spase,
	NULL, mono_spase, NULL
};

u_int32_t io;

struct tuner_drv_t *
export_sp(void) {
	return &sp_drv;
}

/* Layer 1: port layer  ************************************************/

void
outport(u_int8_t value){
	int i = 10000;
	OUTB(io, value);
	while (i--)			   /* It's a REALLY slow card ... */
		;
	/* Note: synth chip works without delay; needed by sound chip! */
}

int
inport(void){
	return inb(io);
}

/* Layer 2: I2C layer **************************************************/

void
I2C_start(void) {
	outport(3);
	outport(1);
	outport(0);
}

void
I2C_stop(void) {
	outport(0);
	outport(3);
	outport(0);
	outport(1);
	outport(3);
	outport(0);
}

void
I2C_sendack(void) {
	outport(0);
	outport(1);
	outport(0);
	outport(2);
}

int
I2C_readack(void) {
	int error;
	outport(0);
	outport(3);
	error = ((inport() & 4) == 4);
	outport(0); 
	outport(2);
	return error;
}

signed char
I2C_readbyte(int swap) {
	int bitnr;
	unsigned char byte = 0, addbyte;

	addbyte = swap ? 128 : 1;
	for (bitnr = 1; bitnr <= 7; bitnr++) {
		outport(3);
		if ((inport() & 4) == 4)
			byte += addbyte;
		if (swap)
			byte >>= 1;
		else
			byte <<= 1;
		outport(2);
	}
	outport(3);

	if ((inport() & 4) == 4)
		byte += addbyte;
	outport(2);
	return byte;
}

void
I2C_sendbyte(unsigned char byte) {
	int i;

	for (i = 1; i <= 8; i++) {
		if (byte & 128) {
			outport(2);
			outport(3);
			outport(2);
		} else {
			outport(0);
			outport(1);
			outport(0);
		}
		byte <<= 1;
	}
}

int
I2C_packet(i2cdata_t *data, int NrOfBytes, int swap) {
	int WriteMode, error, byte_i = 0;

	error = FALSE;
	WriteMode = ((*data[byte_i] & 1) == 0);
	I2C_start();
	I2C_sendbyte((*data)[0]);
	error |= I2C_readack();
	if (NrOfBytes > 1) {
		if (WriteMode) {
			if (NrOfBytes > 2)
				for (byte_i = 1; byte_i < NrOfBytes - 1; byte_i++) {
					I2C_sendbyte((*data)[byte_i]);
					error |= I2C_readack();
				}
			I2C_sendbyte((*data)[byte_i]);
		} else {
			if (NrOfBytes > 2)
				for (byte_i = 1; byte_i < NrOfBytes - 1; byte_i++) {
					(*data)[byte_i] = I2C_readbyte(swap);
					I2C_sendack();
				}
			(*data)[byte_i] = I2C_readbyte(swap);
		}
	}
	I2C_stop();
	return error;
}


/* Layer 3: spase layer ************************************************/

int
Sound(int Mode) {
	sounda[1] = 0x05;
	sounda[2] = Mode ? NOMUTE : MUTE;
	return I2C_packet(&sounda, 3, FALSE);
}

int
SetStereo(int Stereo) {
	synth[1] = 0x02;
	synth[2] = Stereo ? STEREO : MONO;
	return I2C_packet(&synth, 3, FALSE);
}

int
SetAudio(unsigned int Volume, unsigned int Balance, unsigned int Treble, unsigned int Bass) {
	int Dummy;

	/* volume :
	   63: +20db
	   62: +18db
	   ...
	   53: 0 db
	   52: -2db
	   ...
	   20: -66db
	   <20: mute
	 */
	if (Volume <  0)  Volume =  0;
	if (Volume >  63) Volume =  63;

	/* mapping volume/balance to volume_left/volume_right */
	sounda[2] = Volume;       /* Left volume:  0..63 */
	sounda[3] = Volume;       /* Right volume: 0..63 */

	Dummy = Balance + 8;     /* 0..16 */
	Dummy = (Dummy<0) ? 0 : Dummy;
	Dummy = (Dummy>16) ? 16 : Dummy;
	if (Dummy < 8) sounda[2] = sounda[2] - (Volume/8 - 3) * abs(Dummy - 8);
	if (Dummy > 8) sounda[3] = sounda[3] - (Volume/8 - 3) * abs(Dummy - 8);

	if (sounda[2] <  0) sounda[2] =  0;
	if (sounda[3] <  0) sounda[3] =  0;

	/* bass */
	Bass += 7;     /* 3..11 */
	if (Bass <  3) Bass =  3;
	if (Bass > 11) Bass = 11;
	sounda[4] = Bass;

	/* treble */
	Treble += 7;   /* 3..11 */
	if (Treble <  3) Treble =  3;
	if (Treble > 11) Treble = 11;
	sounda[5] = Treble;
	sounda[1] = 0x00;
	return I2C_packet(&sounda, 6, FALSE);
}

int
GetTuningInfo(char * Level, char * Stereo, int * Deviation) {
	int error;

	error = I2C_packet(&fmif, 3, TRUE);
	if (Stereo)
		*Stereo = ((fmif[1] & 0xF0) >> 5) <= 3;	/* 0..1     */
	if (Level)
		*Level = (fmif[1] & 0x0F) >> 1;		/* 0..7     */
	if (Deviation)
		*Deviation = (fmif[2] - 127) / 2 ;	/* -64..+64 */
	return error;
}

/* returns 0:card not found, 1:found */
int
CheckAddress(void) {
	outport(0);
	outport(1);
	outport(0);
	if ((inport() & 4) == 0) {
		outport(2);
		outport(3);
		outport(2);
		if ((inport() & 4) != 4)
			return FALSE;
	} else
		return FALSE;

	return TRUE;
}

int
InitRadio_spase(void) {
	if (!(CheckAddress()))
		return -1;

	synth[0] = (signed char)SYNTH_ADDRESS;
	synth[1] = 0x00;		/* sub address			   */
	synth[4] = (signed char)STEREO;
	synth[5] = 0x00;		/* initialization code at program start  */

	fmif[0] = (signed char)FMIF_WRITE_ADDRESS;
	fmif[1] = (signed char)0xFE;	 /* initialization code at program start  */
	I2C_packet(&fmif, 3, FALSE);     /* initialize fmif for FM mode	   */
	fmif[0] = (signed char)FMIF_READ_ADDRESS;

	sounda[0] = (signed char)SOUND_ADDRESS;
	sounda[1] = 0x00;		/* sub address			   */
	sounda[6] = (signed char)0xFF;	/* initialization code at program start  */
	sounda[7] = 0x00;

	SetStereo(TRUE);
	SetAudio(0, 0, 0, 0);
	Sound(TRUE);

	return 0;
}

/* Layer 4: fmio layer */
int
grab_port_spase(u_int32_t port) {
	io = port;
	return radio_get_ioperms(io, 1) < 0 ? -1 : 0;
}

int
find_card_spase(void) {
	if (InitRadio_spase() == 0)
		return 0;
	print_wx("spase-pcradio not found at 0x%x", io);
	return -1;
}

int
release_port_spase(void) {
	return radio_release_ioperms(io, 1);
}

void
tsa6057_encode_freq(u_int32_t f, i2cdata_t tsa6057_data) {
	f += 1070; /* add 10.7 MHz IF intermediate frequency */
	tsa6057_data[0]= (signed char)SYNTH_ADDRESS;
	/* subaddress 0 (Start Autoincrement at register 0) */
	tsa6057_data[1]= 0;
	/* Set CP */
	tsa6057_data[2]= 1 | (f << 1);
	tsa6057_data[3]= f >> 7;
	/* Set 10kHz stepsize, FM Band */
	tsa6057_data[4]= 0x60 | ((f >> 15) & 3);
	tsa6057_data[5]= 0; /* no need to send this */
	
}

/* frequency is given in 10kHz units  */
void
set_freq_spase(u_int16_t frequency) {
	i2cdata_t tsa6057_data;

	tsa6057_encode_freq(frequency, tsa6057_data);
	I2C_packet(&tsa6057_data, 5, FALSE);
}

u_int32_t
info_port_spase(void) {
	return io;
}

void
set_vol_spase(int v) {
	if (v > 63)
		v = 63;
	if (v < 0)
		v = 0;
	Sound(v ? 1 : 0);
	SetAudio(v, 0, 0, 0);
}
 
void
mono_spase(void) {
	SetStereo(0); /* set mono */
}

int
state_spase(void) {
	char l, s;
	int d;
	GetTuningInfo(&l, &s, &d);
	return 0;
}
