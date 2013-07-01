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
 * $Id: access.c,v 1.4 2002/01/14 10:21:31 pva Exp $
 * lowlevel functions for port access
 */

#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ostypes.h"

#include "radio_drv.h"

const char *open_error = "%s open error";
const char *close_error = "%s close error";

#ifdef linux
const char *tuner_device_1 = "/dev/radio";
const char *tuner_device_2 = "/dev/radio0";
const char *radio_device_1 = NULL;
const char *radio_device_2 = NULL;
#else
const char *tuner_device_1 = "/dev/tuner";
const char *tuner_device_2 = "/dev/tuner0";
const char *radio_device_1 = "/dev/radio";
const char *radio_device_2 = "/dev/radio0";
#endif /* linux */

#ifdef __FreeBSD__
const char *devio = "/dev/io";
static int fd = -1;

int
fbsd_get_ioperms(void) {
	if ((fd = open(devio, O_RDONLY)) < 0) {
		print_w(open_error, devio);
		return -1;
	}

	return 0;
}

int
fbsd_release_ioperms(void) {
	if (close(fd) < 0) {
		print_w(close_error, devio);
		return -1;
	}

	return 0;
}
#elif defined __QNXNTO__
int
qnx_iopl_acquire() {
	ThreadCtl(_NTO_TCTL_IO, 0);
	return 0;
}
#elif defined linux
int
os_iopl(int v) {
	if (iopl(v) < 0) {
		print_w("iopl() error");
		return -1;
	}
	return 0;
}

int
os_ioperms(u_int32_t port, int no, int v) {
	if (ioperm(port, no, v) < 0) {
		print_w("ioperm() error");
		return -1;
	}

	return 0;
}
#elif defined __OpenBSD__ || defined __NetBSD__
int
os_iopl(int v) {
	struct i386_iopl_args iopls;

	iopls.iopl = v;
	if (sysarch(I386_IOPL, (char *)&iopls) < 0) {
		print_w("I386_IOPL error");
		return -1;
	}

	return 0;
}

int
os_ioperms(u_int32_t port, int no, int v) {
	unsigned long iomap[32];
	int offset;
	u_int32_t mask;
	u_int32_t ports = ~0;
	struct i386_set_ioperm_args ioperms;

	memset(iomap, 0xFFFF, sizeof(iomap));
	if (v) {
		while (no--)
			ports <<= 1;
		offset = port / 32;
		mask = ~ports << port % 32;
		iomap[offset] ^= mask;
	}
	ioperms.iomap = iomap;
	if (sysarch(I386_SET_IOPERM, (char *)&ioperms) < 0) {
		print_w("I386_SET_IOPERM error");
		return -1;
	}

	return 0;
}
#elif defined __DOS__
int
os_iopl(int v) {
	return 0;
}

int
os_ioperms(u_int32_t port, int no, int v) {
	return 0;
}
#else
int
os_iopl(int v) {
	return -1;
}

int
os_ioperms(u_int32_t port, int no, int v) {
	return -1;
}
#endif /* __FreeBSD__ */

int
radio_get_iopl(void) {
#ifdef __FreeBSD__
	return fbsd_get_ioperms();
#elif defined __QNXNTO__
	return qnx_iopl_acquire();
#else
	return os_iopl(3);
#endif
}

int
radio_release_iopl(void) {
#ifdef __FreeBSD__
	return fbsd_release_ioperms();
#elif defined __QNXNTO__
	return 0;
#else
	return os_iopl(0);
#endif
}

int
radio_get_ioperms(u_int32_t port, int no) {
#ifdef __FreeBSD__
	return fbsd_get_ioperms();
#elif defined __QNXNTO__
	return qnx_iopl_acquire();
#else
	return os_ioperms(port, no, 1);
#endif
}

int
radio_release_ioperms(u_int32_t port, int no) {
#ifdef __FreeBSD__
	return fbsd_release_ioperms();
#elif defined __QNXNTO__
	return 0;
#else
	return os_ioperms(port, no, 0);
#endif
}

int
radio_device_get(const char *name, const char *backup, int flags) {
	char buf[FILENAME_MAX + 1], next_buf[FILENAME_MAX + 1];
	struct stat s;
	int rfd = -1;
	int rfnb = 0; /* Length of file name read from readlink() */
	int depth = 0, found = 0;

	strncpy(buf, name, FILENAME_MAX);
	buf[FILENAME_MAX] = '\0';

	/* Follow symlinks until the file is found */
	for (depth = 0; depth < SYMLINK_DEPTH; depth++) {

		if (lstat(buf, &s) < 0)
			break;

		/* It's not a symlink. Search is finished then */
		if (S_ISLNK(s.st_mode) == 0) {
			found = 1;
			break;
		}

		rfnb = readlink(buf, next_buf, FILENAME_MAX);
		if (rfnb < 0 || rfnb == 0)
			break;
		next_buf[rfnb] = '\0';

		strncpy(buf, next_buf, FILENAME_MAX);
		buf[FILENAME_MAX] = '\0';
	}

	/*
	 * Ok, the "name" doesn't exist or is buried too deep
	 * Let's try last resort -- the "backup"
	 */
	if (!found) {
		if (backup == NULL || *backup == '\0') {
			print_wx("%s does not exist, "
			      "backup file was not specified too", name);
			return -1;
		}
		strncpy(buf, backup, FILENAME_MAX);
		buf[FILENAME_MAX] = '\0';
	}

	if ((rfd = open(buf, flags)) < 0) {
		print_w(open_error, buf);
		return -1;
	}
 
	return rfd;
}

int
radio_device_release(int rfd, const char *fdname) {
	if (close(rfd) < 0) {
		print_w(close_error, fdname);
		return -1;
	}

	return 0;
}
