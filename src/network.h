/*
   Copyright (c) 1993 Richard V. Nash.
   Copyright (c) 2000 Dan Papasian
   Copyright (C) Andrew Tridgell 2002
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef NETWORK_H
#define NETWORK_H

#define NET_NETERROR 0
#define NET_NEW 1
#define NET_DISCONNECT 2
#define NET_READLINE 3
#define NET_TIMEOUT 4
#define NET_NOTCOMPLETE 5

#define LINE_WIDTH 80

#ifndef O_NONBLOCK
#define O_NONBLOCK	00004
#endif

GENSTRUCT enum netstatus {NETSTAT_EMPTY, NETSTAT_CONNECTED, NETSTAT_IDENT};

GENSTRUCT struct connection_t {
	int fd;
	int outFd;
	struct in_addr fromHost;
	enum netstatus status;
	/* Input buffering */
	int numPending;
	int processed;
	unsigned char inBuf[MAX_STRING_LENGTH+1]; _NULLTERM
	/* Output buffering */
	int sndbufsize;		/* size of send buffer (this changes) */
	int sndbufpos;		/* position in send buffer */
	unsigned char *sndbuf; _LEN(sndbufsize)_NULLTERM  /* our send buffer, or NULL if none yet */
	int outPos;		/* column count */
	int state;		/* 'telnet state' */
	int mypal;
	int timeseal;           /* are they using timeseal */
	int timeseal_init;      /* are they in timeseal initialisation? */
	unsigned time;          /* last time received from the client */
};

#endif /* NETWORK_H */
