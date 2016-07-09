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

static int check_kings(struct game_state_t *gs);
static int get_empty_slot(void);
static int ReadGameAttrs_common(FILE *fp, int g, int version);
static long OldestHistGame(char *login);
static void RemoveHistGame(char *file, int maxlines);
static void write_g_out(int g, char *file, int maxlines, int isDraw, char *EndSymbol, char *name, time_t *now);
static int game_zero(int g);
 
const char *TypeStrings[NUM_GAMETYPES] = {"untimed", "blitz", "standard", 
					  "nonstandard", "wild", "lightning", 
					  "bughouse", "gothic", "knightmate", 
					  "capablanca"};

/* this method is awful! how about allocation as we need it and freeing
    afterwards! */
static int get_empty_slot(void)
{
	int i;

	for (i = 0; i < game_globals.g_num; i++) {
		if (game_globals.garray[i].status == GAME_EMPTY)
			return i;
	}
	game_globals.g_num++;
	game_globals.garray = (struct game *)realloc(game_globals.garray, sizeof(struct game) * game_globals.g_num);
	/* yeah great, bet this causes lag!  - DAV*/
	/* I have serious doubt of the truth to the above client- bugg */
	game_globals.garray[game_globals.g_num - 1].status = GAME_EMPTY;
	return game_globals.g_num - 1;
}

int game_new(void)
{
	int new = get_empty_slot();
	game_zero(new);
	return new;
}

static int game_zero(int g)
{
	ZERO_STRUCT(game_globals.garray[g]);

	game_globals.garray[g].white = -1;
	game_globals.garray[g].black = -1;

	game_globals.garray[g].status = GAME_NEW;
	game_globals.garray[g].link = -1;
	game_globals.garray[g].result = END_NOTENDED;
	game_globals.garray[g].type = TYPE_UNTIMED;
	game_globals.garray[g].game_state.gameNum = g;
	game_globals.garray[g].wInitTime = 300;	/* 5 minutes */
	game_globals.garray[g].wIncrement = 0;
	game_globals.garray[g].bInitTime = 300;	/* 5 minutes */
	game_globals.garray[g].bIncrement = 0;
	game_globals.garray[g].flag_pending = FLAG_NONE;
	strcpy(game_globals.garray[g].FENstartPos,INITIAL_FEN);
	return 0;
}

int game_free(int g)
{
	FREE(game_globals.garray[g].moveList);
	FREE(game_globals.garray[g].examMoveList);
	game_globals.garray[g].moveList = NULL;
	game_globals.garray[g].examMoveList = NULL;
	game_globals.garray[g].moveListSize = 0;
	game_globals.garray[g].examMoveListSize = 0;
	return 0;
}

int game_remove(int g)
{
	/* Should remove game from players observation list */
	game_free(g);
	game_zero(g);
	game_globals.garray[g].status = GAME_EMPTY;
	return 0;
}

/* old moves not stored now - uses smoves */
int game_finish(int g)
{
	player_game_ended(g);		/* Alert playerdb that game ended */
	game_remove(g);
	return 0;
}

void MakeFENpos (int g, char *FEN)
{
	strcpy(FEN, boardToFEN(g));
}

static char *game_time_str(int wt, int winc, int bt, int binc)
{
  static char tstr[50];

  if ((!wt) && (!winc)) {			/* Untimed */
    strcpy(tstr, "");
    return tstr;
  }
  if ((wt == bt) && (winc == binc)) {
    sprintf(tstr, " %d %d", wt, winc);
  } else {
    sprintf(tstr, " %d %d : %d %d", wt, winc, bt, binc);
  }
  return tstr;
}

const char *bstr[] = {"untimed", "blitz", "standard", "non-standard", "wild", "lightning", "Bughouse", "Gothic", "Knightmate", "Capablanca"};

const char *rstr[] = {"unrated", "rated"};

char *game_str(int rated, int wt, int winc, int bt, int binc,
		       char *cat, char *board)
{
  static char tstr[200];

  if (cat && cat[0] && board && board[0] &&
      (strcmp(cat, "standard") || strcmp(board, "standard"))) {
    sprintf(tstr, "%s %s%s Loaded from %s/%s",
	    rstr[rated],
	    bstr[game_isblitz(wt / 60, winc, bt / 60, binc, cat, board)],
	    game_time_str(wt / 60, winc, bt / 60, binc),
	    cat, board);
  } else {
    sprintf(tstr, "%s %s%s",
	    rstr[rated],
	    bstr[game_isblitz(wt / 60, winc, bt / 60, binc, cat, board)],
	    game_time_str(wt / 60, winc, bt / 60, binc));
  }
  return tstr;
}

int game_isblitz(int wt, int winc, int bt, int binc,
			 char *cat, char *board)
{
  int total;

  if(cat && cat[0]) {
    if (!strcmp(cat, "bughouse"))
      return TYPE_BUGHOUSE;
    if (!strcmp(cat, "gothic"))
      return TYPE_GOTHIC;
    if (!strcmp(cat, "knightmate"))
      return TYPE_KNIGHTMATE;
    if (!strcmp(cat, "capablanca"))
      return TYPE_CAPABLANCA;
    if (board && board[0] && strcmp(board, "0")) {
      if (!strcmp(cat, "wild"))
        return TYPE_WILD;
      if (strcmp(cat, "standard") || strcmp(board, "standard"))
        return TYPE_NONSTANDARD;
    }
    return TYPE_WILD;
  }

  if ((wt == 0) || (bt == 0))
			/* nonsense if one is timed and one is not */
    return TYPE_UNTIMED;

  if ((wt != bt) || (winc != binc))
    return TYPE_NONSTANDARD;
  total = wt * 60 + winc * 40;
  if (total < 180)		/* 3 minute */
    return TYPE_LIGHT;
  if (total >= 900)		/* 15 minutes */
    return TYPE_STAND;
  else
    return TYPE_BLITZ;
}

void send_board_to(int g, int p)
{
  struct player *pp = &player_globals.parray[p];
  char *b;
  int side;
  int relation;

/* since we know g and p, figure out our relationship to this game */

  side = WHITE;
  if ((game_globals.garray[g].status == GAME_EXAMINE) || (game_globals.garray[g].status == GAME_SETUP)) {
    if (pp->game == g) {
      relation = 2;
    } else {
      relation = -2;
    }
  } else {
    if (pp->game == g) {
      side = pp->side;
      relation = ((side == game_globals.garray[g].game_state.onMove) ? 1 : -1);
    } else {
      relation = 0;
    }
  }

  if (CheckPFlag(p, PFLAG_FLIP)) {		/* flip board? */
    if (side == WHITE)
      side = BLACK;
    else
      side = WHITE;
  }
  game_update_time(g);
  b = board_to_string(game_globals.garray[g].white_name,
		      game_globals.garray[g].black_name,
		      game_globals.garray[g].wTime,
		      game_globals.garray[g].bTime,
		      &game_globals.garray[g].game_state,
		      (game_globals.garray[g].status == GAME_EXAMINE || game_globals.garray[g].status == GAME_SETUP) ?
		      game_globals.garray[g].examMoveList : game_globals.garray[g].moveList,
		      pp->style,
		      side, relation, p);
  Bell(p);

  if (pp->game == g && net_globals.con[pp->socket]->timeseal) {
    pprintf_noformat(p, "\n%s\n[G]\n", b);
  } else {
    pprintf_noformat(p, "\n%s", b);
  }

  if (p != command_globals.commanding_player) {
	  send_prompt(p);
  }
}

void send_boards(int g)
{
	int p,p1;
	struct simul_info_t *simInfo = player_globals.parray[game_globals.garray[g].white].simul_info;
	int which_board = -1;

	/* Begin code added 3/28/96 - Figbert */

	if ((game_globals.garray[g].status != GAME_SETUP) && 
	    (check_kings(&game_globals.garray[g].game_state))) {
		d_printf( "Game has invalid number of kings.  Aborting...\n");
{ int f, r;
d_printf("files = %d, ranks = %d\n", game_globals.garray[g].game_state.files, game_globals.garray[g].game_state.ranks);
for(r=0; r<game_globals.garray[g].game_state.ranks; r++) {
  for(f=0; f<game_globals.garray[g].game_state.files; f++)
  	d_printf("b[%d][%d]=%d\n",f,r,game_globals.garray[g].game_state.board[f][r]);
}
}
		game_finish(g);
		
		for (p = 0; p < player_globals.p_num; p++) {
			struct player *pp = &player_globals.parray[p];
			if (pp->status == PLAYER_EMPTY)
				continue;
			if (pp->game == g) {
				
				p1 = pp->opponent;
				
				if (p1 >= 0 && player_globals.parray[p1].game == g) {
					pprintf (p1, "Disconnecting you from game number %d.\n", g+1);
					player_globals.parray[p1].game = -1;
				}
				
				pprintf (p, "Disconnecting you from game number %d.\n", g+1);
				pp->game = -1;
			}
		}
		return;
	}
	
	/* End code added 3/28/96 - Figbert */
	
	if (simInfo != NULL)
		which_board = simInfo->boards[simInfo->onBoard];
	
	if ((simInfo == NULL) || (which_board == g)) {
		for (p = 0; p < player_globals.p_num; p++) {
			struct player *pp = &player_globals.parray[p];
			if (pp->status == PLAYER_EMPTY)
				continue;
			if (player_is_observe(p, g) || (pp->game == g))
				send_board_to(g, p);
		}
	}
}

void game_update_time(int g)
{
	struct game *gg;
	unsigned now, timesince;
	
	if (g == -1) {
		return;
	}

	gg = &game_globals.garray[g];

	/* no update on first move */
	if (gg->game_state.moveNum == 1) 
		return;
	if (gg->clockStopped)
		return;
	if (gg->type == TYPE_UNTIMED)
		return;
	now = tenth_secs();
	timesince = now - gg->lastDecTime;
	if (gg->game_state.onMove == WHITE) {
		gg->wTime -= timesince;
	} else {
		gg->bTime -= timesince;
	}
	gg->lastDecTime = now;
}

#if 0
static void game_update_times(void)
{
	int g;
	
	for (g = 0; g < game_globals.g_num; g++) {
		if (game_globals.garray[g].status != GAME_ACTIVE)
			continue;
		if (game_globals.garray[g].clockStopped)
			continue;
		game_update_time(g);
	}
}
#endif

char *EndString(int g, int personal)
{
  static char endstr[200];
  char *blackguy, *whiteguy;
  static char blackstr[] = "Black";
  static char whitestr[] = "White";

  blackguy = (personal ? game_globals.garray[g].black_name : blackstr);
  whiteguy = (personal ? game_globals.garray[g].white_name : whitestr);

  switch (game_globals.garray[g].result) {
  case END_CHECKMATE:
    sprintf(endstr, "%s checkmated",
	    game_globals.garray[g].winner == WHITE ? blackguy : whiteguy);
    break;
  case END_BARE:
    sprintf(endstr, "%s bared",
	    game_globals.garray[g].winner == WHITE ? blackguy : whiteguy);
    break;
  case END_PERPETUAL:
    sprintf(endstr, "%s perpetually checking",
	    game_globals.garray[g].winner == WHITE ? blackguy : whiteguy);
    break;
  case END_RESIGN:
    sprintf(endstr, "%s resigned",
	    game_globals.garray[g].winner == WHITE ? blackguy : whiteguy);
    break;
  case END_FLAG:
    sprintf(endstr, "%s ran out of time",
	    game_globals.garray[g].winner == WHITE ? blackguy : whiteguy);
    break;
  case END_AGREEDDRAW:
    sprintf(endstr, "Game drawn by mutual agreement");
    break;
  case END_BOTHFLAG:
    sprintf(endstr, "Game drawn because both players ran out of time");
    break;
  case END_REPETITION:
    sprintf(endstr, "Game drawn by repetition");
    break;
  case END_50MOVERULE:
    sprintf(endstr, "Draw by the 50 move rule");
    break;
  case END_ADJOURN:
    sprintf(endstr, "Game adjourned by mutual agreement");
    break;
  case END_LOSTCONNECTION:
    sprintf(endstr, "%s lost connection, game adjourned",
	    game_globals.garray[g].winner == WHITE ? whiteguy : blackguy);
    break;
  case END_ABORT:
    sprintf(endstr, "Game aborted by mutual agreement");
    break;
  case END_STALEMATE:
    sprintf(endstr, "Stalemate.");
    break;
  case END_NOTENDED:
    sprintf(endstr, "Still in progress");
    break;
  case END_COURTESY:
    sprintf(endstr, "Game courtesyaborted by %s",
	    game_globals.garray[g].winner == WHITE ? whiteguy : blackguy);
    break;
  case END_COURTESYADJOURN:
    sprintf(endstr, "Game courtesyadjourned by %s",
	    game_globals.garray[g].winner == WHITE ? whiteguy : blackguy);
    break;
  case END_NOMATERIAL:
    sprintf(endstr, "Game drawn because neither player has mating material");
    break;
  case END_FLAGNOMATERIAL:
    sprintf(endstr, "%s ran out of time and %s has no material to mate",
	    game_globals.garray[g].winner == WHITE ? blackguy : whiteguy,
	    game_globals.garray[g].winner == WHITE ? whiteguy : blackguy);
    break;
  case END_ADJDRAW:
    sprintf(endstr, "Game drawn by adjudication");
    break;
  case END_ADJWIN:
    sprintf(endstr, "%s wins by adjudication",
	    game_globals.garray[g].winner == WHITE ? whiteguy : blackguy);
    break;
  case END_ADJABORT:
    sprintf(endstr, "Game aborted by adjudication");
    break;
  default:
    sprintf(endstr, "???????");
    break;
  }

  return (endstr);
}

const char *EndSym(int g)
{
	static const char *symbols[] = {"1-0", "0-1", "1/2-1/2", "*"};

	switch (game_globals.garray[g].result) {
	case END_CHECKMATE:
	case END_RESIGN:
	case END_FLAG:
	case END_ADJWIN:
	case END_BARE:
	case END_PERPETUAL:
		return ((game_globals.garray[g].winner == WHITE) ? symbols[0] : symbols[1]);
		break;
	case END_AGREEDDRAW:
	case END_BOTHFLAG:
	case END_REPETITION:
	case END_50MOVERULE:
	case END_STALEMATE:
	case END_NOMATERIAL:
	case END_FLAGNOMATERIAL:
	case END_ADJDRAW:
		return (symbols[2]);
		break;
	default:
		break;
	}

	return (symbols[3]);
}

/* This should be enough to hold any game up to at least 8000 moves
 * If we overwrite this, the server will crash :-).
 */
/* 8000? who you trying to kid? this is awful - enough for 600 halfs :) -DAV*/
#define GAME_STRING_LEN 19000
static char gameString[GAME_STRING_LEN];
char *movesToString(int g, int pgn)
{
  char tmp[160];
  int wr, br;
  int i, col;
  time_t curTime;
  struct move_t *moves;

  if (game_globals.garray[g].status == GAME_EXAMINE || game_globals.garray[g].status == GAME_SETUP)
    moves = game_globals.garray[g].examMoveList;
  else moves = game_globals.garray[g].moveList;

  wr = game_globals.garray[g].white_rating;
  br = game_globals.garray[g].black_rating;
  

  curTime = untenths(game_globals.garray[g].timeOfStart);

  if (pgn) {
    sprintf(gameString,
	    "\n[Event \"%s %s %s game\"]\n"
	    "[Site \"%s, %s\"]\n",
	    config_get_tmp("SERVER_NAME"),
	    rstr[game_globals.garray[g].rated], /*bstr[game_globals.garray[g].type],*/ 
	    game_globals.garray[g].variant, // [HGM] allow more variation in game_types
	    config_get_tmp("SERVER_NAME"),
	    config_get_tmp("SERVER_LOCATION"));
    strftime(tmp, sizeof(tmp),
	     "[Date \"%Y.%m.%d\"]\n"
	     "[Time \"%H:%M:%S\"]\n",
	     localtime((time_t *) &curTime));
    strcat(gameString, tmp);
    sprintf(tmp,
	    "[Round \"-\"]\n"
	    "[White \"%s\"]\n"
	    "[Black \"%s\"]\n"
	    "[WhiteElo \"%d\"]\n"
	    "[BlackElo \"%d\"]\n",
	    game_globals.garray[g].white_name, game_globals.garray[g].black_name, wr, br);
    strcat(gameString, tmp);
    if(game_globals.garray[g].game_state.variant[0]) { // [HGM] variant: print variant tag
        sprintf(tmp,
	    "[Variant \"%s\"]\n",
	    game_globals.garray[g].game_state.variant);
        strcat(gameString, tmp);
    }
    if(game_globals.garray[g].game_state.setup) { // [HGM] setup: print the FEN
        sprintf(tmp,
	    "[Setup \"1\"]\n"
	    "[FEN \"%s\"]\n",
	    game_globals.garray[g].FENstartPos);
        strcat(gameString, tmp);
    }
    sprintf(tmp,
	    "[TimeControl \"%d+%d\"]\n"
	    "[Mode \"ICS\"]\n"
	    "[Result \"%s\"]\n\n",
	    game_globals.garray[g].wInitTime / 10, game_globals.garray[g].wIncrement / 10, EndSym(g));
    strcat(gameString, tmp);

    col = 0;
    for (i = 0; i < game_globals.garray[g].numHalfMoves; i++) {
      if (moves[i].color == WHITE) {
	if ((col += sprintf(tmp, "%d. ", (i+1) / 2 + 1)) > 70) {
	  strcat(gameString, "\n");
	  col = 0;
	}
	strcat(gameString, tmp);
      } else if (i==0) {
        strcat (tmp, "1. ... ");
        col += 7;
      }
      if ((col += sprintf(tmp, "%s ", moves[i].algString)) > 70) {
	strcat(gameString, "\n");
	col = 0;
      }
      strcat(gameString, tmp);
      if(moves[i].depth > 0) { // [HGM] computer game, add {score/depth} comment
	if ((col += sprintf(tmp, "{%s%.2f/%d} ", moves[i].score > 0 ? "+" : "",
				moves[i].score, moves[i].depth)) > 70) {
	  strcat(gameString, "\n");
	  col = 0;
	}
	strcat(gameString, tmp);
      }
    }
    strcat(gameString, "\n");

  } else {

    sprintf(gameString, "\n%s ", game_globals.garray[g].white_name);
    if (wr > 0) {
      sprintf(tmp, "(%d) ", wr);
    } else {
      sprintf(tmp, "(UNR) ");
    }
    strcat(gameString, tmp);
    sprintf(tmp, "vs. %s ", game_globals.garray[g].black_name);
    strcat(gameString, tmp);
    if (br > 0) {
      sprintf(tmp, "(%d) ", br);
    } else {
      sprintf(tmp, "(UNR) ");
    }
    strcat(gameString, tmp);
    strcat(gameString, "--- ");
    strcat(gameString, strltime(&curTime));
    if (game_globals.garray[g].rated) {
      strcat(gameString, "\nRated ");
    } else {
      strcat(gameString, "\nUnrated ");
    }
//    strcat (gameString, TypeStrings[game_globals.garray[g].type]);
    strcat (gameString, game_globals.garray[g].variant);
    strcat(gameString, " match, initial time: ");
    if ((game_globals.garray[g].bInitTime != game_globals.garray[g].wInitTime) || (game_globals.garray[g].wIncrement != game_globals.garray[g].bIncrement)) { /* different starting times */ 
      sprintf(tmp, "%d minutes, increment: %d seconds AND %d minutes, increment: %d seconds.\n\n", game_globals.garray[g].wInitTime / 600, game_globals.garray[g].wIncrement / 10, game_globals.garray[g].bInitTime / 600, game_globals.garray[g].bIncrement / 10);
    } else {
      sprintf(tmp, "%d minutes, increment: %d seconds.\n\n", game_globals.garray[g].wInitTime / 600, game_globals.garray[g].wIncrement / 10);
    }
    strcat(gameString, tmp);

    if(game_globals.garray[g].game_state.setup) { // [HGM] setup: print the initial position board
	char *q; struct game_state_t initial_gs; struct move_t ml[600]; int r, f;

	initial_gs.gameNum = g;
	initial_gs.wkrmoved = 0; // [HGM] for some reason calling reset_board_vars() does not work here
	initial_gs.wqrmoved = 0; //       so I just duplicated the code and pasted it here...
	initial_gs.wkmoved = 0;
	initial_gs.bkrmoved = 0;
	initial_gs.bqrmoved = 0;
	initial_gs.bkmoved = 0;
	initial_gs.onMove = WHITE;
	initial_gs.lastIrreversable = -1;
	initial_gs.files = game_globals.garray[g].game_state.files;
	initial_gs.ranks = game_globals.garray[g].game_state.ranks;
	initial_gs.holdings = game_globals.garray[g].game_state.holdings;
	strcpy(initial_gs.variant, game_globals.garray[g].game_state.variant);
        for (f = 0; f < 2; f++) {
          for (r = 0; r < initial_gs.files; r++)
             initial_gs.ep_possible[f][r] = 0;
          for (r = PAWN; r <= PIECES-1; r++)
            initial_gs.holding[f][r-PAWN] = 0;
	}
	FEN_to_board(game_globals.garray[g].FENstartPos ,&initial_gs);

	kludgeFlag = 1; // [HGM] setup: this is not thread safe. Must it be???
	q = board_to_string(
		      game_globals.garray[g].white_name,
		      game_globals.garray[g].black_name,
		      game_globals.garray[g].wTime,
		      game_globals.garray[g].bTime,
		      &initial_gs,
		      ml,
		      11,
		      WHITE, 0, 0);
	kludgeFlag = 0;

        strcat(gameString, q);
        strcat(gameString, "\n");
    }
    sprintf(tmp, "Move  %-19s%-19s\n", game_globals.garray[g].white_name, game_globals.garray[g].black_name);
    strcat(gameString, tmp);
    strcat(gameString, "----  ----------------   ----------------\n");

    for (i = 0; i < game_globals.garray[g].numHalfMoves; i += 2) {
      if (i==0 && (moves[i].color == BLACK)) {
	sprintf(tmp, "%3d.  %-16s   %-16s\n", (i+1)/2 + 1, "...",
                     move_and_time(&moves[i]));
        i--;
      } else if (i + 1 < game_globals.garray[g].numHalfMoves) {
	sprintf(tmp, "%3d.  %-16s   ", (i+1)/2 + 1, move_and_time(&moves[i]));
	strcat(gameString, tmp);
	sprintf(tmp, "%-16s\n", move_and_time(&moves[i+1]));
      } else {
	sprintf(tmp, "%3d.  %-16s\n", (i+1)/2 + 1, move_and_time(&moves[i]));
      }
      strcat(gameString, tmp);
      if (strlen(gameString) > GAME_STRING_LEN - 100) {	/* Bug out if getting
							   close to filling this
							   string */
	return gameString;
      }
    }

    strcat(gameString, "      ");
  }

  sprintf(tmp, "{%s} %s\n", EndString(g, 0), EndSym(g));
  strcat(gameString, tmp);

  return gameString;
}

void game_disconnect(int g, int p)
{
  game_ended(g, (game_globals.garray[g].white == p) ? WHITE : BLACK, END_LOSTCONNECTION);
}

int CharToPiece(char c, char *variant)
{

  if(variant) {
    if(!strcmp(variant, "shogi")) {
      switch(c) {
	case 'N':
	  return W_HONORABLEHORSE;
	case 'n':
	  return B_HONORABLEHORSE;
	case 'L':
	  return W_LANCE;
	case 'l':
	  return B_LANCE;
	case 'S':
	  return W_SILVER;
	case 's':
	  return B_SILVER;
	case 'G':
	  return W_GOLD;
	case 'g':
	  return B_GOLD;
      }
    } else if(!strcmp(variant, "xiangqi")) {
      switch(c) {
	case 'A':
	  return W_MANDARIN;
	case 'a':
	  return B_MANDARIN;
	case 'H':
	  return W_HORSE;
	case 'h':
	  return B_HORSE;
	case 'C':
	  return W_CANNON;
	case 'c':
	  return B_CANNON;
      }
    } else if(!strcmp(variant, "super")) {
      switch(c) {
	case 'E':
	  return W_EMPRESS;
	case 'e':
	  return B_EMPRESS;
	case 'S':
	  return W_PRINCESS;
	case 's':
	  return B_PRINCESS;
	case 'Z':
	  return W_AMAZON;
	case 'z':
	  return B_AMAZON;
	case 'V':
	  return W_CENTAUR;
	case 'v':
	  return B_CENTAUR;
      }
    } else if(!strcmp(variant, "spartan")) {
      switch(c) {
	case 'w':
	  return B_WARLORD;
	case 'g':
	  return B_GENERAL;
	case 'l':
	  return B_LIEUTENANT;
	case 'c':
	  return B_CAPTAIN;
	case 'h':
	  return B_HOPLITE;
      }
    }
  }
  switch (c) {
  case 'P':
    return W_PAWN;
  case 'p':
    return B_PAWN;
  case 'N':
    return W_KNIGHT;
  case 'n':
    return B_KNIGHT;
  case 'B':
    return W_BISHOP;
  case 'b':
    return B_BISHOP;
  case 'R':
    return W_ROOK;
  case 'r':
    return B_ROOK;
  case 'A':
    return W_CARDINAL;
  case 'a':
    return B_CARDINAL;
  case 'C':
    return W_MARSHALL;
  case 'c':
    return B_MARSHALL;
  case 'M':
    return W_MAN;
  case 'm':
    return B_MAN;
  case 'Q':
    return W_QUEEN;
  case 'q':
    return B_QUEEN;
  case 'E':
    if(!strcmp(variant, "seirawan")) return W_ELEPHANT;
    return W_ELEPHANT;
  case 'e':
    if(!strcmp(variant, "seirawan")) return B_ELEPHANT;
    return B_ELEPHANT;
  case 'H':
    return W_HAWK;
  case 'h':
    return B_HAWK;
  case 'K':
    return W_KING;
  case 'k':
    return B_KING;
  default:
    return NOPIECE;
  }
}

char PieceToChar(int piece)
{
  switch (piece) {
    case W_PAWN:return 'P';
  case B_PAWN:
    return 'p';
  case W_KNIGHT:
  case W_HONORABLEHORSE:
    return 'N';
  case B_KNIGHT:
  case B_HONORABLEHORSE:
    return 'n';
  case W_BISHOP:
    return 'B';
  case B_BISHOP:
    return 'b';
  case W_ROOK:
    return 'R';
  case B_ROOK:
    return 'r';
  case W_CARDINAL:
  case W_MANDARIN:
  case W_ALFIL2:
    return 'A';
  case B_CARDINAL:
  case B_MANDARIN:
  case B_ALFIL2:
    return 'a';
  case W_CANNON:
  case W_MARSHALL:
    return 'C';
  case B_CAPTAIN:
  case B_CANNON:
  case B_MARSHALL:
    return 'c';
  case W_MAN:
  case W_MINISTER:
    return 'M';
  case B_MAN:
  case B_MINISTER:
    return 'm';
  case W_QUEEN:
    return 'Q';
  case B_QUEEN:
    return 'q';
  case W_SELEPHANT:
  case W_ELEPHANT:
  case W_EMPRESS:
    return 'E';
  case B_SELEPHANT:
  case B_ELEPHANT:
  case B_EMPRESS:
    return 'e';
  case W_ALFIL:
    return 'B';
  case B_ALFIL:
    return 'b';
  case W_FERZ:
    return 'Q';
  case B_FERZ:
    return 'q';
  case W_FERZ2:
    return 'F';
  case B_FERZ2:
    return 'f';
  case W_WAZIR:
  case W_WOODY:
    return 'W';
  case B_WARLORD:
  case B_WAZIR:
  case B_WOODY:
    return 'w';
  case W_HAWK:
  case W_HORSE:
  case W_PRIESTESS:
  case W_NIGHTRIDER:
    return 'H';
  case B_HAWK:
  case B_HORSE:
  case B_HOPLITE:
  case B_PRIESTESS:
  case B_NIGHTRIDER:
    return 'h';
  case W_SILVER:
  case W_PRINCESS:
  case W_MAN2:
    return 'S';
  case B_SILVER:
  case B_PRINCESS:
  case B_MAN2:
    return 's';
  case W_GOLD:
  case W_MASTODON:
    return 'G';
  case B_GOLD:
  case B_GENERAL:
  case B_MASTODON:
    return 'g';
  case W_AMAZON:
    return 'Z';
  case B_AMAZON:
    return 'z';
  case W_CENTAUR:
    return 'V';
  case B_CENTAUR:
    return 'v';
  case W_KING:
    return 'K';
  case B_KING:
    return 'k';
  case W_LANCE:
    return 'L';
  case B_LIEUTENANT:
  case B_LANCE:
    return 'l';
  default:
    return ' ';
  }
}

/*
  write a new style generic game file
*/
static void WriteGameFile_v100(FILE * fp, int g)
{
	struct game gg = game_globals.garray[g];
	const char *s;

	/* zero any elements we don't want to save */
	memset(&gg, 0, offsetof(struct game, not_saved_marker));
	gg.game_state.gameNum = 0;

	/* marshall it into a string */
	s = marshall_game(&gg);
	if (!s) {
		d_printf("Unable to marshall game structure!\n");
		return;
	}

	/* and save it */
	fprintf(fp, "v 100\n%s\n", s);
	free(s);
}


/*
  read a game file using the new generic and extensible format 
*/
static int ReadGameAttrs_v100(FILE *fp, int g)
{
	char *s, *s2;       
	struct game *gg = &game_globals.garray[g];
	struct game g1;

	s = fd_load(fileno(fp), NULL);
	if (!s) {
		d_printf("Error reading game file!\n");
		return -1;
	}

	/* skip first line */
	s2 = strchr(s, '\n');

	/* remember the game state for later */
	g1 = *gg;

	/* the marshaller doesn't save zero elements, but some elements don't
	   default to zero. Make sure they get the right value */
	memset(&gg->not_saved_marker, 0, 
	       sizeof(struct game) - offsetof(struct game, not_saved_marker));
	gg->game_state.gameNum = g;

	if (!s2 || unmarshall_game(gg, s2) != 0) {
		d_printf("Error unmarshalling game data!\n");
		free(s);
		return -1;
	}
	free(s);

	/* when examining we are not supposed to restore the game
	   state, so put it back here */
	if (g1.status == GAME_EXAMINE || g1.status == GAME_SETUP) { 
		gg->game_state = g1.game_state;
	}
	gg->status = g1.status;

	/* cope with continuing a game with timeseal that was started without it */
	gg->wRealTime = gg->wTime * 100;
	gg->bRealTime = gg->bTime * 100;

	return 0;
}


static int ReadGameAttrs_common(FILE * fp, int g,int version)
{
	if (version == 9 || version == 100) {
		return ReadGameAttrs_v100(fp, g);
	}

	return ReadGameAttrs_old(fp, g, version);
}

int ReadGameAttrs_exam(FILE * fp, int g)
{
  int version = 0;
  char line[MAX_GLINE_SIZE];

  fgets(line, MAX_GLINE_SIZE - 1, fp);

   if (line[0] == 'v') {
    sscanf(line, "%*c %d", &version);

    if (version < 5)
      return -1;
    else
      return ReadGameAttrs_common(fp,g,version);
  }

   else return -1;
}

int ReadGameAttrs(FILE * fp, int g)
{
	int version = 0;
	char line[MAX_GLINE_SIZE];

	fgets(line, MAX_GLINE_SIZE - 1, fp);

	if (line[0] == 'v') {
		sscanf(line, "%*c %d", &version);
	}
	return ReadGameAttrs_common(fp,g,version);
}

int game_read(int g, int wp, int bp)
{
  FILE *fp;
  int piece;

  game_globals.garray[g].white = wp;
  game_globals.garray[g].black = bp;
/*  game_globals.garray[g].old_white = -1;
    game_globals.garray[g].old_black = -1;
*/
  game_globals.garray[g].moveListSize = 0;
  game_globals.garray[g].game_state.gameNum = g;
  strcpy(game_globals.garray[g].white_name, player_globals.parray[wp].name);
  strcpy(game_globals.garray[g].black_name, player_globals.parray[bp].name);
  if (game_globals.garray[g].type == TYPE_BLITZ) {
    game_globals.garray[g].white_rating = player_globals.parray[wp].b_stats.rating;
    game_globals.garray[g].black_rating = player_globals.parray[bp].b_stats.rating;
  } else if (game_globals.garray[g].type == TYPE_WILD ||
	     game_globals.garray[g].type == TYPE_KNIGHTMATE ||
	     game_globals.garray[g].type == TYPE_CAPABLANCA ||
	     game_globals.garray[g].type == TYPE_GOTHIC) {
    game_globals.garray[g].white_rating = player_globals.parray[wp].w_stats.rating;
    game_globals.garray[g].black_rating = player_globals.parray[bp].w_stats.rating;
  } else if (game_globals.garray[g].type == TYPE_LIGHT) {
    game_globals.garray[g].white_rating = player_globals.parray[wp].l_stats.rating;
    game_globals.garray[g].black_rating = player_globals.parray[bp].l_stats.rating;
  } else if (game_globals.garray[g].type == TYPE_BUGHOUSE) {
    game_globals.garray[g].white_rating = player_globals.parray[wp].bug_stats.rating;
    game_globals.garray[g].black_rating = player_globals.parray[bp].bug_stats.rating;
  } else {
    game_globals.garray[g].white_rating = player_globals.parray[wp].s_stats.rating;
    game_globals.garray[g].black_rating = player_globals.parray[bp].s_stats.rating;
  }
  fp = fopen_p(ADJOURNED_DIR "/%c/%s-%s", "r", player_globals.parray[wp].login[0],
	     player_globals.parray[wp].login, player_globals.parray[bp].login);
  if (!fp) {
    return -1;
  }
  for (piece=PAWN; piece < KING; piece++) {
    game_globals.garray[g].game_state.holding[0][piece-PAWN]
      = game_globals.garray[g].game_state.holding[1][piece-PAWN] = 0;
  }
  if (ReadGameAttrs(fp, g) < 0) {
    fclose(fp);
    return -1;
  }
  fclose(fp);

  if (game_globals.garray[g].result == END_ADJOURN
      || game_globals.garray[g].result == END_COURTESYADJOURN)
    game_globals.garray[g].result = END_NOTENDED;
  game_globals.garray[g].status = GAME_ACTIVE;
  game_globals.garray[g].startTime = tenth_secs();
  game_globals.garray[g].lastMoveTime = game_globals.garray[g].startTime;
  game_globals.garray[g].lastDecTime = game_globals.garray[g].startTime;

  /* cope with continuing a game with timeseal that was started without it */
  game_globals.garray[g].wRealTime = game_globals.garray[g].wTime * 100;
  game_globals.garray[g].bRealTime = game_globals.garray[g].bTime * 100;

  /* Need to do notification and pending cleanup */
  return 0;
}

int game_delete(int wp, int bp)
{
  char fname[MAX_FILENAME_SIZE];
  char lname[MAX_FILENAME_SIZE];

  sprintf(fname, ADJOURNED_DIR "/%c/%s-%s", player_globals.parray[wp].login[0],
	  player_globals.parray[wp].login, player_globals.parray[bp].login);
  sprintf(lname, ADJOURNED_DIR "/%c/%s-%s", player_globals.parray[bp].login[0],
	  player_globals.parray[wp].login, player_globals.parray[bp].login);
  unlink(fname);
  unlink(lname);
  return 0;
}

int game_save(int g)
{
	FILE *fp;
	struct player *wp, *bp;
	struct game *gg = &game_globals.garray[g];
	char *fname;

	wp = &player_globals.parray[gg->white];
	bp = &player_globals.parray[gg->black];
	asprintf(&fname, ADJOURNED_DIR "/%c/%s-%s", wp->login[0],
		 wp->login, bp->login);
	fp = fopen_p("%s", "w", fname);
	if (!fp) {
		d_printf("CHESSD: Problem opening adjourn file %s-%s for write\n", 
			  wp->login, bp->login);
		free(fname);
		return -1;
	}
	WriteGameFile_v100(fp, g);
	fclose(fp);
	/* Create link for easier stored game finding */
	if (bp->login[0] != wp->login[0]) {
		char *lname;
		asprintf(&lname, ADJOURNED_DIR "/%c/%s-%s", bp->login[0],
			 wp->login, bp->login);
		link(fname, lname);
		free(lname);
	}
	free(fname);
	return 0;
}

static long OldestHistGame(char *login)
{
	FILE *fp;
	long when;
	
	fp = fopen_p(STATS_DIR "/player_data/%c/%s.%s", "r",
		     login[0], login, STATS_GAMES);
	if (fp == NULL) {
		fp = fopen_p(STATS_DIR "/player_data/%c/.rem.%s.%s", "r",
			     login[0], login, STATS_GAMES);
	}
	if (!fp) return 0;

	fscanf(fp, "%*d %*c %*d %*c %*d %*s %*s %*d %*d %*d %*d %*s %*s %ld",
	       &when);
	fclose(fp);
	return when;
}

static void RemoveHistGame(char *file, int maxlines)
{
  FILE *fp;
  char GameFile[MAX_FILENAME_SIZE];
  char Opponent[MAX_LOGIN_NAME];
  char line[MAX_LINE_SIZE];
  long When, oppWhen;
  int count = 0;

  fp = fopen_p("%s", "r", file);
  if (fp == NULL)
    return;

  fgets(line, MAX_LINE_SIZE - 1, fp);
  sscanf(line, "%*d %*c %*d %*c %*d %s %*s %*d %*d %*d %*d %*s %*s %ld",
	 Opponent, &When);
  count++;

  while (!feof(fp)) {
    fgets(line, MAX_LINE_SIZE - 1, fp);
    if (!feof(fp))
      count++;
  }
  fclose(fp);

  stolower(Opponent);
  if (count > maxlines) {
    truncate_file(file, maxlines);

    oppWhen = OldestHistGame(Opponent);
    if (oppWhen > When || oppWhen <= 0L) {
      sprintf(GameFile, HISTORY_DIR "/%ld/%ld", When % 100, When);
      unlink(GameFile);
    }
  }
}

void RemHist(char *who)
{
	FILE *fp;
	char Opp[MAX_LOGIN_NAME];
	long When, oppWhen;

	fp = fopen_p(STATS_DIR "/player_data/%c/%s.%s", "r",
		     who[0], who, STATS_GAMES);
	if (!fp) {
		return;
	}
	while (!feof(fp)) {
		fscanf(fp, "%*d %*c %*d %*c %*d %s %*s %*d %*d %*d %*d %*s %*s %ld",
		       Opp, &When);
		stolower(Opp);
		oppWhen = OldestHistGame(Opp);
		if (oppWhen > When || oppWhen <= 0L) {
			char *fName;
			asprintf(&fName, HISTORY_DIR "/%ld/%ld", When % 100, When);
			unlink(fName);
			free(fName);
		}
	}
	fclose(fp);
}

static void write_g_out(int g, char *file, int maxlines, int isDraw,
			  char *EndSymbol, char *name, time_t *now)
{
  FILE *fp;
  int wp, bp;
  int wr, br;
  char type[4];
  char tmp[2048];
  char *ptmp = tmp;
  char cResult;
  int count = -1;
  char *goteco;

  wp = game_globals.garray[g].white;
  bp = game_globals.garray[g].black;

  if (game_globals.garray[g].private) {
    type[0] = 'p';
  } else {
    type[0] = ' ';
  }
  if (game_globals.garray[g].type == TYPE_BLITZ) {
    wr = player_globals.parray[wp].b_stats.rating;
    br = player_globals.parray[bp].b_stats.rating;
    type[1] = 'b';
  } else if (game_globals.garray[g].type == TYPE_WILD ||
	     game_globals.garray[g].type == TYPE_KNIGHTMATE ||
	     game_globals.garray[g].type == TYPE_CAPABLANCA ||
	     game_globals.garray[g].type == TYPE_GOTHIC) {
    wr = player_globals.parray[wp].w_stats.rating;
    br = player_globals.parray[bp].w_stats.rating;
    type[1] = 'w';
  } else if (game_globals.garray[g].type == TYPE_STAND) {
    wr = player_globals.parray[wp].s_stats.rating;
    br = player_globals.parray[bp].s_stats.rating;
    type[1] = 's';
  } else if (game_globals.garray[g].type == TYPE_LIGHT) {
    wr = player_globals.parray[wp].l_stats.rating;
    br = player_globals.parray[bp].l_stats.rating;
    type[1] = 'l';
  } else if (game_globals.garray[g].type == TYPE_BUGHOUSE) {
    wr = player_globals.parray[wp].bug_stats.rating;
    br = player_globals.parray[bp].bug_stats.rating;
    type[1] = 'B';
  } else {
    wr = 0;
    br = 0;
    if (game_globals.garray[g].type == TYPE_NONSTANDARD)
      type[1] = 'n';
    else
      type[1] = 'u';
  }
  if (game_globals.garray[g].rated) {
    type[2] = 'r';
  } else {
    type[2] = 'u';
  }
  type[3] = '\0';

  fp = fopen_s(file, "r");
  if (fp) {
    while (!feof(fp))
      fgets(tmp, 1024, fp);
    sscanf(ptmp, "%d", &count);
    fclose(fp);
  }
  count = (count + 1) % 100;

  fp = fopen_s(file, "a");
  if (!fp)
    return;

  goteco = getECO(g);

/* Counter Result MyRating MyColor OppRating OppName [pbr 2 12 2 12] ECO End Date */
  if (name == player_globals.parray[wp].name) {
    if (isDraw)
      cResult = '=';
    else if (game_globals.garray[g].winner == WHITE)
      cResult = '+';
    else
      cResult = '-';

    fprintf(fp, "%d %c %d W %d %s %s %d %d %d %d %s %s %ld\n",
	    count, cResult, wr, br, player_globals.parray[bp].name, type,
	    game_globals.garray[g].wInitTime, game_globals.garray[g].wIncrement,
	    game_globals.garray[g].bInitTime, game_globals.garray[g].bIncrement,
	    goteco,
	    EndSymbol,
	    (long) *now);
  } else {
    if (isDraw)
      cResult = '=';
    else if (game_globals.garray[g].winner == BLACK)
      cResult = '+';
    else
      cResult = '-';

    fprintf(fp, "%d %c %d B %d %s %s %d %d %d %d %s %s %ld\n",
	    count, cResult, br, wr, player_globals.parray[wp].name, type,
	    game_globals.garray[g].wInitTime, game_globals.garray[g].wIncrement,
	    game_globals.garray[g].bInitTime, game_globals.garray[g].bIncrement,
	    goteco,
	    EndSymbol,
	    (long) *now);
  }
  fclose(fp);

  RemoveHistGame(file, maxlines);
}

/* Test if entry is present - 1 yes 0 no */
/* returns -1 if an error */

char get_journalgame_type(int p,char* fname,char slot)

{
  struct player *pp = &player_globals.parray[p];
  char cur_slot;
  char type[4];

  FILE* fp = fopen_s(fname,"r");
  if (!fp) {
    d_printf("Corrupt journal file! %s\n",fname);
    pprintf (p, "The journal file is corrupt! See an admin.\n");
    return 0;
  }

  while (!feof(fp)) {
    if (fscanf(fp, "%c %*s %*d %*s %*d %s %*d %*d %*s %*s %*s\n",
       &cur_slot,type) != 2) {
      d_printf( "CHESSD: Error in journal info format for player %s.\n",
        pp->name);
      pprintf(p, "The journal file is corrupt! Error in internal format.\n");
      return '\0';
    }
    if (cur_slot == slot) {
      fclose (fp);
      if (type[0] == 'p')
        return type[1];
      else
        return type[0];
    }
  }
  fclose (fp);
  return '0';
}


/* Returns 1 if successful */

int removejournalitem(int p, char slot,FILE* fp,char* fname, int* empty)

{
	FILE* fp_new;
	int found = 0;
	struct journal* j;

	*empty = 1;
	fp_new = fopen_s(fname, "w");
	if (!fp_new) {
		d_printf("Can't write to journal %s!\n",fname);
		pprintf (p, "Was unable to write to the file! See an admin.\n");
		return 0;
	}
	j = (struct journal*) malloc(sizeof(struct journal));
	while (!feof(fp)) {
		if (fscanf(fp, "%c %s %d %s %d %s %d %d %s %s %s\n",
			   &j->slot,
			   j->WhiteName,
			   &j->WhiteRating,
			   j->BlackName,
			   &j->BlackRating,
			   j->type,
			   &j->t, &j->i,
			   j->eco,
			   j->ending,
			   j->result) != 11) {
			d_printf( "CHESSD: Error in journal info format. %s\n", fname);
			pprintf(p, "The journal file is corrupt! Error in internal format.\n");
			fclose(fp_new);
			free (j);
			return 0;
		}
		if (slot != j->slot) {
			*empty = 0;
			fprintf(fp_new, "%c %s %d %s %d %s %d %d %s %s %s\n",
				j->slot,
				j->WhiteName,
				j->WhiteRating,
				j->BlackName,
				j->BlackRating,
				j->type,
				j->t, j->i,
				j->eco,
				j->ending,
				j->result);
		} else
			found = 1;
	}
	fclose(fp_new);
	free (j);
	return found;
}

/* Find from_spot in journal list - return 0 if corrupted */
int journal_get_info(int p, char from_spot,struct journal* j, char *fname)
{
	FILE *fp;

	fp = fopen_s(fname, "r");
	if (!fp) {
		d_printf("Corrupt journal file! %s\n",fname);
		pprintf (p, "The journal file is corrupt! See an admin.\n");
		return 0;
	}
	while (!feof(fp)) {
		if (fscanf(fp, "%c %s %d %s %d %s %d %d %s %s %s\n",
			   &j->slot,
			   j->WhiteName,
			   &j->WhiteRating,
			   j->BlackName,
			   &j->BlackRating,
			   j->type,
			   &j->t, &j->i,
			   j->eco,
			   j->ending,
			   j->result) != 11) {
			d_printf( "CHESSD: Error in journal info format. %s\n", fname);
			pprintf(p, "The journal file is corrupt! Error in internal format.\n");
			fclose(fp);
			return 0;
		}
		if (tolower(j->slot) == from_spot) {
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}

void addjournalitem(int p,struct journal* j_add, char* fname)
{
	struct journal* j_cur;
	int have_output=0;
	char fname2[MAX_FILENAME_SIZE];
	FILE *fp;
	FILE *fp2;

	strcpy (fname2,fname);
	strcat (fname2,".w");
	fp2 = fopen_s(fname2, "w");
	if (!fp2) {
		d_printf( "CHESSD: Problem opening file %s for write\n", fname);
		pprintf (p, "Couldn't update journal! Report this to an admin.\n");
		return;
	} 
	fp = fopen_s(fname, "r");
	if (!fp) { /* Empty? */
		fprintf(fp2, "%c %s %d %s %d %s %d %d %s %s %s\n",
			j_add->slot, j_add->WhiteName, j_add->WhiteRating,
			j_add->BlackName, j_add->BlackRating,
			j_add->type, j_add->t, j_add->i, j_add->eco, j_add->ending,
			j_add->result);
		fclose (fp2);
		rename (fname2, fname);
		return;
	} else {
		j_cur = (struct journal*) malloc(sizeof(struct journal));
		while (!feof(fp)) {
			if (fscanf(fp, "%c %s %d %s %d %s %d %d %s %s %s\n",
				   &j_cur->slot,
				   j_cur->WhiteName,
				   &j_cur->WhiteRating,
				   j_cur->BlackName,
				   &j_cur->BlackRating,
				   j_cur->type,
				   &j_cur->t, &j_cur->i,
				   j_cur->eco,
				   j_cur->ending,
				   j_cur->result) != 11) {
				d_printf( "CHESSD: Error in journal info format - aborting. %s\n", fname);
				free (j_cur);
				fclose(fp);
				fclose(fp2);
				return;
			}
			if ((j_cur->slot >= j_add->slot) && (!have_output)) {
				
				fprintf(fp2, "%c %s %d %s %d %s %d %d %s %s %s\n",
					j_add->slot,
					j_add->WhiteName,
					j_add->WhiteRating,
					j_add->BlackName,
					j_add->BlackRating,
					j_add->type,
					j_add->t, j_add->i,
					j_add->eco,
					j_add->ending,
					j_add->result);
				have_output = 1;

			} 
			if (j_cur->slot != j_add->slot) {

				fprintf(fp2, "%c %s %d %s %d %s %d %d %s %s %s\n",
					j_cur->slot,
					j_cur->WhiteName,
					j_cur->WhiteRating,
					j_cur->BlackName,
					j_cur->BlackRating,
					j_cur->type,
					j_cur->t, j_cur->i,
					j_cur->eco,
					j_cur->ending,
					j_cur->result);
			}
		}
		
		if (!have_output) { /* Haven't written yet */
			fprintf(fp2, "%c %s %d %s %d %s %d %d %s %s %s\n",
				j_add->slot,
				j_add->WhiteName,
				j_add->WhiteRating,
				j_add->BlackName,
				j_add->BlackRating,
				j_add->type,
				j_add->t, j_add->i,
				j_add->eco,
				j_add->ending,
				j_add->result);
		}
		free (j_cur);
		fclose(fp);
		fclose(fp2);
		rename(fname2, fname);
		return;
	}
} 

int pjournal(int p, int p1, char *fname)
{
	FILE *fp;
	struct journal* j;
	
	fp = fopen_s(fname, "r");
	if (!fp) {
		pprintf(p, "Sorry, no journal information available.\n");
		return COM_OK;
	}

	j = (struct journal*) malloc(sizeof(struct journal));
	pprintf(p, "Journal for %s:\n", player_globals.parray[p1].name);
	pprintf(p, "   White         Rating  Black         Rating  Type         ECO End Result\n");
	while (!feof(fp)) {
		if (fscanf(fp, "%c %s %d %s %d %s %d %d %s %s %s\n",
			   &j->slot,
			   j->WhiteName,
			   &j->WhiteRating,
			   j->BlackName,
			   &j->BlackRating,
			   j->type,
			   &j->t, &j->i,
			   j->eco,
			   j->ending,
			   j->result) != 11) {
			d_printf( "CHESSD: Error in journal info format. %s\n", fname);
			fclose(fp);
			free(j);
			return COM_OK;
		}
		j->WhiteName[13] = '\0';         /* only first 13 chars in name */
		j->BlackName[13] = '\0';
		pprintf(p, "%c: %-13s %4d    %-13s %4d    [%3s%3d%4d] %s %3s %-7s\n",
			j->slot, j->WhiteName, j->WhiteRating,
			j->BlackName, j->BlackRating,
			j->type, j->t / 600, j->i / 10, j->eco, j->ending,
			j->result);
	}
	free (j);
	fclose(fp);
	return COM_OK;
}

int pgames(int p, int p1, char *fname)
{
	FILE *fp;
	time_t t;
	int MyRating, OppRating;
	int wt, wi, bt, bi;
	char OppName[MAX_LOGIN_NAME + 1];
	char type[100];
	char eco[100];
	char ending[100];
	char MyColor[2];
	int count;
	char result[2];
	
	fp = fopen_s(fname, "r");
	if (!fp) {
		pprintf(p, "Sorry, no game information available.\n");
		return COM_OK;
	}
	pprintf(p, "History for %s:\n", player_globals.parray[p1].name);
	pprintf(p, "                  Opponent      Type         ECO End Date\n");
	while (!feof(fp)) {
		if (fscanf(fp, "%d %s %d %s %d %s %s %d %d %d %d %s %s %ld\n",
			   &count,
			   result,
			   &MyRating,
			   MyColor,
			   &OppRating,
			   OppName,
			   type,
			   &wt, &wi,
			   &bt, &bi,
			   eco,
			   ending,
			   (long *) &t) != 14) {
			d_printf( "CHESSD: Error in games info format. %s\n", fname);
			fclose(fp);
			return COM_OK;
		}
		OppName[13] = '\0';		/* only first 13 chars in name */
		pprintf(p, "%2d: %s %4d %s %4d %-13s [%3s%3d%4d] %s %3s %s",
			count, result, MyRating, MyColor,
			OppRating, OppName,
			type, wt / 600, wi / 10, eco, ending,
			ctime(&t));
	}
	fclose(fp);
	return COM_OK;
}

void game_write_complete(int g, int isDraw, char *EndSymbol)
{
	char fname[MAX_FILENAME_SIZE];
	int wp = game_globals.garray[g].white, bp = game_globals.garray[g].black;
	time_t now = time(NULL);
	int Result;
	FILE *fp = NULL;
	
	do {
		if (fp) {
			fclose(fp);
			now++;
		}
		sprintf(fname, HISTORY_DIR "/%ld/%ld", (long) now % 100, (long) now);
		fp = fopen_s(fname, "r");
	} while (fp);   /* terminates when the file doesn't exist */
	
	fp = fopen_s(fname, "w");
	
	if (fp) {
		WriteGameFile_v100(fp, g);
		fclose(fp);
	} else {
		d_printf( "Trouble writing history file %s", fname);
	}
	
	sprintf(fname, STATS_DIR "/player_data/%c/%s.%s",
		player_globals.parray[wp].login[0], player_globals.parray[wp].login, STATS_GAMES);
	write_g_out(g, fname, 40, isDraw, EndSymbol, player_globals.parray[wp].name, &now);
	sprintf(fname, STATS_DIR "/player_data/%c/%s.%s",
		player_globals.parray[bp].login[0], player_globals.parray[bp].login, STATS_GAMES);
	write_g_out(g, fname, 40, isDraw, EndSymbol, player_globals.parray[bp].name, &now);
	
	if (isDraw)
		Result = -1;
	else if (game_globals.garray[g].winner == WHITE)
		Result = 1;
	else
		Result = 0;
	sprintf(fname, STATS_DIR "/player_data/%c/%s.gstats", player_globals.parray[game_globals.garray[g].white].login[0], player_globals.parray[game_globals.garray[g].white].login);
	if ((CheckPFlag(bp, PFLAG_REG)) && (CheckPFlag(wp, PFLAG_REG)) && (game_globals.garray[g].type != TYPE_WILD))
		game_save_playerratio(fname,player_globals.parray[game_globals.garray[g].black].name,Result,game_globals.garray[g].rated);
	if (isDraw)
		Result = -1;
	else if (game_globals.garray[g].winner == BLACK)
		Result = 1;
	else
		Result = 0;
	sprintf(fname, STATS_DIR "/player_data/%c/%s.gstats", player_globals.parray[game_globals.garray[g].black].login[0], player_globals.parray[game_globals.garray[g].black].login);
	if ((CheckPFlag(bp, PFLAG_REG)) && (CheckPFlag(wp, PFLAG_REG)) && (game_globals.garray[g].type != TYPE_WILD))
		game_save_playerratio(fname,player_globals.parray[game_globals.garray[g].white].name,Result,game_globals.garray[g].rated);
}

int game_count(void)
{
	int g, count = 0;
	
	for (g = 0; g < game_globals.g_num; g++) {
		if ((game_globals.garray[g].status == GAME_ACTIVE) || (game_globals.garray[g].status == GAME_EXAMINE)
		    || (game_globals.garray[g].status == GAME_SETUP))
			count++;
	}
	if (count > command_globals.game_high)
		command_globals.game_high = count;
	return count;
}

static int check_kings(struct game_state_t *gs)
{
	/* Function written 3/28/96 by Figbert to check for 1 king each side only! */
	int blackking = 0, whiteking = 0;
	int f, r;
	
	
	for (f = 0; f < gs->files; f++) {
		for (r = 0; r < gs->ranks; r++) {
			if (gs->board[f][r] == B_KING) blackking++;
			if (gs->board[f][r] == W_KING) whiteking++;
		}
	}
	
	if ((blackking == 1 || blackking == 2 && !strcmp(gs->variant, "spartan")) && whiteking == 1) return 0; /* Perfect! */
	
	return -1;
}
