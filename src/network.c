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

#include "includes.h"

extern int errno;

/* Index == fd, for sparse array, quick lookups! wasted memory :( */
static int findConnection(int fd)
{
	if (fd == -1 || net_globals.con[fd]->status == NETSTAT_EMPTY)
		return -1;
	else
		return fd;
}


static void set_sndbuf(int fd, int len)
{
	net_globals.con[fd]->sndbufpos = len;
	if (len < net_globals.con[fd]->sndbufsize) {
		net_globals.con[fd]->sndbuf[len] = 0;
	}
}

static int net_addConnection(int fd, struct in_addr fromHost)
{
	int noblock = 1;

	/* possibly expand the connections array */
	if (fd >= net_globals.no_file) {
		int i;
		net_globals.con = (struct connection_t **)realloc(net_globals.con,
								  (fd+1) * sizeof(struct connection_t *));
		for (i=net_globals.no_file;i<fd+1;i++) {
			net_globals.con[i] = (struct connection_t *)calloc(1, 
									   sizeof(struct connection_t));
			net_globals.con[i]->status = NETSTAT_EMPTY;
		}
		net_globals.no_file = fd+1;
	}

	if (findConnection(fd) >= 0) {
		d_printf( "CHESSD: FD already in connection table!\n");
		return -1;
	}
	if (ioctl(fd, FIONBIO, &noblock) == -1) {
		d_printf( "Error setting nonblocking mode errno=%d\n", errno);
	}
	net_globals.con[fd]->fd = fd;
	if (fd != 0)
		net_globals.con[fd]->outFd = fd;
	else
		net_globals.con[fd]->outFd = 1;
	net_globals.con[fd]->fromHost = fromHost;
	net_globals.con[fd]->status = NETSTAT_CONNECTED;
	net_globals.con[fd]->timeseal = 0;
	net_globals.con[fd]->timeseal_init = 1;
	net_globals.con[fd]->time = 0;
	net_globals.con[fd]->numPending = 0;
	net_globals.con[fd]->inBuf[0] = 0;
	net_globals.con[fd]->processed = 0;
	net_globals.con[fd]->outPos = 0;
	if (net_globals.con[fd]->sndbuf == NULL) {
#ifdef DEBUG
		d_printf( "CHESSD: nac(%d) allocating sndbuf.\n", fd);
#endif
		net_globals.con[fd]->sndbufpos = 0;
		net_globals.con[fd]->sndbufsize = MAX_STRING_LENGTH;
		net_globals.con[fd]->sndbuf = malloc(MAX_STRING_LENGTH);
	} else {
#ifdef DEBUG
		d_printf( "CHESSD: nac(%d) reusing old sndbuf size %d pos %d.\n", fd, net_globals.con[fd].sndbufsize, net_globals.con[fd].sndbufpos);
#endif
	}
	net_globals.con[fd]->state = 0;
	net_globals.numConnections++;
	
#ifdef DEBUG
	d_printf( "CHESSD: fd: %d connections: %d  descriptors: %d \n", fd, numConnections, getdtablesize());	/* sparky 3/13/95 */
#endif
	
	return 0;
}

static int remConnection(int fd)
{
	int which, i;
	if ((which = findConnection(fd)) < 0) {
		d_printf( "remConnection: Couldn't find fd to close.\n");
		return -1;
	}
	net_globals.numConnections--;
	net_globals.con[fd]->status = NETSTAT_EMPTY;
	if (net_globals.con[fd]->sndbuf == NULL) {
		d_printf( "CHESSD: remcon(%d) SNAFU, this shouldn't happen.\n", fd);
	} else {
		if (net_globals.con[fd]->sndbufsize > MAX_STRING_LENGTH) {
			net_globals.con[fd]->sndbufsize = MAX_STRING_LENGTH;
			net_globals.con[fd]->sndbuf = realloc(net_globals.con[fd]->sndbuf, MAX_STRING_LENGTH);
		}
		if (net_globals.con[fd]->sndbufpos) {	/* didn't send everything, bummer */
			set_sndbuf(fd, 0);
		}
	}

	/* see if we can shrink the con array */
	for (i=fd;i<net_globals.no_file;i++) {
		if (net_globals.con[i]->status != NETSTAT_EMPTY) {
			return 0;
		}
	}

	/* yep! shrink it */
	for (i=fd;i<net_globals.no_file;i++) {
		FREE(net_globals.con[i]->sndbuf);
		FREE(net_globals.con[i]);
	}
	net_globals.no_file = fd;
	net_globals.con = (struct connection_t **)realloc(net_globals.con,
							  net_globals.no_file * sizeof(struct connection_t *));

	return 0;
}

static void net_flushme(int which)
{
  int sent;

  sent = send(net_globals.con[which]->outFd, net_globals.con[which]->sndbuf, net_globals.con[which]->sndbufpos, 0);
  if (sent == -1) {
    if (errno != EPIPE)		/* EPIPE = they've disconnected */
      d_printf( "CHESSD: net_flushme(%d) couldn't send, errno=%d.\n", which, errno);
    set_sndbuf(which, 0);
  } else {
    net_globals.con[which]->sndbufpos -= sent;
    if (net_globals.con[which]->sndbufpos)
      memmove(net_globals.con[which]->sndbuf, net_globals.con[which]->sndbuf + sent, net_globals.con[which]->sndbufpos);
  }
  if (net_globals.con[which]->sndbufsize > MAX_STRING_LENGTH && net_globals.con[which]->sndbufpos < MAX_STRING_LENGTH) {
    /* time to shrink the buffer */
    net_globals.con[which]->sndbuf = realloc(net_globals.con[which]->sndbuf, MAX_STRING_LENGTH);
    net_globals.con[which]->sndbufsize = MAX_STRING_LENGTH;
  }
  set_sndbuf(which, net_globals.con[which]->sndbufpos);
}

static void net_flush_all_connections(void)
{
	int which;
	fd_set writefds;
	struct timeval to;
	
	FD_ZERO(&writefds);
	for (which = 0; which < net_globals.no_file; which++) {
		if (net_globals.con[which]->status == NETSTAT_CONNECTED && 
		    net_globals.con[which]->sndbufpos){
			FD_SET(net_globals.con[which]->outFd, &writefds);
		}
	}

	to.tv_usec = 0;
	to.tv_sec = 0;
	select(net_globals.no_file, NULL, &writefds, NULL, &to);
	for (which = 0; which < net_globals.no_file; which++) {
		if (FD_ISSET(net_globals.con[which]->outFd, &writefds)) {
			net_flushme(which);
		}
	}
}

static void net_flush_connection(int fd)
{
  int which;
  fd_set writefds;
  struct timeval to;

  if (((which = findConnection(fd)) >= 0) && (net_globals.con[which]->sndbufpos)) {
    FD_ZERO(&writefds);
    FD_SET(net_globals.con[which]->outFd, &writefds);
    to.tv_usec = 0;
    to.tv_sec = 0;
    select(net_globals.no_file, NULL, &writefds, NULL, &to);
    if (FD_ISSET(net_globals.con[which]->outFd, &writefds)) {
      net_flushme(which);
    }
  }
}

static int sendme(int which, char *str, int len)
{
  int i, count;
  fd_set writefds;
  struct timeval to;
  count = len;

  while ((i = ((net_globals.con[which]->sndbufsize - net_globals.con[which]->sndbufpos) < len) ? (net_globals.con[which]->sndbufsize - net_globals.con[which]->sndbufpos) : len) > 0) {
    memmove(net_globals.con[which]->sndbuf + net_globals.con[which]->sndbufpos, str, i);
    net_globals.con[which]->sndbufpos += i;
    if (net_globals.con[which]->sndbufpos == net_globals.con[which]->sndbufsize) {

      FD_ZERO(&writefds);
      FD_SET(net_globals.con[which]->outFd, &writefds);
      to.tv_usec = 0;
      to.tv_sec = 0;
      select(net_globals.no_file, NULL, &writefds, NULL, &to);
      if (FD_ISSET(net_globals.con[which]->outFd, &writefds)) {
	net_flushme(which);
      } else {
	/* time to grow the buffer */
	net_globals.con[which]->sndbufsize += MAX_STRING_LENGTH;
	net_globals.con[which]->sndbuf = realloc(net_globals.con[which]->sndbuf, net_globals.con[which]->sndbufsize);
      }
    }
    str += i;
    len -= i;
  }
  set_sndbuf(which, net_globals.con[which]->sndbufpos);
  return count;
}

/*
 * -1 for an error other than EWOULDBLOCK.
 * Put <lf> after every <cr> and put \ at the end of overlength lines.
 * Doesn't send anything unless the buffer fills, output waits until
 * flushed
*/
/* width here is terminal width = width var + 1 at presnt) */
int net_send_string(int fd, char *str, int format, int width)
{
  int which, i, j;

  if ((which = findConnection(fd)) < 0) {
    return -1;
  }
  while (*str) {
    for (i = 0; str[i] >= ' '; i++);
    if (i) {
      if (format && (i >= (j = width - net_globals.con[which]->outPos))) {	/* word wrap */
	i = j-1;
	while (i > 0 && str[i - 1] != ' ')
	  i--;
/*
	while (i > 0 && str[i - 1] == ' ')
	  i--;
*/
	if (i == 0)
	  i = j - 1;
	sendme(which, str, i);
	sendme(which, "\n\r\\   ", 6);
	net_globals.con[which]->outPos = 4;
	while (str[i] == ' ')	/* eat the leading spaces after we wrap */
	  i++;
      } else {
	sendme(which, str, i);
	net_globals.con[which]->outPos += i;
      }
      str += i;
    } else {			/* non-printable stuff handled here */
      switch (*str) {
      case '\t':
	sendme(which, "        ", 8 - (net_globals.con[which]->outPos & 7));
	net_globals.con[which]->outPos &= ~7;
	if (net_globals.con[which]->outPos += 8 >= width)
	  net_globals.con[which]->outPos = 0;
	break;
      case '\n':
	sendme(which, "\n\r", 2);
	net_globals.con[which]->outPos = 0;
	break;
      case '\033':
	net_globals.con[which]->outPos -= 3;
      default:
	sendme(which, str, 1);
      }
      str++;
    }
  }
  return 0;
}

/* if we get a complete line (something terminated by \n), copy it to com
   and return 1.
   if we don't get a complete line, but there is no error, return 0.
   if some error, return -1.
 */
static int readline2(char *com, int who)
{
  unsigned char *start, *s, *d;
  int howmany, state, fd, pending;

  const unsigned char will_tm[] = {IAC, WILL, TELOPT_TM, '\0'};
  const unsigned char will_sga[] = {IAC, WILL, TELOPT_SGA, '\0'};
  const unsigned char ayt[] = "[Responding to AYT: Yes, I'm here.]\n";

  state = net_globals.con[who]->state;
  if ((state == 2) || (state > 4)) {
    d_printf( "CHESSD: state screwed for net_globals.con[%d], this is a bug.\n", who);
    state = 0;
  }
  s = start = net_globals.con[who]->inBuf;
  pending = net_globals.con[who]->numPending;
  fd = net_globals.con[who]->fd;

  howmany = recv(fd, start + pending, MAX_STRING_LENGTH - 1 - pending, 0);
  if (howmany == 0)		/* error: they've disconnected */
    return (-1);
  else if (howmany == -1) {
    if (errno != EWOULDBLOCK) {	/* some other error */
      return (-1);
    } else if (net_globals.con[who]->processed) {	/* nothing new and nothing old */
      return (0);
    } else {			/* nothing new, but some unprocessed old */
      howmany = 0;
    }
  }
  if (net_globals.con[who]->processed)
    s += pending;
  else
    howmany += pending;
  d = s;

  for (; howmany-- > 0; s++) {
    switch (state) {
    case 0:			/* Haven't skipped over any control chars or
				   telnet commands */
      if (*s == IAC) {
	d = s;
	state = 1;
      } else if (*s == '\n') {
	*s = '\0';
	strcpy(com, start);
	if (howmany)
	  bcopy(s + 1, start, howmany);
	net_globals.con[who]->state = 0;
	net_globals.con[who]->numPending = howmany;
	net_globals.con[who]->inBuf[howmany] = 0;
	net_globals.con[who]->processed = 0;
	net_globals.con[who]->outPos = 0;
	return (1);
/*
 Cannot strip ^? yet otherwise timeseal probs occur
*/
      } else if (*s == '\010') { /* ^H lets delete */
        if (s > start)
          d = s-1;
        else
          d = s;
        state = 2;
      } else if ((*s > (0xff - 0x20)) || (*s < 0x20)) {   
	d = s;
	state = 2;
      }
      break;
    case 1:			/* got telnet IAC */
      if (*s == IP)
	return (-1);		/* ^C = logout */
      else if (*s == DO)
	state = 4;
      else if ((*s == WILL) || (*s == DONT) || (*s == WONT))
	state = 3;		/* this is cheesy, but we aren't using em */
      else if (*s == AYT) {
	send(fd, (char *) ayt, strlen((char *) ayt), 0);
	state = 2;
      } else if (*s == EL) {	/* erase line */
	d = start;
	state = 2;
      } else			/* dunno what it is, so ignore it */
	state = 2;
      break;
    case 2:			/* we've skipped over something, need to
				   shuffle processed chars down */
      if (*s == IAC)
	state = 1;
      else if (*s == '\n') {
	*d = '\0';
	strcpy(com, start);
	if (howmany)
	  memmove(start, s + 1, howmany);
	net_globals.con[who]->state = 0;
	net_globals.con[who]->numPending = howmany;
	net_globals.con[who]->inBuf[howmany] = 0;
	net_globals.con[who]->processed = 0;
	net_globals.con[who]->outPos = 0;
	return (1);
/*
 Cannot strip ^? yet otherwise timeseal probs occur
*/
      } else if (*s == '\010') { /* ^H lets delete */
        if (d > start)
          d = d-1;
      } else if (*s >= 0x20)
        *(d++) = *s;
      break;
    case 3:			/* some telnet junk we're ignoring */
      state = 2;
      break;
    case 4:			/* got IAC DO */
      if (*s == TELOPT_TM)
	send(fd, (char *) will_tm, strlen((char *) will_tm), 0);
      else if (*s == TELOPT_SGA)
	send(fd, (char *) will_sga, strlen((char *) will_sga), 0);
      state = 2;
      break;
    }
  }
  if (state == 0)
    d = s;
  else if (state == 2)
    state = 0;
  net_globals.con[who]->state = state;
  net_globals.con[who]->numPending = d - start;
  net_globals.con[who]->inBuf[d-start] = 0;
  net_globals.con[who]->processed = 1;
  if (net_globals.con[who]->numPending == MAX_STRING_LENGTH - 1) {	/* buffer full */
    *d = '\0';
    strcpy(com, start);
    net_globals.con[who]->state = 0;
    net_globals.con[who]->numPending = 0;
    net_globals.con[who]->inBuf[0] = 0;
    net_globals.con[who]->processed = 0;
    return (1);
  }
  return (0);
}

int net_init(int port)
{
  int opt;
  struct sockaddr_in serv_addr;
  struct linger lingeropt;
  
  net_globals.no_file = 0;
  net_globals.con = NULL;

  /* Open a TCP socket (an Internet stream socket). */
  if ((net_globals.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    d_printf( "CHESSD: can't open stream socket\n");
    return -1;
  }
  /* Bind our local address so that the client can send to us */
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  /** added in an attempt to allow rebinding to the port **/

  opt = 1;
  setsockopt(net_globals.sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));
  opt = 1;
  setsockopt(net_globals.sockfd, SOL_SOCKET, SO_KEEPALIVE, (char *) &opt, sizeof(opt));
  lingeropt.l_onoff = 0;
  lingeropt.l_linger = 0;
  setsockopt(net_globals.sockfd, SOL_SOCKET, SO_LINGER, (char *) &lingeropt, sizeof(lingeropt));
  
/*
#ifdef DEBUG
  opt = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_DEBUG, (char *)&opt, sizeof(opt));
#endif
*/

  if (bind(net_globals.sockfd, (struct sockaddr *) & serv_addr, sizeof(serv_addr)) < 0) {
    d_printf( "CHESSD: can't bind local address.  errno=%d\n", errno);
    return -1;
  }
  opt = 1;
  ioctl(net_globals.sockfd, FIONBIO, &opt);
  fcntl(net_globals.sockfd, F_SETFD, 1);
  listen(net_globals.sockfd, 5);
  return 0;
}

void net_close(void)
{
	int i;
	for (i = 0; i < net_globals.no_file; i++) {
		if (net_globals.con[i]->status != NETSTAT_EMPTY)
			net_close_connection(net_globals.con[i]->fd);
	}
}

void net_close_connection(int fd)
{
  if (net_globals.con[fd]->status == NETSTAT_CONNECTED)
    net_flush_connection(fd);
  else
    d_printf( "Trying to close an unconnected socket?!?!\n");

  if (!remConnection(fd)) {
    if (fd > 2)
      if (close(fd) < 0) {
	d_printf( "Couldn't close socket %d - errno %d\n", fd, errno);
      }
  } else {
    d_printf( "Failed to remove connection (Socket %d)\n", fd);
  }
}

void turn_echo_on(int fd)
{
	const unsigned char wont_echo[] = {IAC, WONT, TELOPT_ECHO, '\0'};

	send(fd, (char *) wont_echo, strlen((char *) wont_echo), 0);
}

void turn_echo_off(int fd)
{
  static unsigned char will_echo[] = {IAC, WILL, TELOPT_ECHO, '\0'};

  send(fd, (char *) will_echo, strlen((char *) will_echo), 0);
}

static struct in_addr net_connected_host(int fd)
{
	int which;
	
	if ((which = findConnection(fd)) < 0) {
		static struct in_addr ip_zero;
		d_printf( "CHESSD: FD not in connection table!\n");
		return ip_zero;
	}
	return net_globals.con[which]->fromHost;
}

void select_loop(void )
{
	char com[MAX_STRING_LENGTH];
	struct sockaddr_in cli_addr;
	int cli_len = (int) sizeof(struct sockaddr_in);
	int fd, loop, nfound, lineComplete;
	fd_set readfds;
	struct timeval to;
	int current_socket;
	int timeout = 2;

#if 0
	m_check_all();
#endif

	/* we only want to get signals here. This tries to
	   ensure a clean shutdown on 'kill' */
	unblock_signal(SIGTERM);
	block_signal(SIGTERM);
	
	while ((fd = accept(net_globals.sockfd, (struct sockaddr *) & cli_addr, &cli_len)) != -1) {
		if (net_addConnection(fd, cli_addr.sin_addr)) {
			d_printf( "FICS is full.  fd = %d.\n", fd);
			psend_raw_file(fd, MESS_DIR, MESS_FULL);
			close(fd);
		} else {
			if (fd >= net_globals.no_file)
				d_printf("FICS (ngc2): Out of range fd!\n");
			else {
				fcntl(fd, F_SETFD, 1);
				process_new_connection(fd, net_connected_host(fd));
			}
		}
	}
  
	if (errno != EWOULDBLOCK)
		d_printf( "CHESSD: Problem with accept().  errno=%d\n", errno);

	net_flush_all_connections();
	
	if (net_globals.numConnections == 0) {
		sleep(1); /* prevent the server spinning */
	}

	FD_ZERO(&readfds);
	for (loop = 0; loop < net_globals.no_file; loop++)
		if (net_globals.con[loop]->status != NETSTAT_EMPTY)
			FD_SET(net_globals.con[loop]->fd, &readfds);
	
	to.tv_usec = 0;
	to.tv_sec = timeout;
	nfound = select(net_globals.no_file, &readfds, NULL, NULL, &to);
	for (loop = 0; loop < net_globals.no_file; loop++) {
		if (net_globals.con[loop]->status != NETSTAT_EMPTY) {
			fd = net_globals.con[loop]->fd;
		more_commands:
			lineComplete = readline2(com, fd);
			if (lineComplete == 0)	/* partial line: do nothing with it */
				continue;
			if (lineComplete > 0) {	/* complete line: process it */
				if (!timeseal_parse(com, net_globals.con[loop])) continue;
				if (process_input(fd, com) != COM_LOGOUT) {
					net_flush_connection(fd);
					goto more_commands;
				}
			}
			/* Disconnect anyone who gets here */
			process_disconnection(fd);
			net_close_connection(fd);
		}
	}

	if (process_heartbeat(&current_socket) == COM_LOGOUT) {
		process_disconnection(current_socket);
		net_close_connection(current_socket);
	}
}

