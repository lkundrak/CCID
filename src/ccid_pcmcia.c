/*
    ccid_pcmcia.c: communicate with a CCID PCMCIA card reader
    Copyright (C) 2016 Lubomir Rintel

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc., 51
	Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <config.h>
#include "defs.h"
#include "ccid.h"
#include "ccid_pcmcia.h"
#include "ccid_ifdhandler.h"

static struct {
	int fd;
	unsigned char seq;
	_ccid_descriptor ccid;
	char *dev_name;
} readers[CCID_DRIVER_MAX_READERS];

#define THIS readers[reader_index]

static status_t status_from_errno()
{
	switch (errno)
	{
		case 0:
			return STATUS_SUCCESS;
		case EIO:
			return STATUS_COMM_ERROR;
		case ENOENT:
		case ENODEV:
			return STATUS_NO_SUCH_DEVICE;
		default:
			return STATUS_UNSUCCESSFUL;
	}
}

/* We close the underlying device file as soon as we detect an error, so
 * that kernel has a chance to release resources associated with the file
 * descriptor in case the device was removed. We also attempt to re-open
 * the device to increase robustness in case of transient errors. */

static int
open_device(unsigned int reader_index)
{
	if (!THIS.dev_name) {
		errno = EINVAL;
		return -1;
	}
	if (THIS.fd <= 0)
		THIS.fd = open(THIS.dev_name, O_RDWR);
	return THIS.fd;
}

static void
close_device(unsigned int reader_index)
{
	close(THIS.fd);
	THIS.fd = -1;
}

status_t WritePCMCIA(unsigned int reader_index, unsigned int length, unsigned char *buffer)
{
	int ret;

	if (open_device(reader_index) == -1)
		return status_from_errno();

	ret = write(THIS.fd, buffer, length);
	if (ret == -1) {
		close_device(reader_index);
		return status_from_errno();
	}
	if (ret != length)
		return STATUS_DEVICE_PROTOCOL_ERROR;

	return STATUS_SUCCESS;
}

status_t ReadPCMCIA(unsigned int reader_index, unsigned int *length, unsigned char *buffer)
{
	int ret;

	if (open_device(reader_index) == -1)
		return status_from_errno();

	ret = read(THIS.fd, buffer, *length);
	if (ret == -1) {
		close_device(reader_index);
		return status_from_errno();
	}

	*length = ret;
	return STATUS_SUCCESS;
}

status_t OpenPCMCIA(unsigned int reader_index, int channel)
{
	char actual_name[sizeof("/dev/scr24xXX")];

	if (snprintf(actual_name, sizeof(actual_name), "/dev/scr24x%d", channel) >= sizeof(actual_name))
		return STATUS_UNSUCCESSFUL;

	return OpenPCMCIAByName(reader_index, actual_name);
}

status_t OpenPCMCIAByName(unsigned int reader_index, char *dev_name)
{
	if (THIS.fd > 0)
		return STATUS_UNSUCCESSFUL;

	THIS.dev_name = strdup(dev_name);
	if (!THIS.dev_name)
		return STATUS_UNSUCCESSFUL;
	THIS.seq = 0;
	THIS.ccid.pbSeq = &THIS.seq;
	THIS.ccid.dwMaxCCIDMessageLength = 271;
	THIS.ccid.dwMaxIFSD = 254;
	THIS.ccid.dwFeatures = CCID_CLASS_TPDU;
	THIS.ccid.bVoltageSupport = 7;

	if (strstr(dev_name, "scr24x"))
		THIS.ccid.readerID = SCR24X;

	if (open_device(reader_index) == -1)
		return status_from_errno();

	return STATUS_SUCCESS;
}

status_t ClosePCMCIA(unsigned int reader_index)
{
	if (!THIS.dev_name)
		return STATUS_UNSUCCESSFUL;

	free(THIS.dev_name);
	THIS.dev_name = NULL;

	close_device(reader_index);
	return STATUS_SUCCESS;
}

_ccid_descriptor *get_ccid_descriptor(unsigned int reader_index)
{
	return &THIS.ccid;
}
