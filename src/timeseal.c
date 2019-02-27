/* 
   Copyright 2002 Andrew Tridgell <tridge@samba.org>
   
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

/*   glue code that connects the chess server to the external timeseal
 *   decoder */

#include "includes.h"

/*
  send a string to the decoder sub-process and return the modified (decoded) string 
  the return value is the decoded timestamp. It will be zero if the decoder didn't
  recognise the input as valid timeseal data
 */
static unsigned decode(char *s)
{
	char line[1024];
	char *p;
	unsigned t = 0;

        snprintf(line, 1000, "%s", s); // [HGM] limit length to 1000, to prevent crashing timeseal decoder
  
	/* send the encoded data to the decoder process */
	dprintf(timeseal_globals.decoder_conn, "%s\n", line);
	
	if (!fd_gets(line, sizeof(line), timeseal_globals.decoder_conn)) {
		d_printf("Bad result from timeseal decoder? (t=%u)\n", t);
		close(timeseal_globals.decoder_conn);
		timeseal_globals.decoder_conn = -1;
		return 0;
	}
	line[strlen(line)-1] = 0;

	p = strchr(line, ':');
	if (!p) {
		d_printf("Badly formed timeseal decoder line: [%s]\n", line);
		close(timeseal_globals.decoder_conn);
		timeseal_globals.decoder_conn = -1;
		return 0;
	}
	
	t = atoi(line);
	strcpy(s, p+2);
	
	return t;
}

/* 
   initialise the timeseal decoder sub-process
*/
void timeseal_init(const char *path)
{
	int fd[2];
	pid_t pid;
	
	/* use a socketpair to get a bi-directional pipe with large buffers */
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) != 0) {
		d_printf("Failed to create socket pair!\n");
		exit(1);
	}
	
	pid = fork();
	
	if (pid == 0) {
		close(fd[1]);
		close(0);
		close(1);
		close(2);
		dup2(fd[0], 0);
		dup2(fd[0], 1);
		open("/dev/null", O_WRONLY); /* stderr */
		execl(path, "[timeseal]", NULL);
		exit(1);
	}
	
	timeseal_globals.decoder_conn = fd[1];
	close(fd[0]);
}


/* 
   parse a command line from a user on *con that may be timeseal encoded. 
   return 1 if the command should be processed further, 0 if the command
   should be discarded
 */
int timeseal_parse(char *command, struct connection_t *con)
{
	unsigned t;
	
	/* do we have a decoder sub-process? */
	if (timeseal_globals.decoder_conn <= 0) return 1;
	
	/* are they using timeseal on this connection? */
        if (!con->timeseal_init && !con->timeseal) return 1;

#if 1
       {
       static char *key="Timestamp (FICS) v1.0 - programmed by Henrik Gram.";
#define SWAP(X,Y) { int h = buf[i+X]; buf[i+X] = buf[i+Y]; buf[i+Y] = h; }
               char buf[1024]; int i, l, offs;
               snprintf(buf, 1010, "%s\n", command);
               offs = command[strlen(command)-1] & 0x7F;
               l = strlen(buf);
               for(i=0; buf[i] != '\n'; i++)
                       buf[i] = ((buf[i] + 32) ^ key[(i+offs)%50]) & 0x7F;
               for(i=0; i<l; i+=12) {
                       SWAP(0,11); SWAP(4,7); SWAP(2,9);
               }
               t = 0;
               for(i=0; buf[i]; i++) if(buf[i] == 0x18) {
                       buf[i++] = 0;
                       for(l=i; buf[l]; l++) if(buf[l] == 0x19) {
                               buf[l] = 0; t = atoi(buf + i);
                               break;
                       }
                       break;
               }
               if(t) strcpy(command, buf);
       }
#else
	t = decode(command);
#endif
	
	if (t == 0) {
		/* this wasn't encoded using timeseal */
		d_printf("Non-timeseal data [%s]\n", command);
		con->timeseal_init = 0;
		return 1;
	}
	
	if (con->timeseal_init) {
		con->timeseal_init = 0;
		con->timeseal = 1;
		d_printf("Connected with timeseal %s\n", command);
		if (strncmp(command, "TIMESTAMP|", 10) == 0) {
			return 0;
		}
	}
	
	con->time = t;
	
	/* now check for the special move time tag */
	if (strcmp(command, "9") == 0) {
		int p = player_find(con->fd);
		struct player *pp = &player_globals.parray[p];
		if (p >= 0 && pp->game >= 0) {
			int g = pp->game;
			if (game_globals.garray[g].game_state.onMove != 
			    pp->side) {
				return 0;
			}
			if (pp->side == WHITE) {
				if (game_globals.garray[g].wTimeWhenReceivedMove == 0) {
					game_globals.garray[g].wTimeWhenReceivedMove = t;
				}
			} else {
				if (game_globals.garray[g].bTimeWhenReceivedMove == 0) {
					game_globals.garray[g].bTimeWhenReceivedMove = t;
				}
			}
			if (game_globals.garray[g].flag_pending != FLAG_NONE) {
				ExecuteFlagCmd(p, net_globals.con[pp->socket]);
			}
		}
		/* we have processed this special tag - don't process it further */
		return 0;
	}
	
	return 1;
}

/*
  used to call flag on players with timeseal
 */
void ExecuteFlagCmd(int p, struct connection_t *con)
{
	struct player *pp = &player_globals.parray[p];
	struct game *gg;

	if (pp->game == -1) {
		return;
	}

	gg = &game_globals.garray[pp->game];

	if (pp->side == WHITE) {
		gg->wRealTime -= con->time - gg->wTimeWhenReceivedMove;
		gg->wTimeWhenReceivedMove = con->time;
		if (gg->wRealTime < 0) {
			pcommand(pp->opponent, "flag");
		}
	} else if (pp->side == BLACK) {
		gg->bRealTime -= con->time - gg->bTimeWhenReceivedMove;
		gg->bTimeWhenReceivedMove = con->time;
		if (gg->bRealTime < 0) {
			pcommand(pp->opponent, "flag");
		}
	}

	game_update_time(pp->game);
	gg->flag_pending = FLAG_NONE;
}
