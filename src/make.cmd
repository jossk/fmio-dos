set CC=wcl386
set CFLAGS=-q -l=pmodew -d__DOS__ -dNOMIXER -uUSE_BKTR -uBSDRADIO -uBSDBKTR
set FILES=fmio.c access.c aztech.c bmc-hma.c bu2614.c ecoradio.c gemtek-isa.c gemtek-pci.c lm700x.c pci.c pt2254a.c radio.c radiotrack.c radiotrackII.c sf16fmd2.c sf16fmr.c sf16fmr2.c sf256pcpr.c sf256pcsr.c sf64pce2.c sf64pcr.c spase.c tc921x.c tea5757.c terratec-isa.c trust.c zoltrix.c
%CC% %CFLAGS% %FILES%

























