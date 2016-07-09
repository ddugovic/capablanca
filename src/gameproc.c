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

/*Let users know that someone is available (format the message)*/
static void getavailmess(int p, char* message)
{
	struct player *pp = &player_globals.parray[p];
	char titles[100];

	titles [0]='\0';
	AddPlayerLists(p,titles);
	sprintf (message,"%s%s Blitz (%s), Std (%s), Wild (%s), Light(%s), Bug(%s)\n"
		 "  is now available for matches.",
		 pp->name, titles,
	ratstrii(pp->b_stats.rating, p),
        ratstrii(pp->s_stats.rating, p),
	ratstrii(pp->w_stats.rating, p),
	ratstrii(pp->l_stats.rating, p),
        ratstrii(pp->bug_stats.rating, p));
}

void getnotavailmess(int p, char* message)
{
	struct player *pp = &player_globals.parray[p];
	char titles[100];

	titles[0]='\0';
	AddPlayerLists(p,titles);
	sprintf (message,"%s%s is no longer available for matches.",
		 pp->name, titles);
}

void announce_avail(int p)
{
	struct player *pp = &player_globals.parray[p];
	char avail[200];
	int p1;
	if ((pp->game < 0) && (CheckPFlag(p, PFLAG_OPEN))) {
		getavailmess (p,avail);
		
		for (p1 = 0; p1 < player_globals.p_num; p1++) {
			if (p == p1)
				continue;
			if (player_globals.parray[p1].status != PLAYER_PROMPT)
				continue;
			if (CheckPFlag(p1, PFLAG_AVAIL) && CheckPFlag(p1, PFLAG_OPEN)
			    && (player_globals.parray[p1].game < 0))
				if (((pp->b_stats.rating <= player_globals.parray[p1].availmax) && (pp->b_stats.rating >= player_globals.parray[p1].availmin)) || (!player_globals.parray[p1].availmax))
					pprintf_prompt (p1,"\n%s\n",avail);
		}
	}
}

void announce_notavail(int p)
{
	struct player *pp = &player_globals.parray[p];
	char avail[200];
	int p1;
	
	getnotavailmess (p,avail);
	
	for (p1 = 0; p1 < player_globals.p_num; p1++) {
		if (p == p1)
			continue;
		if (player_globals.parray[p1].status != PLAYER_PROMPT)
			continue;
		if (CheckPFlag(p1, PFLAG_AVAIL) && CheckPFlag(p1, PFLAG_OPEN)
		    && (player_globals.parray[p1].game < 0))
			if (((pp->b_stats.rating <= player_globals.parray[p1].availmax) && (pp->b_stats.rating >= player_globals.parray[p1].availmin)) || (!player_globals.parray[p1].availmax))
				pprintf_prompt (p1,"\n%s\n",avail);
	}
}

void game_ended(int g, int winner, int why)
{
  struct game *gg = &game_globals.garray[g];
  char outstr[200];
  char avail_black[200]; /* for announcing white/black avail */
  char avail_white[200];
  char avail_bugwhite[200];
  char avail_bugblack[200];
  char tmp[200];
  int p;
  int gl = gg->link;
  int rate_change = 0;
  int isDraw = 0;
  int whiteResult;
  char winSymbol[10];
  char EndSymbol[10];
  char *NameOfWinner, *NameOfLoser;
  int beingplayed = 0;		/* i.e. it wasn't loaded for adjudication */
  int print_avail = 0;

  avail_white[0] = '\0';
  avail_black[0] = '\0';
  avail_bugwhite[0] = '\0';
  avail_bugblack[0] = '\0';

  beingplayed = (player_globals.parray[gg->black].game == g);

  sprintf(outstr, "\n{Game %d (%s vs. %s) ", g + 1,
	  player_globals.parray[gg->white].name,
	  player_globals.parray[gg->black].name);
  gg->result = why;
  gg->winner = winner;
  if (winner == WHITE) {
    whiteResult = RESULT_WIN;
    strcpy(winSymbol, "1-0");
    NameOfWinner = player_globals.parray[gg->white].name;
    NameOfLoser = player_globals.parray[gg->black].name;
  } else {
    whiteResult = RESULT_LOSS;
    strcpy(winSymbol, "0-1");
    NameOfWinner = player_globals.parray[gg->black].name;
    NameOfLoser = player_globals.parray[gg->white].name;
  }
  switch (why) {
  case END_CHECKMATE:
    sprintf(tmp, "%s checkmated} %s", NameOfLoser, winSymbol);
    strcpy(EndSymbol, "Mat");
    rate_change = 1;
    break;
  case END_BARE:
    sprintf(tmp, "%s bared} %s", NameOfLoser, winSymbol);
    strcpy(EndSymbol, "Bar");
    rate_change = 1;
    break;
  case END_PERPETUAL:
    sprintf(tmp, "%s perpetually checking} %s", NameOfLoser, winSymbol);
    strcpy(EndSymbol, "Per");
    rate_change = 1;
    break;
  case END_RESIGN:
    sprintf(tmp, "%s resigns} %s", NameOfLoser, winSymbol);
    strcpy(EndSymbol, "Res");
    rate_change = 1;
    break;
  case END_FLAG:
    sprintf(tmp, "%s forfeits on time} %s", NameOfLoser, winSymbol);
    strcpy(EndSymbol, "Fla");
    rate_change = 1;
    break;
  case END_STALEMATE:
    sprintf(tmp, "Game drawn by stalemate} 1/2-1/2");
    isDraw = 1;
    strcpy(EndSymbol, "Sta");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_AGREEDDRAW:
    sprintf(tmp, "Game drawn by mutual agreement} 1/2-1/2");
    isDraw = 1;
    strcpy(EndSymbol, "Agr");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_BOTHFLAG:
    sprintf(tmp, "Game drawn because both players ran out of time} 1/2-1/2");
    isDraw = 1;
    strcpy(EndSymbol, "Fla");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_REPETITION:
    sprintf(tmp, "Game drawn by repetition} 1/2-1/2");
    isDraw = 1;
    strcpy(EndSymbol, "Rep");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_50MOVERULE:
    sprintf(tmp, "Game drawn by the 50 move rule} 1/2-1/2");
    isDraw = 1;
    strcpy(EndSymbol, "50");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_ADJOURN:
    if (gl >= 0) {
      sprintf(tmp, "Bughouse game aborted.} *");
      whiteResult = RESULT_ABORT;
    } else {
    sprintf(tmp, "Game adjourned by mutual agreement} *");
    game_save(g);
    }
    break;
  case END_LOSTCONNECTION:
    sprintf(tmp, "%s lost connection; game ", NameOfWinner);
    if (CheckPFlag(gg->white, PFLAG_REG)
        && CheckPFlag(gg->black, PFLAG_REG)
        && gl < 0) {
      sprintf(tmp, "adjourned} *");
      game_save(g);
    } else
      sprintf(tmp, "aborted} *");
    whiteResult = RESULT_ABORT;
    break;
  case END_ABORT:
    sprintf(tmp, "Game aborted by mutual agreement} *");
    whiteResult = RESULT_ABORT;
    break;
  case END_COURTESY:
    sprintf(tmp, "Game courtesyaborted by %s} *", NameOfWinner);
    whiteResult = RESULT_ABORT;
    break;
  case END_COURTESYADJOURN:
    if (gl >= 0) {
      sprintf(tmp, "Bughouse game courtesyaborted by %s.} *", NameOfWinner);
      whiteResult = RESULT_ABORT;
    } else {
    sprintf(tmp, "Game courtesyadjourned by %s} *", NameOfWinner);
    game_save(g);
    }
    break;
  case END_NOMATERIAL:
    /* Draw by insufficient material (e.g., lone K vs. lone K) */
    sprintf(tmp, "Neither player has mating material} 1/2-1/2");
    isDraw = 1;
    strcpy(EndSymbol, "NM ");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_FLAGNOMATERIAL:
    sprintf(tmp, "%s ran out of time and %s has no material to mate} 1/2-1/2",
	    NameOfLoser, NameOfWinner);
    isDraw = 1;
    strcpy(EndSymbol, "TM ");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_ADJWIN:
    sprintf(tmp, "%s wins by adjudication} %s", NameOfWinner, winSymbol);
    strcpy(EndSymbol, "Adj");
    rate_change = 1;
    break;
  case END_ADJDRAW:
    sprintf(tmp, "Game drawn by adjudication} 1/2-1/2");
    isDraw = 1;
    strcpy(EndSymbol, "Adj");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_ADJABORT:
    sprintf(tmp, "Game aborted by adjudication} *");
    whiteResult = RESULT_ABORT;
    break;
  default:
    sprintf(tmp, "Hmm, the game ended and I don't know why} *");
    break;
  }
  strcat(outstr, tmp);

  if (CheckPFlag(gg->white, PFLAG_TOURNEY) &&
      CheckPFlag(gg->black, PFLAG_TOURNEY)) {
	  /* mamer wants more info */
	  sprintf(tmp," [%d %d %d %d %d]",
		  gg->wInitTime/(60*10), gg->wIncrement/10, gg->rated, gg->private, (int)gg->type);
	  strcat(outstr, tmp);
  }

  strcat(outstr, "\n");

  if (gg->rated && rate_change && gg->type != TYPE_BUGHOUSE)
    /* Adjust ratings; bughouse gets done later. */
    rating_update(g, -1);

  if (beingplayed) {
    int printed = 0;
    int avail_printed = 0;

    pprintf_noformat(gg->white, outstr);
    pprintf_noformat(gg->black, outstr);
    Bell (gg->white);
    Bell (gg->black);

    gg->link = -1;		/*IanO: avoids recursion */
    if (gl >= 0 && game_globals.garray[gl].link >= 0) {
      pprintf_noformat(game_globals.garray[gl].white, outstr);
      pprintf_noformat(game_globals.garray[gl].black, outstr);
      if (CheckPFlag(game_globals.garray[gl].white, PFLAG_OPEN)) {
        getavailmess (game_globals.garray[gl].white, avail_bugwhite);
        print_avail = 1;
      }
      if (CheckPFlag(game_globals.garray[gl].black, PFLAG_OPEN)) {
        getavailmess (game_globals.garray[gl].black, avail_bugblack);
        print_avail = 1;
      }
      if ((gg->rated) && (rate_change)) {
        /* Adjust ratings */
        rating_update(g, gl);
      }
      game_ended(gl, CToggle(winner), why);
    }

    if ((player_num_active_boards(gg->white) <= 1) /* not a simul or */
         && CheckPFlag(gg->white, PFLAG_OPEN)) {   /* simul is over? */
      getavailmess (gg->white,avail_white);
      print_avail = 1;
    } else {    /* Part of an ongoing simul!  Let's shrink the array. */
      
    }
    
    if (CheckPFlag(gg->black, PFLAG_OPEN)) {
      getavailmess (gg->black,avail_black);
      print_avail = 1;
    }

    for (p = 0; p < player_globals.p_num; p++) {
      struct player *pp = &player_globals.parray[p];
      if ((p == gg->white) || (p == gg->black))
	continue;
      if (pp->status != PLAYER_PROMPT)
	continue;

      if (CheckPFlag(p, PFLAG_GIN) || player_is_observe(p, g)) {
        pprintf_noformat(p, outstr);
        printed = 1;
      }

      if (CheckPFlag(p, PFLAG_AVAIL) && (CheckPFlag(p, PFLAG_OPEN)) && (pp->game < 0) && (print_avail)) {
        if (((player_globals.parray[gg->white].b_stats.rating <= pp->availmax) && (player_globals.parray[gg->white].b_stats.rating >= pp->availmin)) || (!pp->availmax)) {
          pprintf (p,"\n%s",avail_white);
          avail_printed = 1;
        }
        if (((player_globals.parray[gg->black].b_stats.rating <= pp->availmax) && (player_globals.parray[gg->black].b_stats.rating >= pp->availmin)) || (!pp->availmax)) {
          pprintf (p,"\n%s",avail_black);
          avail_printed = 1;
        }
        if (gl == -1) /* bughouse ? */ {
          if (((player_globals.parray[game_globals.garray[gl].white].b_stats.rating <= pp->availmax) && (player_globals.parray[game_globals.garray[gl].white].b_stats.rating >= pp->availmin)) || (!pp->availmax)) {
            pprintf (p,"\n%s",avail_bugwhite);
            avail_printed = 1;
          }
          if (((player_globals.parray[game_globals.garray[gl].black].b_stats.rating <= pp->availmax) && (player_globals.parray[game_globals.garray[gl].black].b_stats.rating >= pp->availmin)) || (!pp->availmax)) {
            pprintf (p,"\n%s",avail_bugblack);
            avail_printed = 1;
          }
        }
        if (avail_printed) {
          avail_printed = 0;
	  printed = 1; 
	  pprintf (p,"\n");
        }
      }

      if (printed) {
        send_prompt(p);
        printed = 0;
      }
    }

    if (!(gg->rated && rate_change)) {
      pprintf(gg->white, "No ratings adjustment done.\n");
      pprintf(gg->black, "No ratings adjustment done.\n");
    } 
  }

  if (rate_change && gl < 0)
    game_write_complete(g, isDraw, EndSymbol);
  /* Mail off the moves */
  if (CheckPFlag(gg->white, PFLAG_AUTOMAIL)) {
    pcommand(gg->white, "mailmoves");
  }
  if (CheckPFlag(gg->black, PFLAG_AUTOMAIL)) {
    pcommand(gg->black, "mailmoves");
  }
  if (!((player_globals.parray[gg->white].simul_info != NULL) &&
         (player_globals.parray[gg->white].simul_info->numBoards))) {
    player_globals.parray[gg->white].num_white++;
    PFlagOFF(gg->white, PFLAG_LASTBLACK);
    player_globals.parray[gg->black].num_black++;
    PFlagON(gg->black, PFLAG_LASTBLACK);
  }
  player_globals.parray[gg->white].last_opponent = 
	  strdup(gg->black_name);
  player_globals.parray[gg->black].last_opponent = 
	  strdup(gg->white_name);
  if (beingplayed) {
    player_globals.parray[gg->white].game = -1;
    player_globals.parray[gg->black].game = -1;
    player_globals.parray[gg->white].opponent = -1;
    player_globals.parray[gg->black].opponent = -1;
    if (gg->white != command_globals.commanding_player)
      send_prompt(gg->white);
    if (gg->black != command_globals.commanding_player)
      send_prompt(gg->black);
    if ((player_globals.parray[gg->white].simul_info != NULL) && 
         (player_globals.parray[gg->white].simul_info->numBoards))
      player_simul_over(gg->white, g, whiteResult);
  }
  game_finish(g); 
}

static int was_promoted(struct game *g, int f, int r)
{
#define BUGHOUSE_PAWN_REVERT 1
#ifdef BUGHOUSE_PAWN_REVERT
  int i;

  for (i = g->numHalfMoves-2; i > 0; i -= 2) {
    if (g->moveList[i].toFile == f && g->moveList[i].toRank == r) {
      if (g->moveList[i].piecePromotionTo) {
	switch(g->moveList[i].moveString[0]) { // [HGM] return original piece type rather than just TRUE
          case 'P': return PAWN;
          case 'N': return HONORABLEHORSE; // !!! this is Shogi, so no KNIGHT !!!
          case 'B': return BISHOP;
          case 'R': return ROOK;
          case 'L': return LANCE;
          case 'S': return SILVER;
          default:  return GOLD;
        }
      }
      if (g->moveList[i].fromFile == ALG_DROP)
	return 0;
      f = g->moveList[i].fromFile;
      r = g->moveList[i].fromRank;
    }
  }
#endif
  return 0;
}

int pIsPlaying (int p)
{
	struct player *pp = &player_globals.parray[p];
	int g = pp->game;
	int p1 = pp->opponent;
	
	if (g < 0 || game_globals.garray[g].status != GAME_ACTIVE) {
		pprintf (p, "You are not playing a game.\n");
		return 0;
	} 

	if (game_globals.garray[g].white != p && game_globals.garray[g].black != p) {
		/* oh oh; big bad game bug. */
		d_printf("BUG:  Player %s playing game %d according to player_globals.parray,"
			 "\n      but not according to game_globals.garray.\n", pp->name, g+1);
		pprintf (p, "Disconnecting you from game number %d.\n", g+1);
		pp->game = -1;
		if (p1 >= 0 && player_globals.parray[p1].game == g
		    && game_globals.garray[g].white != p1 && game_globals.garray[g].black != p1) {
			pprintf (p1, "Disconnecting you from game number %d.\n", g+1);
			player_globals.parray[p1].game = -1;
		}
		return 0;
	}
	return 1;
}

/* add clock increments */
static void game_add_increment(struct player *pp, struct game *gg)
{
	/* no update on first move */
	if (gg->game_state.moveNum == 1) return;

	if (net_globals.con[pp->socket]->timeseal) {	/* does he use timeseal? */
		if (pp->side == WHITE) {
			gg->wRealTime += gg->wIncrement * 100;
			gg->wTime = gg->wRealTime / 100;	/* remember to conv to
												   tenth secs */
		} else if (pp->side == BLACK) {
			gg->bRealTime += gg->bIncrement * 100;	/* conv to ms */
			gg->bTime = gg->bRealTime / 100;	/* remember to conv to
												   tenth secs */
		}
	} else {
		if (gg->game_state.onMove == BLACK) {
			gg->bTime += gg->bIncrement;
		}
		if (gg->game_state.onMove == WHITE) {
			gg->wTime += gg->wIncrement;
		}
	}
}

/* updates clocks for a game with timeseal */
void timeseal_update_clocks(struct player *pp, struct game *gg)
{
	/* no update on first move */
	if (gg->game_state.moveNum == 1) return;

	if (pp->side == WHITE) {
		gg->wLastRealTime = gg->wRealTime;
		gg->wTimeWhenMoved = net_globals.con[pp->socket]->time;
		if (((gg->wTimeWhenMoved - gg->wTimeWhenReceivedMove) < 0) ||
		    (gg->wTimeWhenReceivedMove == 0)) {
			/* might seem weird - but could be caused by a person moving BEFORE
			   he receives the board pos (this is possible due to lag) but it's
			   safe to say he moved in 0 secs :-) */
			gg->wTimeWhenReceivedMove = gg->wTimeWhenMoved;
		} else {
			gg->wRealTime -= gg->wTimeWhenMoved - gg->wTimeWhenReceivedMove;
		}
	} else if (pp->side == BLACK) {
		gg->bLastRealTime = gg->bRealTime;
		gg->bTimeWhenMoved = net_globals.con[pp->socket]->time;
		if (((gg->bTimeWhenMoved - gg->bTimeWhenReceivedMove) < 0) ||
		    (gg->bTimeWhenReceivedMove == 0)) {
			/* might seem weird - but could be caused by a person moving BEFORE
			   he receives the board pos (this is possible due to lag) but it's
			   safe to say he moved in 0 secs :-) */
			gg->bTimeWhenReceivedMove = gg->bTimeWhenMoved;
		} else {
			gg->bRealTime -= gg->bTimeWhenMoved - gg->bTimeWhenReceivedMove;
		}
	}
}


void process_move(int p, char *command)
{
  struct player *pp = &player_globals.parray[p];
  struct game *gg;
  int g, result, len, i, f;
  struct move_t move;
  unsigned now = 0;

  if (pp->game < 0) {
    pprintf(p, "You are not playing or examining a game.\n");
    return;
  }
  decline_withdraw_offers(p, -1, -PEND_SIMUL,DO_DECLINE);

  g = pp->game;
  gg = &game_globals.garray[g];

  if (gg->status == GAME_SETUP) {
    if (!attempt_drop(p,g,command)) {
      pprintf(p, "You are still setting up the position.\n");
      pprintf(p, "Type: 'setup done' when you are finished editing.\n");
    } else
    send_board_to(g, p); 
    return;
  }

  if (gg->status != GAME_EXAMINE) {
    if (!pIsPlaying(p)) return;

    if (pp->side != gg->game_state.onMove) {
      pprintf(p, "It is not your move.\n");
      return;
    }
    if (gg->clockStopped) {
      pprintf(p, "Game clock is paused, use \"unpause\" to resume.\n");
      return;
    }
  }
  pp->promote = NOPIECE; // [HGM] this seemed to be uninitialized, which caused spurious promotion in Shogi
  if ((len = strlen(command)) > 1) {
    if (command[len - 2] == '=' || gg->game_state.drops == 2 && command[len - 2] == '/') { // [HGM] encode gating as promotion
printf("promo '%s'\n", command);
      switch (tolower(command[len - 1])) {
      case 'n':
	pp->promote = KNIGHT;
	break;
      case 'b':
	pp->promote = BISHOP;
	break;
      case 'r':
	pp->promote = ROOK;
	break;
      case 'a':
	pp->promote = CARDINAL;
	break;
      case 'c':
	pp->promote = MARSHALL;
	break;
      case 'm':
	pp->promote = MAN;
	break;
      case 'q':
	pp->promote = QUEEN;
	break;
      // courier promotion
      case 'f':
	pp->promote = FERZ2;
	break;
      // Superchess promotions
      case 'e':
	pp->promote = EMPRESS;
	break;
      case 's':
	pp->promote = PRINCESS;
	break;
      case 'v':
	pp->promote = CENTAUR;
	break;
      case 'w':
	pp->promote = WOODY;
	break;
      case 'o':
	pp->promote = SQUIRREL;
	break;
      case 'g':
	pp->promote = MASTODON;
	break;
      case 'l':
	pp->promote = LIEUTENANT;
	break;
      case 'k':
	pp->promote = KING;
	break;
      // Shogi promotions
      case 'h':
	pp->promote = DRAGONHORSE;
	break;
      case 'd':
	pp->promote = DRAGONKING;
	break;
      case '^':
      case '+':
	pp->promote = GOLD;
	break;
      case '=':
	pp->promote = NOPIECE;
	break;
      default:
	pprintf(p, "Don't understand that move.\n");
	return;
	break;
      }
    }
  }

  switch (parse_move(command, &gg->game_state, &move, pp->promote)) {
  case MOVE_ILLEGAL:
    pprintf(p, "Illegal move.\n");
    return;
    break;
  case MOVE_AMBIGUOUS:
    pprintf(p, "Ambiguous move.\n");
    return;
    break;
  default:
    break;
  }

  if (gg->status == GAME_EXAMINE) {
    gg->numHalfMoves++;
    if (gg->numHalfMoves > gg->examMoveListSize) {
      gg->examMoveListSize += 20;	/* Allocate 20 moves at a time */
      gg->examMoveList = (struct move_t *) realloc(gg->examMoveList, sizeof(struct move_t) * gg->examMoveListSize);
    }
    result = execute_move(&gg->game_state, &move, 1);
    move.atTime = now;
    move.tookTime = 0;
    MakeFENpos(g, move.FENpos);
    gg->examMoveList[gg->numHalfMoves - 1] = move;
    /* roll back time */
    if (gg->game_state.onMove == WHITE) {
      gg->wTime += (gg->lastDecTime - gg->lastMoveTime);
    } else {
      gg->bTime += (gg->lastDecTime - gg->lastMoveTime);
    }
    now = tenth_secs();
    if (gg->numHalfMoves == 0)
      gg->timeOfStart = now;
    gg->lastMoveTime = now;
    gg->lastDecTime = now;

  } else {			/* real game */
    i = pp->opponent;
    if ((player_globals.parray[i].simul_info != NULL) && (player_globals.parray[i].simul_info->numBoards &&
	 (player_globals.parray[i].simul_info->boards[player_globals.parray[i].simul_info->onBoard] != g))) {
      pprintf(p, "It isn't your turn: wait until the simul giver is at your board.\n");
      return;
    }
    if (net_globals.con[pp->socket]->timeseal) {	/* does he use timeseal? */
	    timeseal_update_clocks(pp, &game_globals.garray[g]);
    }
    /* we need to reset the opp's time for receiving the board since the
       timeseal decoder only alters the time if it's 0 Otherwise the time
       would be changed if the player did a refresh which would screw up
       the timings */
    if (pp->side == WHITE) {
      gg->bTimeWhenReceivedMove = 0;
    } else {
      gg->wTimeWhenReceivedMove = 0;
    }

    game_update_time(g);
    game_add_increment(pp, gg);

    /* Do the move */
    gg->numHalfMoves++;
    if (gg->numHalfMoves > gg->moveListSize) {
      gg->moveListSize += 20;	/* Allocate 20 moves at a time */
      gg->moveList = (struct move_t *) realloc(gg->moveList, sizeof(struct move_t) * gg->moveListSize);
    }
    result = execute_move(&gg->game_state, &move, 1);
    if (result == MOVE_OK && (gg->link >= 0 || gg->game_state.holdings) && move.pieceCaptured != NOPIECE) {
      /* transfer captured piece to partner */
      /* check if piece reverts to a pawn */
      int victim = move.pieceCaptured, partner = gg->link, demoted;
      // [HGM] zh: if not Bughouse, the game_state.holdings field decides what happens
      if(gg->link < 0) { 
	partner = g; // pieces stay with current board
	if(gg->game_state.holdings == -1) victim ^= WHITE|BLACK; // flip color
      } 
      if (demoted = was_promoted(&game_globals.garray[g], move.toFile, move.toRank))
        update_holding(partner, colorval(victim) | demoted); // [HGM] was_promoted now returns original piece type
      else
        update_holding(partner, victim);
    }
    now = tenth_secs();
    move.atTime = now;
    if (gg->numHalfMoves > 1) {
      move.tookTime = move.atTime - gg->lastMoveTime;
    } else {
      move.tookTime = move.atTime - gg->startTime;
    }
    gg->lastMoveTime = now;
    gg->lastDecTime = now;
    move.wTime = gg->wTime;
    move.bTime = gg->bTime;

    if (net_globals.con[pp->socket]->timeseal) {	/* does he use timeseal? */
      if (pp->side == WHITE) {
	move.tookTime = (game_globals.garray[pp->game].wTimeWhenMoved -
			 game_globals.garray[pp->game].wTimeWhenReceivedMove) / 100;
      } else {
	move.tookTime = (game_globals.garray[pp->game].bTimeWhenMoved -
			 game_globals.garray[pp->game].bTimeWhenReceivedMove) / 100;
      }
    }

    if (gg->numHalfMoves <= 2) {
	    move.tookTime = 0;
    }

    MakeFENpos(g, move.FENpos);
    gg->moveList[gg->numHalfMoves - 1] = move;
  }

  send_boards(g);

  if (result == MOVE_ILLEGAL) {
    pprintf(p, "Internal error, illegal move accepted!\n");
  }
  if ((result == MOVE_OK) && (gg->status == GAME_EXAMINE)) {
    int p1;

    for (p1 = 0; p1 < player_globals.p_num; p1++) {
      if (player_globals.parray[p1].status != PLAYER_PROMPT)
	continue;
      if (player_is_observe(p1, g) || player_globals.parray[p1].game == g) {
	pprintf(p1, "%s moves: %s\n", pp->name, move.algString);
      }
    }
  }
  if (result == MOVE_CHECKMATE) {
    if (gg->status == GAME_EXAMINE) {
      int p1;

      for (p1 = 0; p1 < player_globals.p_num; p1++) {
	if (player_globals.parray[p1].status != PLAYER_PROMPT)
	  continue;
	if (player_is_observe(p1, g) || player_globals.parray[p1].game == g) {
	  pprintf(p1, "%s has been checkmated.\n",
		  (CToggle(gg->game_state.onMove) == BLACK) ? "White" : "Black");
	}
      }
    } else {
      game_ended(g, CToggle(gg->game_state.onMove), END_CHECKMATE);
    }
  }
  if (result == MOVE_STALEMATE) {
    if (gg->status == GAME_EXAMINE) {
      int p1;

      for (p1 = 0; p1 < player_globals.p_num; p1++) {
	if (player_globals.parray[p1].status != PLAYER_PROMPT)
	  continue;
	if (player_is_observe(p1, g) || player_globals.parray[p1].game == g) {
	  pprintf(p1, "Stalemate.\n");
	}
      }
    } else {
      game_ended(g, CToggle(gg->game_state.onMove), END_STALEMATE);
    }
  }
  if (result == MOVE_NOMATERIAL) {
    if (gg->status == GAME_EXAMINE) {
      int p1;

      for (p1 = 0; p1 < player_globals.p_num; p1++) {
	if (player_globals.parray[p1].status != PLAYER_PROMPT)
	  continue;
	if (player_is_observe(p1, g) || player_globals.parray[p1].game == g) {
	  pprintf(p1, "No mating material.\n");
	}
      }
    } else {
      game_ended(g, CToggle(gg->game_state.onMove), END_NOMATERIAL);
    }
  }
  if (result == MOVE_BARE) {
    if (gg->status == GAME_EXAMINE) {
      int p1;

      for (p1 = 0; p1 < player_globals.p_num; p1++) {
	if (player_globals.parray[p1].status != PLAYER_PROMPT)
	  continue;
	if (player_is_observe(p1, g) || player_globals.parray[p1].game == g) {
	  pprintf(p1, "%s bared.\n",
		  (gg->game_state.onMove == BLACK) ? "White" : "Black");
	}
      }
    } else {
      game_ended(g, gg->game_state.onMove, END_BARE);
    }
  }
}

int com_resign(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int g, o, oconnected;

  if (param[0].type == TYPE_NULL) {
    g = pp->game;
    if (!pIsPlaying(p))
      return COM_OK;
    else {
      decline_withdraw_offers(p, -1, -1, DO_DECLINE);
      game_ended(g, (game_globals.garray[g].white == p) ? BLACK : WHITE, END_RESIGN);
    }
  } else if (FindPlayer(p, param[0].val.word, &o, &oconnected)) {
    g = game_new();
    if (game_read(g, p, o) < 0) {
      if (game_read(g, o, p) < 0) {
	pprintf(p, "You have no stored game with %s\n", player_globals.parray[o].name);
        if (!oconnected)
          player_remove(o);
        return COM_OK;
      } else {
	game_globals.garray[g].white = o;
	game_globals.garray[g].black = p;
      }
    } else {
      game_globals.garray[g].white = p;
      game_globals.garray[g].black = o;
    }
    pprintf(p, "You resign your stored game with %s\n", player_globals.parray[o].name);
    pcommand(p, "message %s I have resigned our stored game \"%s vs. %s.\"",
	     player_globals.parray[o].name,
	     player_globals.parray[game_globals.garray[g].white].name,
	     player_globals.parray[game_globals.garray[g].black].name);
    game_delete(game_globals.garray[g].white, game_globals.garray[g].black);
    game_ended(g, (game_globals.garray[g].white == p) ? BLACK : WHITE, END_RESIGN);
    if (!oconnected)
      player_remove(o);
  }
  return COM_OK;
}

static int Check50MoveRule (int p, int g)
{
  int num_reversible = game_globals.garray[g].numHalfMoves;

  if (game_globals.garray[g].game_state.lastIrreversable >= 0) {
    num_reversible -= game_globals.garray[g].game_state.lastIrreversable;
  }
  if (num_reversible > 99) {
    game_ended(g, (game_globals.garray[g].white == p) ? BLACK : WHITE, END_50MOVERULE);
    return 1;
  }
  return 0;
}

static int perp_check(struct game g, int first, int third)
{
  struct game_state_t gs = g.game_state; // current position, both first and last of loop
  int half_move, no_perp = 0;
printf("perp %d %d\n",first,third);
  for(half_move=first+1; half_move<third; half_move++) {
    gs.onMove = CToggle(gs.onMove);
    if(!in_check(&gs)) no_perp |= (half_move&1) + 1; // 1 = white not in check, 2 = black not in check
    gs.onMove = CToggle(gs.onMove);
printf("move%d, p=%d\n",half_move,no_perp);
    if(no_perp == 3) break;
    execute_move(&gs, &g.moveList[half_move], 0);
  }
  if(no_perp == (third&1) + 1) return END_NOTENDED;  // stm was checking, other not: defer judgement
  if(no_perp == 2 - (third&1)) return END_PERPETUAL; // stm was not checking, other was: stm wins
  if(no_perp == 0) return END_REPETITION; // mutual perpertual check, draw
  // here we should check for chasing
  return END_REPETITION;
}

static char *GetFENpos (int g, int half_move)
{
  if (half_move < 0)
    return game_globals.garray[g].FENstartPos;
  else return game_globals.garray[g].moveList[half_move].FENpos;
}

static int CheckRepetition (int p, int g)
{
  struct player *pp = &player_globals.parray[p];
  struct pending* pend;
  int move_num, s1, s2, result = END_REPETITION;
  int flag1 = 1, flag2 = 1;
  int numPly = game_globals.garray[g].numHalfMoves;
  char *pos1 = GetFENpos (g, numPly - 1); // current position
  char *pos2 = "";
  char *pos;
  int  turn = numPly - 1;

  if (numPly < 8)  /* can't have three repeats any quicker. */
    return 0;

  if((game_globals.garray[g].white == p) != (numPly&1)) { // claimer has the move
    pos2 = pos1;
    pos1 = GetFENpos (g, turn = numPly - 2); // also check position before opponent's move (which could have pre-empted him)
  } // pos1 is now always a position where the opponent has the move

  for (move_num = numPly - 3; // [HGM] FEN stored in moveList[numHalfMoves-1] !
       move_num >= game_globals.garray[g].game_state.lastIrreversable - 1; move_num--) {
    pos = GetFENpos (g, move_num);
    if (!(turn - move_num & 1) && strlen(pos1) == strlen(pos) && !strcmp(pos1, pos))
      flag1++ == 2 && (s1 = move_num);
    if ( (turn - move_num & 1) && strlen(pos2) == strlen(pos) && !strcmp(pos2, pos))
      flag2++ == 2 && (s2 = move_num); // remember start of last two loops
printf("%2d. %d-%d '%s' '%s' '%s'\n", move_num, flag1, flag2, pos1,pos2,pos);
  }
  if (flag1 >= 3 || flag2 >= 3) {
    if ((pend = find_pend(pp->opponent, p, PEND_DRAW)) != NULL) {
      delete_pending(pend);
      decline_withdraw_offers(p, -1, -1,DO_DECLINE);
    }
    if(game_globals.garray[g].game_state.palace) { // [HGM] in Xiangqi we have to test for perpetuals to determine the outcome
      if(flag2 >= 3) result = perp_check(game_globals.garray[g], s2, numPly);
      else  result = perp_check(game_globals.garray[g], s1, numPly - (pos2[0] != 0));
      if(result == END_NOTENDED) {
	pprintf(p, "Perpetuals can be claimed only during the turn of the winner\n");
	return 1;
      }
      game_ended(g, (numPly&1) ? BLACK : WHITE, result); // stm wins
      return 1;
    }
    game_ended(g, (game_globals.garray[g].white == p) ? BLACK : WHITE, result);
    return 1;
  }
  else return 0;
}

int com_draw(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  struct pending* pend;
  int p1, g = pp->game;

  if (!pIsPlaying(p)) {
    return COM_OK;
  }
  if (Check50MoveRule (p, g) || CheckRepetition(p, g)) {
    return COM_OK;
  }
  p1 = pp->opponent;

  if ((player_globals.parray[p1].simul_info != NULL) && (player_globals.parray[p1].simul_info->numBoards &&
        player_globals.parray[p1].simul_info->boards[player_globals.parray[p1].simul_info->onBoard] != g)) {
    pprintf(p, "You can only make requests when the simul player is at your board.\n");
    return COM_OK;
  }

  if ((pend = (find_pend(pp->opponent, p, PEND_DRAW))) != NULL) {
    delete_pending(pend);
    decline_withdraw_offers(p, -1, -1,DO_DECLINE);
    game_ended(g, (game_globals.garray[g].white == p) ? BLACK : WHITE, END_AGREEDDRAW);
  } else {
    pprintf(pp->opponent, "\n");
    pprintf_highlight(pp->opponent, "%s", pp->name);
    pprintf_prompt(pp->opponent, " offers you a draw.\n");
    pprintf(p, "Draw request sent.\n");
    add_request(p, pp->opponent, PEND_DRAW);
  }
  return COM_OK;
}

int com_pause(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int g, now;
  struct pending* pend;

  if (!pIsPlaying(p)) {
    return COM_OK;
  }
  g = pp->game;
  if (game_globals.garray[g].wTime == 0) {
    pprintf(p, "You can't pause untimed games.\n");
    return COM_OK;
  }
  if (game_globals.garray[g].clockStopped) {
    pprintf(p, "Game is already paused, use \"unpause\" to resume.\n");
    return COM_OK;
  }
  if ((pend = find_pend(pp->opponent, p, PEND_PAUSE)) != NULL) {
    delete_pending(pend);
    game_globals.garray[g].clockStopped = 1;
    /* Roll back the time */
    if (game_globals.garray[g].game_state.onMove == WHITE) {
      game_globals.garray[g].wTime += (game_globals.garray[g].lastDecTime - game_globals.garray[g].lastMoveTime);
    } else {
      game_globals.garray[g].bTime += (game_globals.garray[g].lastDecTime - game_globals.garray[g].lastMoveTime);
    }
    now = tenth_secs();
    if (game_globals.garray[g].numHalfMoves == 0)
      game_globals.garray[g].timeOfStart = now;
    game_globals.garray[g].lastMoveTime = now;
    game_globals.garray[g].lastDecTime = now;
    send_boards(g);
    pprintf_prompt(pp->opponent, "\n%s accepted pause. Game clock paused.\n",
		   pp->name);
    pprintf(p, "Game clock paused.\n");
  } else {
    pprintf(pp->opponent, "\n");
    pprintf_highlight(pp->opponent, "%s", pp->name);
    pprintf_prompt(pp->opponent, " requests to pause the game.\n");
    pprintf(p, "Pause request sent.\n");
    add_request(p, pp->opponent, PEND_PAUSE);
  }
  return COM_OK;
}

int com_unpause(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int g;
  int now;
  struct pending* pend;

  if (!pIsPlaying(p)) {
    return COM_OK;
  }

  g = pp->game;

  if (!game_globals.garray[g].clockStopped) {
    pprintf(p, "Game is not paused.\n");
    return COM_OK;
  }
  if ((pend = find_pend(pp->opponent, p, PEND_UNPAUSE)) != NULL) {
    delete_pending(pend);
    game_globals.garray[g].clockStopped = 0;
    now = tenth_secs();
    if (game_globals.garray[g].numHalfMoves == 0)
      game_globals.garray[g].timeOfStart = now;
    game_globals.garray[g].lastMoveTime = now;
    game_globals.garray[g].lastDecTime = now;
    send_boards(g);
    pprintf(p, "Game clock resumed.\n");
    pprintf_prompt(pp->opponent, "\nGame clock resumed.\n");
  } else {
    pprintf(pp->opponent, "\n");
    pprintf_highlight(pp->opponent, "%s", pp->name);
    pprintf_prompt(pp->opponent, " requests to unpause the game.\n");
    pprintf(p, "Unpause request sent.\n");
    add_request(p, pp->opponent, PEND_UNPAUSE);
  }
  return COM_OK;
}

int com_abort(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  struct pending* pend;
  int p1, g, myColor, yourColor, myGTime, yourGTime;
  int courtesyOK = 1;

  g = pp->game;
  if (!pIsPlaying(p))
    return COM_OK;

  p1 = pp->opponent;
  if (p == game_globals.garray[g].white) {
    myColor = WHITE;
    yourColor = BLACK;
    myGTime = game_globals.garray[g].wTime;
    yourGTime = game_globals.garray[g].bTime;
  } else {
    myColor = BLACK;
    yourColor = WHITE;
    myGTime = game_globals.garray[g].bTime;
    yourGTime = game_globals.garray[g].wTime;
  }
  if ((player_globals.parray[p1].simul_info != NULL) && 
     (player_globals.parray[p1].simul_info->numBoards &&
        player_globals.parray[p1].simul_info->boards[player_globals.parray[p1].simul_info->onBoard] != g)) {
    pprintf(p, "You can only make requests when the simul player is at your board.\n");
    return COM_OK;
  }
  if ((pend = find_pend(p1, p, PEND_ABORT)) != NULL) {
    delete_pending(pend);
    decline_withdraw_offers(p, -1, -1,DO_DECLINE);
    game_ended(g, yourColor, END_ABORT);
  } else {
    game_update_time(g);

    if (net_globals.con[pp->socket]->timeseal
        && game_globals.garray[g].game_state.onMove == myColor
        && game_globals.garray[g].flag_pending == FLAG_ABORT) {
      /* It's my move, opponent has asked for abort; I lagged out,
         my timeseal prevented courtesyabort, and I am sending an abort
         request before acknowledging (and processing) my opponent's
         courtesyabort.  OK, let's abort already :-). */
      decline_withdraw_offers(p, -1, -1,DO_DECLINE);
      game_ended(g, yourColor, END_ABORT);
    }

    if (net_globals.con[player_globals.parray[p1].socket]->timeseal) {	/* opp uses timeseal? */

      int yourRealTime = (myColor == WHITE  ?  game_globals.garray[g].bRealTime
       	                                    :  game_globals.garray[g].wRealTime);
      if (myGTime > 0 && yourGTime <= 0 && yourRealTime > 0) {
        /* Override courtesyabort; opponent still has time.  Check for lag. */
        courtesyOK = 0;

        if (game_globals.garray[g].game_state.onMove != myColor
            && game_globals.garray[g].flag_pending != FLAG_CHECKING) {
          /* Opponent may be lagging; let's ask. */
          game_globals.garray[g].flag_pending = FLAG_ABORT;
          game_globals.garray[g].flag_check_time = time(0);
          pprintf(p, "Opponent has timeseal; trying to courtesyabort.\n");
          pprintf(p1, "\n[G]\n");
          return COM_OK;
        }
      }
    }

    if (myGTime > 0 && yourGTime <= 0 && courtesyOK) {
      /* player wants to abort + opponent is out of time = courtesyabort */
      pprintf(p, "Since you have time, and your opponent has none, the game has been aborted.");
      pprintf(p1, "Your opponent has aborted the game rather than calling your flag.");
      decline_withdraw_offers(p, -1, -1, DO_DECLINE);
      game_ended(g, myColor, END_COURTESY);
    } else {
      pprintf(p1, "\n");
      pprintf_highlight(p1, "%s", pp->name);
      pprintf(p1, " would like to abort the game; ");
      pprintf_prompt(p1, "type \"abort\" to accept.\n");
      pprintf(p, "Abort request sent.\n");
      add_request(p, p1, PEND_ABORT);
    }
  }
  return COM_OK;
}

static int player_has_mating_material(struct game_state_t *gs, int color)
{
  int i, j;
  int piece;
  int minor_pieces = 0;

  for (i = 0; i < gs->files; i++)
    for (j = 0; j < gs->ranks; j++) {
      piece = gs->board[i][j];
      switch (piecetype(piece)) {
      case BISHOP:
      case KNIGHT:
	if (iscolor(piece, color))
	  minor_pieces++;
	break;
      case KING:
      case NOPIECE:
	break;
      default:
	if (iscolor(piece, color))
	  return 1;
      }
    }
  return ((minor_pieces > 1) ? 1 : 0);
}

int com_flag(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	struct game *gg;
	int g;
	int myColor;

	if (!pIsPlaying(p)) {
		return COM_OK;
	}
	g = pp->game;

	gg = &game_globals.garray[g];

	myColor = (p == gg->white ? WHITE : BLACK);
	if (gg->type == TYPE_UNTIMED) {
		pprintf(p, "You can't flag an untimed game.\n");
		return COM_OK;
	}
	if (gg->numHalfMoves < 2) {
		pprintf(p, "You cannot flag before both players have moved.\nUse abort instead.\n");
		return COM_OK;
	}
	game_update_time(g);
	
	{
		int myTime, yourTime, opp = pp->opponent, serverTime;
		
		if (net_globals.con[pp->socket]->timeseal) {    /* does caller use timeseal? */
			myTime = (myColor==WHITE?gg->wRealTime:gg->bRealTime);
		} else {
			myTime = (myColor == WHITE?gg->wTime:gg->bTime);
		}
		serverTime = (myColor == WHITE?gg->bTime:gg->wTime);
		
		if (net_globals.con[player_globals.parray[opp].socket]->timeseal) {	/* opp uses timeseal? */
			yourTime = (myColor == WHITE?gg->bRealTime:gg->wRealTime);
		} else {
			yourTime = serverTime;
		}

		/* the clocks to compare are now in myTime and yourTime */
		if ((myTime <= 0) && (yourTime <= 0)) {
			decline_withdraw_offers(p, -1, -1,DO_DECLINE);
			game_ended(g, myColor, END_BOTHFLAG);
			return COM_OK;
		}

		if (yourTime > 0) {
			/* Opponent still has time, but if that's only because s/he
			 * may be lagging, we should ask for an acknowledgement and then
			 * try to call the flag. */
			
			if (serverTime <= 0 && gg->game_state.onMove != myColor
			    && gg->flag_pending != FLAG_CHECKING) {				
				/* server time thinks opponent is down, but RealTIme disagrees.
				 * ask client to acknowledge it's alive. */				
				gg->flag_pending = FLAG_CALLED;
				gg->flag_check_time = time(0);
				pprintf(p, "Opponent has timeseal; checking if (s)he's lagging.\n");
				pprintf (opp, "\n[G]\n");
				return COM_OK;
			}
			
			/* if we're here, it means one of:
			 * 1. the server agrees opponent has time, whether lagging or not.
			 * 2. opp. has timeseal (if yourTime != serverTime), had time left
			 *    after the last move (yourTime > 0), and it's still your move.
			 * 3. we're currently checking a flag call after having receiving
			 *    acknowledgement from the other timeseal (and would have reset
			 *    yourTime if the flag were down). */
			
			pprintf(p, "Your opponent is not out of time!\n");
			return COM_OK;
		}
	}
	
	decline_withdraw_offers(p, -1, -1,DO_DECLINE);
	if (player_has_mating_material(&gg->game_state, myColor))
		game_ended(g, myColor, END_FLAG);
	else
		game_ended(g, myColor, END_FLAGNOMATERIAL);
	return COM_OK;
}

int com_adjourn(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  struct pending* pend;
  int p1, g, myColor, yourColor;

  if (!pIsPlaying(p))
    return COM_OK;

  p1 = pp->opponent;
  g = pp->game;
  if (!CheckPFlag(p, PFLAG_REG) || !CheckPFlag(p, PFLAG_REG)) {
    pprintf(p, "Both players must be registered to adjourn a game.  Use \"abort\".\n");
    return COM_OK;
  }
  if (game_globals.garray[g].link >= 0) {
    pprintf(p, "Bughouse games cannot be adjourned.\n");
    return COM_OK;
  }
  myColor = (p == game_globals.garray[g].white ? WHITE : BLACK);
  yourColor = (myColor == WHITE ? BLACK : WHITE);

  if ((pend = find_pend(p1, p, PEND_ADJOURN)) != NULL) {
    delete_pending(pend);
    decline_withdraw_offers(p, -1, -1,DO_DECLINE);
    game_ended(pp->game, yourColor, END_ADJOURN);
  } else {
    game_update_time(g);
    if (((myColor == WHITE) && (game_globals.garray[g].wTime > 0) && (game_globals.garray[g].bTime <= 0))
	|| ((myColor == BLACK) && (game_globals.garray[g].bTime > 0) && (game_globals.garray[g].wTime <= 0))) {
/* player wants to adjourn + opponent is out of time = courtesyadjourn */
      pprintf(p, "Since you have time, and your opponent has none, the game has been adjourned.");
      pprintf(p1, "Your opponent has adjourned the game rather than calling your flag.");
      decline_withdraw_offers(p, -1, -1,DO_DECLINE);
      game_ended(g, myColor, END_COURTESYADJOURN);
    } else {
      pprintf(p1, "\n");
      pprintf_highlight(p1, "%s", pp->name);
      pprintf(p1, " would like to adjourn the game; ");
      pprintf_prompt(p1, "type \"adjourn\" to accept.\n");
      pprintf(p, "Adjourn request sent.\n");
      add_request(p, p1, PEND_ADJOURN);
    }
  }
  return COM_OK;
}

int com_takeback(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int nHalfMoves = 1, g, i, p1, pend_half_moves;
  struct pending* from;

  if (!pIsPlaying(p))
    return COM_OK;

  p1 = pp->opponent;
  if ((player_globals.parray[p1].simul_info != NULL) && 
     (player_globals.parray[p1].simul_info->numBoards &&
        player_globals.parray[p1].simul_info->boards[player_globals.parray[p1].simul_info->onBoard] !=
        pp->game)) {
    pprintf(p, "You can only make requests when the simul player is at your board.\n");
    return COM_OK;
  }

  g = pp->game;
  if (game_globals.garray[g].link >= 0) {
    pprintf(p, "Takeback not implemented for bughouse games yet.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_INT) {
    nHalfMoves = param[0].val.integer;
    if (nHalfMoves <= 0) {
      pprintf (p,"You can't takeback less than 1 move.\n");
      return COM_OK;
    }
  }
  if ((from = find_pend(pp->opponent, p, PEND_TAKEBACK)) != NULL) {
    pend_half_moves = from->wtime;
    delete_pending(from);
    if (pend_half_moves == nHalfMoves) {
      /* Doing the takeback */
      decline_withdraw_offers(p, -1, -PEND_SIMUL,DO_DECLINE);
      for (i = 0; i < nHalfMoves; i++) {
	if (backup_move(g, REL_GAME) != MOVE_OK) {
	  pprintf(game_globals.garray[g].white, "Can only backup %d moves\n", i);
	  pprintf(game_globals.garray[g].black, "Can only backup %d moves\n", i);
	  break;
	}
      }

      game_globals.garray[g].wTimeWhenReceivedMove = 0;
      game_globals.garray[g].bTimeWhenReceivedMove = 0;

      send_boards(g);
    } else {

      if (!game_globals.garray[g].numHalfMoves) {
        pprintf(p, "There are no moves in your game.\n");
        pprintf_prompt(pp->opponent, "\n%s has declined the takeback request.\n", 
		       pp->name);
        return COM_OK;
      }
 
      if (game_globals.garray[g].numHalfMoves < nHalfMoves) {
	pprintf(p, "There are only %d half moves in your game.\n", game_globals.garray[g].numHalfMoves);
	pprintf_prompt(pp->opponent, "\n%s has declined the takeback request.\n", 
		       pp->name);
	return COM_OK;
      }
      pprintf(p, "You disagree on the number of half-moves to takeback.\n");
      pprintf(p, "Alternate takeback request sent.\n");
      pprintf_prompt(pp->opponent, "\n%s proposes a different number (%d) of half-move(s).\n", pp->name, nHalfMoves);
      from = add_request(p, pp->opponent, PEND_TAKEBACK);
      from->wtime = nHalfMoves;
    }
  } else {

    if (!game_globals.garray[g].numHalfMoves) {
      pprintf(p, "There are no moves in your game.\n");
      return COM_OK;
    }
    if (game_globals.garray[g].numHalfMoves < nHalfMoves) {
      pprintf(p, "There are only %d half moves in your game.\n", game_globals.garray[g].numHalfMoves);
      return COM_OK;
    }
    pprintf(pp->opponent, "\n");
    pprintf_highlight(pp->opponent, "%s", pp->name);
    pprintf_prompt(pp->opponent, " would like to take back %d half move(s).\n",
 	   nHalfMoves);
    pprintf(p, "Takeback request sent.\n");
    from = add_request(p, pp->opponent, PEND_TAKEBACK);
    from->wtime = nHalfMoves;
  }
  return COM_OK;
}


int com_switch(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int g = pp->game, tmp, now, p1;
  char *strTmp;
  struct pending* pend;

  if (!pIsPlaying(p))
    return COM_OK;

  p1 = pp->opponent;
  if ((player_globals.parray[p1].simul_info != NULL) && (player_globals.parray[p1].simul_info->numBoards &&
        player_globals.parray[p1].simul_info->boards[player_globals.parray[p1].simul_info->onBoard] != g)) {
    pprintf(p, "You can only make requests when the simul player is at your board.\n");
    return COM_OK;
  }

  if (game_globals.garray[g].link >= 0) {
    pprintf(p, "Switch not implemented for bughouse games.\n");
    return COM_OK;
  }
  if ((pend = find_pend(pp->opponent, p, PEND_SWITCH)) != NULL) {
    delete_pending(pend);
    /* Doing the switch */
    decline_withdraw_offers(p, -1, -PEND_SIMUL,DO_DECLINE);

    tmp = game_globals.garray[g].white;
    game_globals.garray[g].white = game_globals.garray[g].black;
    game_globals.garray[g].black = tmp;
    pp->side = (pp->side == WHITE) ? BLACK : WHITE;
    strTmp = strdup(game_globals.garray[g].white_name);
    strcpy(game_globals.garray[g].white_name, game_globals.garray[g].black_name);
    strcpy(game_globals.garray[g].black_name, strTmp);
    free(strTmp);

    player_globals.parray[pp->opponent].side =
      (player_globals.parray[pp->opponent].side == WHITE) ? BLACK : WHITE;
    /* Roll back the time */
    if (game_globals.garray[g].game_state.onMove == WHITE) {
      game_globals.garray[g].wTime += (game_globals.garray[g].lastDecTime - game_globals.garray[g].lastMoveTime);
    } else {
      game_globals.garray[g].bTime += (game_globals.garray[g].lastDecTime - game_globals.garray[g].lastMoveTime);
    }
    now = tenth_secs();
    if (game_globals.garray[g].numHalfMoves == 0)
      game_globals.garray[g].timeOfStart = now;
    game_globals.garray[g].lastMoveTime = now;
    game_globals.garray[g].lastDecTime = now;
    send_boards(g);
    return COM_OK;
  }
  if (game_globals.garray[g].rated && game_globals.garray[g].numHalfMoves > 0) {
    pprintf(p, "You cannot switch sides once a rated game is underway.\n");
    return COM_OK;
  }
  pprintf(pp->opponent, "\n");
  pprintf_highlight(pp->opponent, "%s", pp->name);
  pprintf_prompt(pp->opponent, " would like to switch sides.\nType \"accept\" to switch sides, or \"decline\" to refuse.\n");
  pprintf(p, "Switch request sent.\n");
  add_request(p, pp->opponent, PEND_SWITCH);
  return COM_OK;
}

int com_time(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1, g;

  if (param[0].type == TYPE_NULL) {
    g = pp->game;
    if (!pIsPlaying(p))
      return COM_OK;
  } else {
    g = GameNumFromParam(p, &p1, &param[0]);
    if (g < 0)
      return COM_OK;
  }
  if ((g < 0) || (g >= game_globals.g_num) || (game_globals.garray[g].status != GAME_ACTIVE)) {
    pprintf(p, "There is no such game.\n");
    return COM_OK;
  }
  game_update_time(g);
  pprintf(p, "White (%s) : %d mins, %d secs\n",
	  player_globals.parray[game_globals.garray[g].white].name,
	  game_globals.garray[g].wTime / 600,
	  (game_globals.garray[g].wTime - ((game_globals.garray[g].wTime / 600) * 600)) / 10);
  pprintf(p, "Black (%s) : %d mins, %d secs\n",
	  player_globals.parray[game_globals.garray[g].black].name,
	  game_globals.garray[g].bTime / 600,
	  (game_globals.garray[g].bTime - ((game_globals.garray[g].bTime / 600) * 600)) / 10);
  return COM_OK;
}

int com_ptime(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int retval, part = pp->partner;

  if (part < 0) {
    pprintf(p, "You do not have a partner.\n");
    return COM_OK;
  }
  retval = pcommand (p, "time %s", player_globals.parray[part].name);
  if (retval == COM_OK)
    return COM_OK_NOPROMPT;
  else
    return retval;
}

int com_boards(int p, param_list param)
{
  char *category = NULL;
  char dname[MAX_FILENAME_SIZE];
  DIR *dirp;
  struct dirent *dp;

  if (param[0].type == TYPE_WORD)
    category = param[0].val.word;
  if (category) {
    pprintf(p, "Boards Available For Category %s:\n", category);
    sprintf(dname, "%s/%s", BOARD_DIR, category);
  } else {
    pprintf(p, "Categories Available:\n");
    sprintf(dname, "%s", BOARD_DIR);
  }
  dirp = opendir(dname);
  if (!dirp) {
    pprintf(p, "No such category %s, try \"boards\".\n", category);
    return COM_OK;
  }

/* YUK! what a mess, how about printing an ordered directory? - DAV*/

  for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
    if (!strcmp(dp->d_name, "."))
      continue;
    if (!strcmp(dp->d_name, ".."))
      continue;
    pprintf(p, "%s\n", dp->d_name);
  }
  closedir(dirp);
  return COM_OK;
}

int com_simmatch(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1, g, adjourned;
  int num;
  char tmp[100];
  struct pending* pend;
  char* board = NULL;
  char* category = NULL;
  char fname[MAX_FILENAME_SIZE];

  if (pp->game >=0) {
    if (game_globals.garray[pp->game].status == GAME_EXAMINE) {
      pprintf(p, "You are still examining a game.\n");
      return COM_OK;
    }
    if (game_globals.garray[pp->game].status == GAME_SETUP) {
      pprintf(p, "You are still setting up a position.\n");
      return COM_OK;
    }
  } 
  p1 = player_find_part_login(param[0].val.word);
  if (p1 < 0) {
    pprintf(p, "No user named \"%s\" is logged in.\n", param[0].val.word);
    return COM_OK;
  }
  if (p == p1) {
    pprintf(p, "You can't simmatch yourself!\n");
    return COM_OK;
  }
  if ((pend = find_pend(p1, p, PEND_SIMUL)) != NULL) {

    /* Accepting Simul ! */

    if ((pp->simul_info != NULL) && 
       (pp->simul_info->numBoards >= MAX_SIMUL)) {
      pprintf(p, "You are already playing the maximum of %d boards.\n", MAX_SIMUL);
      pprintf(p1, "Simul request removed, boards filled.\n");
      delete_pending(pend);
      return COM_OK;
    }
    unobserveAll(p);		/* stop observing when match starts */
    unobserveAll(p1);

    g = game_new();
    adjourned = 0;
    if (game_read(g, p, p1) >= 0) {
      adjourned = 1;
      delete_pending(pend);
    }

    if (!adjourned) {		/* no adjourned game, so begin a new game */

      if ((pend->category != NULL) && (pend->board_type != NULL)) {
        board = strdup(pend->category);
        category = strdup(pend->board_type);
      } 

      delete_pending(pend);

      if (create_new_match(g,p, p1, 0, 0, 0, 0, 0, ((board == NULL) ? "\0" : board), ((category == NULL) ? "\0" : category), 1,1) == COM_FAILED) {
	pprintf(p, "There was a problem creating the new match.\n");
	pprintf_prompt(p1, "There was a problem creating the new match.\n");
        game_remove(g);

        if (board != NULL) {
          free (board);
          free (category);
        }
	return COM_OK;
      }

      if (board != NULL) {
        free (board);
        free (category);
      }

    } else {			/* resume adjourned game */
      game_delete(p, p1);

      sprintf(tmp, "{Game %d (%s vs. %s) Continuing %s %s simul.}\n", g + 1, pp->name, player_globals.parray[p1].name, rstr[game_globals.garray[g].rated], bstr[game_globals.garray[g].type]);
      pprintf(p, tmp);
      pprintf(p1, tmp);

      game_globals.garray[g].white = p;
      game_globals.garray[g].black = p1;
      game_globals.garray[g].status = GAME_ACTIVE;
      game_globals.garray[g].startTime = tenth_secs();
      game_globals.garray[g].lastMoveTime = game_globals.garray[g].startTime;
      game_globals.garray[g].lastDecTime = game_globals.garray[g].startTime;
      pp->game = g;
      pp->opponent = p1;
      pp->side = WHITE;
      player_globals.parray[p1].game = g;
      player_globals.parray[p1].opponent = p;
      player_globals.parray[p1].side = BLACK;
      send_boards(g);
    }

    if (pp->simul_info == NULL) {
      pp->simul_info = (struct simul_info_t *) malloc(sizeof(struct simul_info_t));
      pp->simul_info->numBoards = 0;
      pp->simul_info->onBoard = 0;
      pp->simul_info->num_wins = pp->simul_info->num_draws
        = pp->simul_info->num_losses = 0;
    }
    num = pp->simul_info->numBoards;
    /*    pp->simul_info->results[num] = -1; */
    pp->simul_info->boards[num] = pp->game;
    pp->simul_info->numBoards++;
    if (pp->simul_info->numBoards > 1 &&
	pp->simul_info->onBoard >= 0)
      player_goto_board(p, pp->simul_info->onBoard);
    else
      pp->simul_info->onBoard = 0;
    return COM_OK;
  }
  if (find_pend(-1, p, PEND_SIMUL) != NULL) {
    pprintf(p, "You cannot be the simul giver and request to join another simul.\nThat would just be too confusing for me and you.\n");
    return COM_OK;
  }
  if (pp->simul_info != NULL) {
    if (pp->simul_info->numBoards) {
      pprintf(p, "You cannot be the simul giver and request to join another simul.\nThat would just be too confusing for me and you.\n");
      return COM_OK;
    }
  }
  if (pp->game >=0) {
    pprintf(p, "You are already playing a game.\n");
    return COM_OK;
  }
  if (!CheckPFlag(p1, PFLAG_SIMOPEN)) {
    pprintf_highlight(p, "%s", player_globals.parray[p1].name);
    pprintf(p, " is not open to receiving simul requests.\n");
    return COM_OK;
  }
  if ((player_globals.parray[p1].simul_info != NULL) && (player_globals.parray[p1].simul_info->numBoards >= MAX_SIMUL)) {
    pprintf_highlight(p, "%s", player_globals.parray[p1].name);
    pprintf(p, " is already playing the maximum of %d boards.\n", MAX_SIMUL);
    return COM_OK;
  }

/* loon: checking for some crazy situations we can't allow :) */

  if ((player_globals.parray[p1].simul_info != NULL) && (player_globals.parray[p1].game >=0) && (player_globals.parray[p1].simul_info->numBoards == 0)) {
    pprintf_highlight(p, "%s", player_globals.parray[p1].name);
    if (player_globals.parray[game_globals.garray[player_globals.parray[p1].game].white].simul_info->numBoards) {
      pprintf(p, " is playing in ");
      pprintf_highlight(p, "%s", player_globals.parray[player_globals.parray[p1].opponent].name);
      pprintf(p, "'s simul, and can't accept.\n");
    } else {
      pprintf(p, " can't begin a simul while playing a non-simul game.\n");
    }
    return COM_OK;
  }

  g = game_new();		/* Check if an adjourned untimed game */
  adjourned = ((game_read(g, p, p1) < 0) && (game_read(g, p1, p) < 0)) ? 0 : 1;
  if (adjourned) {
    if (!(game_globals.garray[g].type == TYPE_UNTIMED))
      adjourned = 0;
  }
  game_remove(g);

  pend = add_request(p, p1, PEND_SIMUL);

  if ((param[1].type == TYPE_WORD) && (param[2].type == TYPE_WORD)) {

    sprintf(fname, "%s/%s/%s", BOARD_DIR, param[1].val.word , param[2].val.word);
    if (!file_exists(fname)) {
      pprintf(p, "No such category/board: %s/%s\n", param[1].val.word , param[2].val.word);
      return COM_OK;
    }
    pend->category = strdup(param[1].val.word);
    pend->board_type = strdup(param[2].val.word);
  } else {
    pend->category = NULL;
    pend->board_type = NULL;
  }
 
  pprintf(p1, "\n");
  pprintf_highlight(p1, "%s", pp->name);
  if (adjourned) {
    pprintf_prompt(p1, " requests to continue an adjourned simul game.\n");
    pprintf(p, "Request to resume simul sent. Adjourned game found.\n");
  } else {
    if (pend->category == NULL)
      pprintf_prompt(p1, " requests to join a simul match with you.\n");
    else
      pprintf_prompt(p1, " requests to join a %s %s simul match with you.\n",
                pend->category,pend->board_type);
    pprintf(p, "Simul match request sent.\n");
  }
  return COM_OK;
}

int com_goboard(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int on, g, p1, gamenum;

  if (pp->simul_info == NULL) {
    pprintf(p, "You are not giving a simul.\n");
    return COM_OK;
  }

  if (!pp->simul_info->numBoards) {
    pprintf(p, "You are not giving a simul.\n");
    return COM_OK;
  }

  if (param[0].type == TYPE_WORD) {

    p1 = player_find_part_login(param[0].val.word);
    if (p1 < 0) {
      pprintf(p, "No user named \"%s\" is logged in.\n", param[0].val.word);
      return COM_OK;
    }
    if (p == p1) {
      pprintf(p, "You can't goboard yourself!\n");
      return COM_OK;
    }

    gamenum = player_globals.parray[p1].game;
    if (gamenum < 0) {
      pprintf (p,"%s is not playing a game.\n", player_globals.parray[p1].login);
      return COM_OK;
    }

  } else { 
    gamenum = param[0].val.integer - 1;
    if (gamenum < 0)
      gamenum = 0;
  }

  on = pp->simul_info->onBoard;
  g = pp->simul_info->boards[on];
  if (gamenum == g) {
    pprintf(p, "You are already at that board!\n");
    return COM_OK;
  }
  if (pp->simul_info->numBoards > 1) {
    decline_withdraw_offers(p, -1, -PEND_SIMUL,DO_DECLINE);
    if (player_goto_simulgame_bynum(p, gamenum) !=-1) {
      if (g >= 0) {
	pprintf(game_globals.garray[g].black, "\n");
	pprintf_highlight(game_globals.garray[g].black, "%s", pp->name);
	pprintf_prompt(game_globals.garray[g].black, " has moved away from your board.\n");
      }
    } else
    pprintf(p, "You are not playing that game/person.\n");
  } else
    pprintf(p, "You are only playing one board!\n");
  return COM_OK;
}

int com_simnext(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int on, g;

  if (pp->simul_info == NULL) {
    pprintf(p, "You are not giving a simul.\n");
    return COM_OK;
  }

  if (!pp->simul_info->numBoards) {
    pprintf(p, "You are not giving a simul.\n");
    return COM_OK;
  }

  if (pp->simul_info->numBoards > 1) {
    decline_withdraw_offers(p, -1, -PEND_SIMUL,DO_DECLINE);
    on = pp->simul_info->onBoard;
    g = pp->simul_info->boards[on];
    if (g >= 0) {
      pprintf(game_globals.garray[g].black, "\n");
      pprintf_highlight(game_globals.garray[g].black, "%s", pp->name);
      pprintf_prompt(game_globals.garray[g].black, " is moving away from your board.\n");
      player_goto_next_board(p);
    }
  } else
    pprintf(p, "You are only playing one board!\n");
  return COM_OK;
}

int com_simprev(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int on, g;

  if (pp->simul_info == NULL) {
    pprintf(p, "You are not giving a simul.\n");
    return COM_OK;
  }

  if (!pp->simul_info->numBoards) {
    pprintf(p, "You are not giving a simul.\n");
    return COM_OK;
  }
  if (pp->simul_info->numBoards > 1) {
    decline_withdraw_offers(p, -1, -PEND_SIMUL,DO_DECLINE);
    on = pp->simul_info->onBoard;
    g = pp->simul_info->boards[on];
    if (g >= 0) {
      pprintf(game_globals.garray[g].black, "\n");
      pprintf_highlight(game_globals.garray[g].black, "%s", pp->name);
      pprintf_prompt(game_globals.garray[g].black, " is moving back to the previous board.\n");
    }
    player_goto_prev_board(p);
  } else
    pprintf(p, "You are only playing one board!\n");
  return COM_OK;
}

int com_simgames(int p, param_list param)
{
  int p1 = p;

  if (param[0].type == TYPE_WORD) {
    if ((p1 = player_find_part_login(param[0].val.word)) < 0) {
      pprintf(p, "No player named %s is logged in.\n", param[0].val.word);
      return COM_OK;
    }
  }
  if (p1 == p)
    pprintf(p, "You are playing %d simultaneous games.\n",
	    player_num_active_boards(p1));
  else
    pprintf(p, "%s is playing %d simultaneous games.\n", player_globals.parray[p1].name,
	    player_num_active_boards(p1));
  return COM_OK;
}

int com_simpass(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int g, p1, on;

  if (!pIsPlaying(p))
    return COM_OK;

  g = pp->game;
  p1 = game_globals.garray[g].white;

  if (player_globals.parray[p1].simul_info == NULL) {
    pprintf(p, "You are not participating in a simul.\n");
    return COM_OK;
  }

  if (!player_globals.parray[p1].simul_info->numBoards) {
    pprintf(p, "You are not participating in a simul.\n");
    return COM_OK;
  }
  if (p == p1) {
    pprintf(p, "You are the simul holder and cannot pass!\n");
    return COM_OK;
  }
  if (player_num_active_boards(p1) == 1) {
    pprintf(p, "This is the only game, so passing is futile.\n");
    return COM_OK;
  }
  on = player_globals.parray[p1].simul_info->onBoard;
  if (player_globals.parray[p1].simul_info->boards[on] != g) {
    pprintf(p, "You cannot pass until the simul holder arrives!\n");
    return COM_OK;
  }
  if (game_globals.garray[g].passes >= MAX_SIMPASS) {
    Bell (p);
    pprintf(p, "You have reached your maximum of %d pass(es).\n", MAX_SIMPASS);
    pprintf(p, "Please move IMMEDIATELY!\n");
    pprintf_highlight(p1, "%s", pp->name);
    pprintf_prompt(p1, " tried to pass, but is out of passes.\n");
    return COM_OK;
  }
  decline_withdraw_offers(p, -1, -PEND_SIMUL,DO_DECLINE);

  game_globals.garray[g].passes++;
  pprintf(p, "You have passed and have %d pass(es) left.\n",
	  (MAX_SIMPASS - game_globals.garray[g].passes));
  pprintf_highlight(p1, "%s", pp->name);
  pprintf_prompt(p1, " has decided to pass and has %d pass(es) left.\n",
		 (MAX_SIMPASS - game_globals.garray[g].passes));
  player_goto_next_board(p1);
  return COM_OK;
}

int com_simabort(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];

  if (pp->simul_info == NULL) {
    pprintf(p, "You are not giving a simul.\n");
    return COM_OK;
  }

  if (!pp->simul_info->numBoards) {
    pprintf(p, "You are not giving a simul.\n");
    return COM_OK;
  }
  decline_withdraw_offers(p, -1, -PEND_SIMUL,DO_DECLINE);
  game_ended(pp->simul_info->boards[pp->simul_info->onBoard],
	     WHITE, END_ABORT);
  return COM_OK;
}

int com_simallabort(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int i;

  if (pp->simul_info == NULL) {
    pprintf(p, "You are not giving a simul.\n");
    return COM_OK;
  }

  if (!pp->simul_info->numBoards) {
    pprintf(p, "You are not giving a simul.\n");
    return COM_OK;
  }

  decline_withdraw_offers(p, -1, -PEND_SIMUL,DO_DECLINE);
  for (i = 0; i < pp->simul_info->numBoards; i++)
    if (pp->simul_info->boards[i] >= 0)
      game_ended(pp->simul_info->boards[i],
		 WHITE, END_ABORT);

  return COM_OK;
}

int com_simadjourn(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];

  if (pp->simul_info == NULL) {
    pprintf(p, "You are not giving a simul.\n");
    return COM_OK;
  }

  if (!pp->simul_info->numBoards) {
    pprintf(p, "You are not giving a simul.\n");
    return COM_OK;
  }
  decline_withdraw_offers(p, -1, -PEND_SIMUL,DO_DECLINE);
  game_ended(pp->simul_info->boards[pp->simul_info->onBoard],
	     WHITE, END_ADJOURN);
  return COM_OK;
}

int com_simalladjourn(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int i;

  if (pp->simul_info == NULL) {
    pprintf(p, "You are not giving a simul.\n");
    return COM_OK;
  }

  if (!pp->simul_info->numBoards) {
    pprintf(p, "You are not giving a simul.\n");
    return COM_OK;
  }
  decline_withdraw_offers(p, -1, -PEND_SIMUL,DO_DECLINE);
  for (i = 0; i < pp->simul_info->numBoards; i++)
    if (pp->simul_info->boards[i] >= 0)
      game_ended(pp->simul_info->boards[i],
		 WHITE, END_ADJOURN);

  return COM_OK;
}

int com_moretime(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int g, increment;

  if ((pp->game >=0) &&((game_globals.garray[pp->game].status == GAME_EXAMINE) ||
        (game_globals.garray[pp->game].status == GAME_SETUP))) {
    pprintf(p, "You cannot use moretime in an examined game.\n");
    return COM_OK;
  }
  increment = param[0].val.integer;
  if (increment <= 0) {
    pprintf(p, "Moretime requires an integer value greater than zero.\n");
    return COM_OK;
  }
  if (!pIsPlaying(p))
    return COM_OK;
 
  if (increment > 600) {
    pprintf(p, "Moretime has a maximum limit of 600 seconds.\n");
    increment = 600;
  }
  g = pp->game;
  if (game_globals.garray[g].white == p) {
    game_globals.garray[g].bTime += increment * 10;
    game_globals.garray[g].bRealTime += increment * 10 * 100;
    pprintf(p, "%d seconds were added to your opponents clock\n",
	    increment);
    pprintf_prompt(pp->opponent,
		   "\nYour opponent has added %d seconds to your clock.\n",
		   increment);
  }
  if (game_globals.garray[g].black == p) {
    game_globals.garray[g].wTime += increment * 10;;
    game_globals.garray[g].wRealTime += increment * 10 * 100;
    pprintf(p, "%d seconds were added to your opponents clock\n",
	    increment);
    pprintf_prompt(pp->opponent,
		   "\nYour opponent has added %d seconds to your clock.\n",
		   increment);
  }
  return COM_OK;
}

