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

#include <config.h>
#include "defs.h"
#include "ccid.h"
#include "ccid_pcmcia.h"
#include "ccid_ifdhandler.h"

static struct {
	int fd;
	unsigned char seq;
	_ccid_descriptor ccid;
} readers[CCID_DRIVER_MAX_READERS];

#define THIS readers[reader_index]

status_t WritePCMCIA(unsigned int reader_index, unsigned int length, unsigned char *buffer)
{
	if (write(THIS.fd, buffer, length) != length)
		return STATUS_UNSUCCESSFUL;

	return STATUS_SUCCESS;
}

status_t ReadPCMCIA(unsigned int reader_index, unsigned int *length, unsigned char *buffer)
{
	*length = read(THIS.fd, buffer, *length);
	if (*length == -1)
		return STATUS_UNSUCCESSFUL;

	return STATUS_SUCCESS;
}

status_t OpenPCMCIA(unsigned int reader_index, int channel)
{
	return STATUS_UNSUCCESSFUL;
}

status_t OpenPCMCIAByName(unsigned int reader_index, char *dev_name)
{
	if (THIS.fd > 0)
		return STATUS_UNSUCCESSFUL;

	THIS.fd = open(dev_name, O_RDWR);
	if (THIS.fd == -1)
		return STATUS_UNSUCCESSFUL;

	THIS.seq = 0;
	THIS.ccid.pbSeq = &THIS.seq;
	THIS.ccid.dwMaxCCIDMessageLength = 271;
	THIS.ccid.dwMaxIFSD = 254;
	THIS.ccid.dwFeatures = CCID_CLASS_TPDU;
	THIS.ccid.bVoltageSupport = 7;

	return STATUS_SUCCESS;
}

status_t ClosePCMCIA(unsigned int reader_index)
{
	if (THIS.fd <= 0)
		return STATUS_UNSUCCESSFUL;

	close(THIS.fd);
	THIS.fd = 0;
	return STATUS_SUCCESS;
}

_ccid_descriptor *get_ccid_descriptor(unsigned int reader_index)
{
	return &THIS.ccid;
}
