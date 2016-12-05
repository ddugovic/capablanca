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

#ifndef _GAMEDB_H
#define _GAMEDB_H

extern const char *bstr[];
extern const char *rstr[];

#define GAMEFILE_VERSION 5 
#define MAX_GLINE_SIZE 1024

#define REL_GAME 0
#define REL_SPOS 1
#define REL_REFRESH 2
#define REL_EXAMINE 3

GENSTRUCT enum gamestatus {GAME_EMPTY, GAME_NEW, GAME_ACTIVE, GAME_EXAMINE, GAME_SETUP};

/* Do not change the order of these - DAV */
GENSTRUCT enum gametype {TYPE_UNTIMED, TYPE_BLITZ, TYPE_STAND, TYPE_NONSTANDARD,
               TYPE_WILD, TYPE_LIGHT, TYPE_BUGHOUSE, TYPE_GOTHIC, TYPE_KNIGHTMATE,
               TYPE_CAPABLANCA, TYPE_SEIRAWAN};

#define NUM_GAMETYPES 11

/* OK, DAV, I'll try it another way. -- hersco */
enum ratetype {RATE_STAND, RATE_BLITZ, RATE_WILD, RATE_LIGHT, RATE_BUGHOUSE};
#define NUM_RATEDTYPE 5

#define FLAG_CHECKING -1
#define FLAG_NONE 0
#define FLAG_CALLED 1
#define FLAG_ABORT 2

GENSTRUCT enum gameend {
	END_CHECKMATE = 0,
	END_RESIGN,
	END_FLAG,
	END_AGREEDDRAW,
	END_REPETITION,
	END_50MOVERULE,
	END_ADJOURN,
	END_LOSTCONNECTION,
	END_ABORT,
	END_STALEMATE,
	END_NOTENDED,
	END_COURTESY,
	END_BOTHFLAG,
	END_NOMATERIAL,
	END_FLAGNOMATERIAL,
	END_ADJDRAW,
	END_ADJWIN,
	END_ADJABORT,
	END_COURTESYADJOURN,
	END_PERPETUAL,
	END_BARE		// [HGM] bare king
};

struct journal {
  char slot;
  char WhiteName[MAX_LOGIN_NAME];
  int WhiteRating;
  char BlackName[MAX_LOGIN_NAME];
  int BlackRating;
  char type[4];
  int t;
  int i;
  char eco[4];
  char ending[4];
  char result[8];
};


GENSTRUCT struct game {
	/* Not saved in game file */
	int revertHalfMove;
	int totalHalfMoves;
	int white;
	int black;
	int link;
	enum gamestatus status;
	int examHalfMoves;
	int examMoveListSize;  
	struct move_t *examMoveList; _LEN(examMoveListSize)    /* extra movelist for examine */

	unsigned startTime;    /* The relative time the game started  */
	unsigned lastMoveTime; /* Last time a move was made */
	unsigned lastDecTime;  /* Last time a players clock was decremented */
	int flag_pending;
	int wTimeWhenReceivedMove;
	int wTimeWhenMoved;
	int bTimeWhenReceivedMove;
	int bTimeWhenMoved;
	int wLastRealTime;
	int wRealTime;
	int bLastRealTime;
	int bRealTime;

	/* this is a dummy variable used to tell which bits are saved in the structure */
	unsigned not_saved_marker;
  
	/* Saved in the game file */
	enum gameend result;
	int winner;
	int wInitTime, wIncrement;
	int bInitTime, bIncrement;
	time_t timeOfStart;
	unsigned flag_check_time;
	int wTime;
	int bTime;
	int clockStopped;
	int rated;
	int private;
	enum gametype type;
	int passes; /* For simul's */
	int numHalfMoves;
	int moveListSize; /* Total allocated in *moveList */
	struct move_t *moveList; _LEN(moveListSize)      /* primary movelist */
	char FENstartPos[74]; _NULLTERM   /* Save the starting position. */
	struct game_state_t game_state;
	
	char white_name[MAX_LOGIN_NAME]; _NULLTERM   /* to hold the playername even after he disconnects */        
	char black_name[MAX_LOGIN_NAME]; _NULLTERM   
	int white_rating;
	int black_rating;
	char variant[80]; // [HGM] arbitrary variant name for sending to interface, derived from load-directory name
};

extern const char *TypeStrings[NUM_GAMETYPES];
//extern const char *TypeChars[NUM_GAMETYPES];

#endif

