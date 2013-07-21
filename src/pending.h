/*
 pending.h

*/

#ifndef _PENDING_H
#define _PENDING_H

#define PEND_MATCH 0 
#define PEND_DRAW 1
#define PEND_ABORT 2
#define PEND_TAKEBACK 3
#define PEND_ADJOURN 4
#define PEND_SWITCH 5
#define PEND_SIMUL 6
#define PEND_PAUSE 7
#define PEND_PARTNER 8
#define PEND_BUGHOUSE 9
#define PEND_UNPAUSE 10
#define PEND_ALL -1

#define DO_DECLINE 0x01
#define DO_WITHDRAW 0x02

GENSTRUCT enum rated {UNRATED=0, RATED=1};

GENSTRUCT struct pending {/*Params 1=wt 2=winc 3=bt 4=binc 5=rated 6=white*/
	int type;              /* 7=type */
	int whoto; /* who is offered */
	int whofrom; /* who offered it */
	int wtime;
	int winc;
	int btime;
	int binc;
	enum rated rated;
	int seek_color; /* for matches */
	enum gametype game_type;
	int status; /* for seek/sought */
	char *category;
	char *board_type;
	struct pending *next;
};

#endif
