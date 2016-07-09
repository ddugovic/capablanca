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

/*
   all global variiables in chessd have been moved here so that they can
   be maintained over a code reload. As long as the code reload does not change the
   shape or meaning of any of the following globals then the reload should work
*/


/* the EXTERN macro is used to allow ficsmain.c to declare real variables while
   other modules declare them as extern */
#ifndef EXTERN
#define EXTERN extern
#endif

/* from network.c */
GENSTRUCT struct net_globals {
	int sockfd; /* main listening socket */
	int numConnections; /* number of active connections */
	int no_file;   /* number of connections allocated */
	struct connection_t **con; _LEN(no_file)
};
EXTERN struct net_globals net_globals;

/* from gamedb.c */

GENSTRUCT struct game_globals {
	int g_num;
	struct game *garray; _LEN(g_num)
};
EXTERN struct game_globals game_globals;

/* from playerdb.c */
GENSTRUCT struct player_globals {
	int p_num;
	unsigned parray_size;
	struct player *parray; _LEN(parray_size)
};
EXTERN struct player_globals player_globals;

/* from command.c */
GENSTRUCT struct command_globals {
	time_t startuptime;
	int player_high;
	int game_high;
	int commanding_player;
};
EXTERN struct command_globals command_globals;

/* from gics.c */
GENSTRUCT struct gics_globals {
	struct userstat_type userstat;
};
EXTERN struct gics_globals gics_globals;

/* from seekproc.c */
GENSTRUCT struct seek_globals {
	int quota_time;
	struct pending *pendlist;
	unsigned max_ads;
	struct pending *ads; _LEN(max_ads)
};
EXTERN struct seek_globals seek_globals;

/* from timeseal.c */
GENSTRUCT struct timeseal_globals {
	int decoder_conn;
};
EXTERN struct timeseal_globals timeseal_globals;

/* a structure containing all globals, to make it easy to save
   globals for a reload
*/
GENSTRUCT struct all_globals {
	struct net_globals *net_globals;
	struct game_globals *game_globals;
	struct player_globals *player_globals;
	struct command_globals *command_globals;
	struct gics_globals *gics_globals;
	struct seek_globals *seek_globals;
	struct timeseal_globals *timeseal_globals;
};
