/*
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

/**
  matchproc.c
  Feb 9 1996 - rewritten - DAV

*/

#include "includes.h"

const char *colorstr[] = {"", "[black] ", "[white] "};
const char *adjustr[] = {"", " (adjourned)"};

static void prepare_match(int g,int wt,int bt,int winc,int binc,int wp, int bp,int rated)
{

  wt = wt * 60;                 /* To Seconds */
  bt = bt * 60;
  game_globals.garray[g].white = wp;
  game_globals.garray[g].black = bp;
  strcpy(game_globals.garray[g].white_name, player_globals.parray[wp].name);
  strcpy(game_globals.garray[g].black_name, player_globals.parray[bp].name);
  game_globals.garray[g].status = GAME_ACTIVE;
  if ((game_globals.garray[g].type == TYPE_UNTIMED) || (game_globals.garray[g].type == TYPE_NONSTANDARD))
    game_globals.garray[g].rated = 0;
  else
    game_globals.garray[g].rated = rated;
  game_globals.garray[g].private = BoolCheckPFlag(wp, PFLAG_PRIVATE)
                      || BoolCheckPFlag(bp, PFLAG_PRIVATE);
  game_globals.garray[g].white = wp;
  if (game_globals.garray[g].type == TYPE_BLITZ) {
    game_globals.garray[g].white_rating = player_globals.parray[wp].b_stats.rating;
    game_globals.garray[g].black_rating = player_globals.parray[bp].b_stats.rating;
  } else if (game_globals.garray[g].type == TYPE_WILD) {
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

  game_globals.garray[g].game_state.gameNum = g;

  game_globals.garray[g].wTime = wt * 10;
  game_globals.garray[g].wInitTime = wt * 10;
  game_globals.garray[g].wIncrement = winc * 10;
  game_globals.garray[g].bTime = bt * 10;

  if (game_globals.garray[g].type != TYPE_UNTIMED) {
    if (wt == 0)
        game_globals.garray[g].wTime = 100;
    if (bt == 0)
        game_globals.garray[g].bTime = 100;
  } /* 0 x games start with 10 seconds */

  game_globals.garray[g].wRealTime = game_globals.garray[g].wTime * 100;
  game_globals.garray[g].bRealTime = game_globals.garray[g].bTime * 100;
  game_globals.garray[g].wTimeWhenReceivedMove = 0;
  game_globals.garray[g].bTimeWhenReceivedMove = 0;

  game_globals.garray[g].bInitTime = bt * 10;
  game_globals.garray[g].bIncrement = binc * 10;

  if (game_globals.garray[g].game_state.onMove == BLACK) {   /* Start with black */
    game_globals.garray[g].numHalfMoves = 1;
    game_globals.garray[g].moveListSize = 1;
    game_globals.garray[g].moveList = (struct move_t *) malloc(sizeof(struct move_t));
    game_globals.garray[g].moveList[0].fromFile = -1;
    game_globals.garray[g].moveList[0].fromRank = -1;
    game_globals.garray[g].moveList[0].toFile = -1;
    game_globals.garray[g].moveList[0].toRank = -1;
    game_globals.garray[g].moveList[0].color = WHITE;
    strcpy(game_globals.garray[g].moveList[0].moveString, "NONE");
    strcpy(game_globals.garray[g].moveList[0].algString, "NONE");
  } else {
    game_globals.garray[g].numHalfMoves = 0;
    game_globals.garray[g].moveListSize = 0;
    game_globals.garray[g].moveList = NULL;
  }

  game_globals.garray[g].timeOfStart = tenth_secs();
  game_globals.garray[g].startTime = tenth_secs();
  game_globals.garray[g].lastMoveTime = game_globals.garray[g].startTime;
  game_globals.garray[g].lastDecTime = game_globals.garray[g].startTime;
  game_globals.garray[g].clockStopped = 0;

}

static void pick_colors(int* wp,int* bp,int white,int wt,int bt,int winc,
								int binc)

{
 int reverse = 0;

  if (white == 0) {
    reverse = 1; /* did challenger ask for black? */

  } else if (white == -1) { /* unknown */

    if ((wt == bt) && (winc == binc)) { /* if diff times challenger is white */

      if (CheckPFlag(*wp, PFLAG_LASTBLACK)==CheckPFlag(*bp, PFLAG_LASTBLACK)) {
        if ((player_globals.parray[*wp].num_white - player_globals.parray[*wp].num_black) >
          (player_globals.parray[*bp].num_white - player_globals.parray[*bp].num_black))
          reverse = 1; /* whose played the most extra whites gets black */

      } else if (!CheckPFlag(*wp, PFLAG_LASTBLACK))
        reverse = 1;

    } else
      reverse = 1;              
  }
  if (reverse) {
    int tmp = *wp;
    *wp = *bp;
    *bp = tmp;
  }
}

static void output_match_messages(int wp,int bp,int g, char* mess)
{

  int printed;
  int avail_printed = 0;
  char notavail_white[200];
  char notavail_black[200];
  int p;
  char *outStr;

  notavail_white[0] = '\0';
  notavail_black[0] = '\0';

  asprintf(&outStr,"\nCreating: %s (%d) %s (%d) %s %s %d %d\n",
	   player_globals.parray[wp].name,
	   game_globals.garray[g].white_rating,
	   player_globals.parray[bp].name,
	   game_globals.garray[g].black_rating,
	   rstr[game_globals.garray[g].rated],
	   //bstr[game_globals.garray[g].type],
	   game_globals.garray[g].variant,
	   game_globals.garray[g].wInitTime/600, 
	   game_globals.garray[g].wIncrement/10);
  pprintf(wp, "%s", outStr);
  pprintf(bp, "%s", outStr);
  free(outStr);

  asprintf(&outStr, "\n{Game %d (%s vs. %s) %s %s %s match.}\n",
          g + 1, player_globals.parray[wp].name,
          player_globals.parray[bp].name,
	  mess,
          rstr[game_globals.garray[g].rated],
          //bstr[game_globals.garray[g].type]);
	  game_globals.garray[g].variant);
  pprintf(wp, "%s", outStr);
  pprintf(bp, "%s", outStr);

  if (!(player_num_active_boards(wp)) && CheckPFlag(wp, PFLAG_OPEN))
/* open may be 0 if a simul */
    getnotavailmess(wp,notavail_white);
  if (CheckPFlag(bp, PFLAG_OPEN))
    getnotavailmess(bp,notavail_black);
  for (p = 0; p < player_globals.p_num; p++) {
    struct player *pp = &player_globals.parray[p];
    int gnw, gnb;
    printed = 0;

    if ((p == wp) || (p == bp))
      continue;
    if (pp->status != PLAYER_PROMPT)
      continue;
    if (CheckPFlag(p, PFLAG_GIN)) {
      pprintf(p, "%s", outStr);
      printed = 1;
    }
    gnw = in_list(p, L_GNOTIFY, player_globals.parray[wp].login);
    gnb = in_list(p, L_GNOTIFY, player_globals.parray[bp].login);
    if (gnw || gnb) {
      pprintf(p, "\nGame notification: ");

      if (gnw)
        pprintf_highlight(p, player_globals.parray[wp].name);
      else
        pprintf(p, player_globals.parray[wp].name);

      pprintf(p, " (%s) vs. ",
              ratstr(GetRating(&player_globals.parray[wp], game_globals.garray[g].type)));

      if (gnb)
        pprintf_highlight(p, player_globals.parray[bp].name);
      else
        pprintf(p, player_globals.parray[bp].name);
      pprintf(p, " (%s) %s %s %d %d: Game %d\n",
                     ratstr(GetRating(&player_globals.parray[bp], game_globals.garray[g].type)),
                     rstr[game_globals.garray[g].rated], bstr[game_globals.garray[g].type],
                     game_globals.garray[g].wInitTime/600, game_globals.garray[g].wIncrement/10, g+1);

      printed = 1;
    }

    if (CheckPFlag(p, PFLAG_AVAIL) && CheckPFlag(p, PFLAG_OPEN) && (pp->game < 0) &&
       (CheckPFlag(wp, PFLAG_OPEN) || CheckPFlag(bp, PFLAG_OPEN))) {

      if (((player_globals.parray[wp].b_stats.rating <= pp->availmax) &&
         (player_globals.parray[wp].b_stats.rating >= pp->availmin)) ||
         (!pp->availmax)) {
         pprintf (p,"\n%s",notavail_white);
         avail_printed = 1;
      }

      if (((player_globals.parray[bp].b_stats.rating <= pp->availmax) &&
         (player_globals.parray[bp].b_stats.rating >= pp->availmin)) ||
         (!pp->availmax)) {
      pprintf (p,"\n%s",notavail_black);
      avail_printed = 1;
      }

      if (avail_printed) {
        printed = 1;
        avail_printed = 0;
        pprintf (p,"\n");
      } /* was black or white open originally (for simuls) */
    }

    if (printed)
      send_prompt (p);
  }

  free(outStr);
}

int create_new_match(int g, int white_player, int black_player,
                             int wt, int winc, int bt, int binc,
                             int rated, char *category, char *board,
                             int white, int simul)
{

  remove_request(white_player, black_player, -PEND_PARTNER);

  pick_colors(&white_player,&black_player,white,wt,bt,winc,binc);

  decline_withdraw_offers(white_player,-1, PEND_MATCH,DO_WITHDRAW | DO_DECLINE);
  decline_withdraw_offers(black_player,-1, PEND_MATCH,DO_WITHDRAW | DO_DECLINE);
  decline_withdraw_offers(black_player,-1, PEND_SIMUL,DO_WITHDRAW | DO_DECLINE);
  if (simul) {
    decline_withdraw_offers(white_player, -1, PEND_SIMUL, DO_WITHDRAW);
  } else {
    decline_withdraw_offers(white_player, -1, PEND_SIMUL,
                            DO_WITHDRAW | DO_DECLINE);
  }
  game_globals.garray[g].FENstartPos[0] = 0; // [HGM] new shuffle
  if (board_init(g,&game_globals.garray[g].game_state, category, board)) {
    pprintf(white_player, "PROBLEM LOADING BOARD. Game Aborted.\n");
    pprintf(black_player, "PROBLEM LOADING BOARD. Game Aborted.\n");
    d_printf( "CHESSD: PROBLEM LOADING BOARD %s %s. Game Aborted.\n",
            category, board);
    game_remove(g);
    return COM_FAILED;
  }

  if (simul)
    game_globals.garray[g].type = TYPE_UNTIMED;
  else
    game_globals.garray[g].type = game_isblitz(wt, winc, bt, binc, category, board);

  if(category && category[0]) { // [HGM] set variant string from directory for games loaded from file
    if(!strcmp(category, "wild") && board)
	sprintf(game_globals.garray[g].variant, "%s/%s", category, board);
    else
	strcpy(game_globals.garray[g].variant, category);
  }
  else
    strcpy(game_globals.garray[g].variant, bstr[game_globals.garray[g].type]);

  prepare_match(g,wt,bt,winc,binc,white_player,black_player,rated);

  output_match_messages(white_player,black_player,g, "Creating");

  player_globals.parray[white_player].game = g;
  player_globals.parray[white_player].opponent = black_player;
  player_globals.parray[white_player].side = WHITE;
  player_globals.parray[white_player].promote = QUEEN;
  player_globals.parray[black_player].game = g;
  player_globals.parray[black_player].opponent = white_player;
  player_globals.parray[black_player].side = BLACK;
  player_globals.parray[black_player].promote = QUEEN;
  send_boards(g);
  gics_globals.userstat.games++;

  /* obey any 'follow' lists */
  follow_start(white_player,black_player);

  return COM_OK;
}
 
int accept_match(struct pending *pend, int p, int p1)
{
  struct player *pp = &player_globals.parray[p];
  int wt, winc, bt, binc, rated, white;
  char category[50], board[50];
  char tmp[100];
  int bh = 0, partner = 0, pp1 = 0, g1, g2;
 
  unobserveAll(p);              /* stop observing when match starts */
  unobserveAll(p1);

  wt = pend->wtime;
  winc = pend->winc;
  bt = pend->btime;
  binc = pend->binc;
  rated = pend->rated;
  strcpy (category, pend->category);
  strcpy (board, pend->board_type);
  white = (pend->seek_color == -1) ? -1 : 1 - pend->seek_color;

  pprintf(p, "You accept the challenge of %s.\n", player_globals.parray[p1].name);
  pprintf_prompt(p1, "\n%s accepts your challenge.\n", pp->name);

  if(!pend->status)
    delete_pending(pend);

  withdraw_seeks(p);
  withdraw_seeks(p1);

  pend_join_match (p,p1);

  if (game_isblitz(wt, winc, bt, binc, category, board) == TYPE_BUGHOUSE) {
    bh = 1;

    if ((partner = pp->partner) >= 0 &&
        (pp1 = player_globals.parray[p1].partner) >= 0) {
      unobserveAll(partner);         /* stop observing when match starts */
      unobserveAll(pp1);

      pprintf(partner, "\nYour partner accepts the challenge of %s.\n", player_globals.parray[p1].name);
      pprintf(pp1, "\n%s accepts your partner's challenge.\n", pp->name);

      pend_join_match (partner,pp1);
    } else {
      return COM_OK;
    }
  }

  g1 = game_new(); /* create a game structure */
  if (g1 < 0) {
    sprintf(tmp, "There was a problem creating the new match.\n");
    pprintf(p, tmp);
    pprintf_prompt(p1, tmp);
  }

  if (game_read(g1, p1, p) >= 0) {
	  int swap;
	  swap = p;
	  p = p1;
	  p1 = swap;
	  pp = &player_globals.parray[p];
  } else if (game_read(g1, p, p1) < 0) { /* so no adjourned game */ 
    if (create_new_match(g1,p, p1, wt, winc, bt, binc, rated, category, board, white,0) == COM_FAILED)
      return COM_OK;

 /* create first game */

    if (bh) { /* do bughouse creation */

      white = (pp->side == WHITE ? 0 : 1);
      g2 = game_new();
      if ((g2 < 0) || (create_new_match(g2,partner, pp1, wt, winc, bt, binc, rated, category, board,white,0) == COM_FAILED)) {
         sprintf(tmp, "There was a problem creating the new match.\n");
         pprintf_prompt(partner, tmp);
         pprintf_prompt(pp1, tmp);
         sprintf(tmp, "There was a problem creating your partner's match.\n");
         game_remove(g1); /* abort first game */
         return COM_OK;
       }

      game_globals.garray[g1].link = g2; /* link the games */
      game_globals.garray[g2].link = g1;

      sprintf(tmp, "\nYour partner is playing game %d (%s vs. %s).\n",
              g2 + 1, game_globals.garray[g2].white_name, game_globals.garray[g2].black_name);
      pprintf(p, tmp);
      pprintf_prompt(p1, tmp);

      sprintf(tmp, "\nYour partner is playing game %d (%s vs. %s).\n",
              g1 + 1, game_globals.garray[g1].white_name, game_globals.garray[g1].black_name);
      pprintf_prompt(partner, tmp);
      pprintf_prompt(pp1, tmp);
    } 
    return COM_OK;
  }

       /* resume adjourned game */

  game_delete(p, p1); /* remove the game from disk */

  sprintf(tmp, "{Game %d (%s vs. %s) Continuing %s %s match.}\n",
        g1+1, pp->name, player_globals.parray[p1].name,
        rstr[game_globals.garray[g1].rated], bstr[game_globals.garray[g1].type]);
  pprintf(p, tmp);
  pprintf(p1, tmp);
  output_match_messages(p, p1, g1, "Continuing");

  game_globals.garray[g1].white = p;
  game_globals.garray[g1].black = p1;
  game_globals.garray[g1].status = GAME_ACTIVE;
  game_globals.garray[g1].result = END_NOTENDED;
  game_globals.garray[g1].startTime = tenth_secs();
  game_globals.garray[g1].lastMoveTime = game_globals.garray[g1].startTime;
  game_globals.garray[g1].lastDecTime = game_globals.garray[g1].startTime;
  pp->game = g1;
  pp->opponent = p1;
  pp->side = WHITE;
  player_globals.parray[p1].game = g1;
  player_globals.parray[p1].opponent = p;
  player_globals.parray[p1].side = BLACK;

  send_boards(g1);

  /* obey any 'follow' lists */
  follow_start(p,p1);

  return COM_OK;
}

/* board and category are initially empty strings */
int parse_match_string(int p, int* wt,int* bt,int* winc,int* binc,
                                int* white,int* rated,char* category,
			        char* board, char* mstring)
{
  int numba;
  int confused = 0;
  char parsebuf[100];
  int bughouse = 0;

  while (!confused && (sscanf(mstring, " %99s", parsebuf) == 1)) {
    mstring = eatword(eatwhite(mstring));

    if ((category[0] != '\0') && (board[0] == '\0') && !bughouse)
      strcpy(board, parsebuf);
    else if (isdigit(*parsebuf)) {
      if ((numba = atoi(parsebuf)) < 0) {
        pprintf(p, "You can't specify negative time controls.\n");
        return 0;
      } else if (numba > 999) {
        pprintf(p, "You can't specify time or inc above 999.\n");
        return 0;
      } else if (*wt == -1) {
        *wt = numba;
      } else if (*winc == -1) {
        *winc = numba;
      } else if (*bt == -1) {
        *bt = numba;
      } else if (*binc == -1) {
        *binc = numba;
      } else
        confused = 1;

    } else if ((!strcmp("rated", parsebuf)) || (!strcmp("r",parsebuf))) {
      if (*rated == -1)
        *rated = 1;
      else
        confused = 1;
    } else if ((!strcmp("unrated", parsebuf)) || (!strcmp("u",parsebuf))) {
      if (*rated == -1)
        *rated = 0;
      else
        confused = 1;
    } else if (!strcmp("white", parsebuf) || !strcmp ("w", parsebuf)) {
      if (*white == -1)
        *white = 1;
      else
        confused = 1;
    } else if (!strcmp("black", parsebuf) || !strcmp ("b", parsebuf)) {
      if (*white == -1)
        *white = 0;
      else
        confused = 1;

    } else if (category[0] == '\0') {
      if ((parsebuf[0] == 'w') && (isdigit(*(parsebuf + 1)))) {
        strcpy (category, "wild");
        strcpy (board, parsebuf+1);
      } else {
        strcpy(category,parsebuf);
        if (!strncmp("bughouse",category, strlen(category))
            && strlen(category) >= 3 || !strcmp("bh", category)) {
          strcpy(category, "bughouse");
          bughouse = 1;
        }
	// [HGM] allow some shortcuts for variant names
	if(!strcmp("bh", category)) {
	  strcpy(category, "bughouse");
	} else
	if(!strcmp("zh", category)) {
	  strcpy(category, "crazyhouse");
	} else
	if(!strcmp("fr", category)) {
	  strcpy(category, "fischerandom");
	} else
	if(!strcmp("sj", category)) {
	  strcpy(category, "shatranj");
	} else
	if(!strcmp("gc", category)) {
	  strcpy(category, "gothic");
	} else
	if(!strcmp("ca", category)) {
	  strcpy(category, "capablanca");
	} else
	if(!strcmp("cr", category)) {
	  strcpy(category, "caparandom");
	} else
	if(!strcmp("su", category)) {
	  strcpy(category, "super");
	} else
	if(!strcmp("sc", category)) {
	  strcpy(category, "seirawan");
	} else
	if(!strcmp("sg", category)) {
	  strcpy(category, "shogi");
	} else
	if(!strcmp("km", category)) {
	  strcpy(category, "knightmate");
	} else
	if(!strcmp("gr", category)) {
	  strcpy(category, "great");
	} else
	if(!strcmp("sp", category)) {
	  strcpy(category, "spartan");
	} else
	if(!strcmp("xq", category)) {
	  strcpy(category, "xiangqi");
	}
      }
    } else
      confused = 1;
  }

  if (confused) {
    pprintf(p, "Can't interpret %s in match command.\n", parsebuf);
    return 0;
  }
  if(category && (!board || !board[0]))
	strcpy(board, "0"); // [HGM] variants: always provide default board
  return 1;
}

static void print_match_rating_info(int p,int p1,int type,int rated)
{
  if (rated) {
    int win, draw, loss;
    double newsterr;

    rating_sterr_delta(p1, p, type, time(0), RESULT_WIN, &win, &newsterr);
    rating_sterr_delta(p1, p, type, time(0), RESULT_DRAW, &draw, &newsterr);
    rating_sterr_delta(p1, p, type, time(0), RESULT_LOSS, &loss, &newsterr);
    pprintf(p1, "Your %s rating will change:  Win: %s%d,  Draw: %s%d,  Loss: %s%d\n",
            bstr[type],
            (win >= 0) ? "+" : "", win,
            (draw >= 0) ? "+" : "", draw,
            (loss >= 0) ? "+" : "", loss);
    pprintf(p1, "Your new RD will be %5.1f\n", newsterr);

    rating_sterr_delta(p, p1, type, time(0), RESULT_WIN, &win, &newsterr);
    rating_sterr_delta(p, p1, type, time(0), RESULT_DRAW, &draw, &newsterr);
    rating_sterr_delta(p, p1, type, time(0), RESULT_LOSS, &loss, &newsterr);
    pprintf(p, "Your %s rating will change:  Win: %s%d,  Draw: %s%d,  Loss: %s%d\n",
            bstr[type],
            (win >= 0) ? "+" : "", win,
            (draw >= 0) ? "+" : "", draw,
            (loss >= 0) ? "+" : "", loss);
    pprintf(p, "Your new RD will be %5.1f\n", newsterr);
  }
}

static void check_lists_match(int p,int p1)
{
  struct player *pp = &player_globals.parray[p];
  if (in_list(p, L_COMPUTER, pp->name)) {
    pprintf(p1, "--** %s is a ", pp->name);
    pprintf_highlight(p1, "computer");
    pprintf(p1, " **--\n");
  }
  if (in_list(p, L_COMPUTER, player_globals.parray[p1].name)) {
    pprintf(p, "--** %s is a ", player_globals.parray[p1].name);
    pprintf_highlight(p, "computer");
    pprintf(p, " **--\n");
  }
  if (in_list(p, L_ABUSER, pp->name)) {
    pprintf(p1, "--** %s is in the ", pp->name);
    pprintf_highlight(p1, "abuser");
    pprintf(p1, " list **--\n");
  }
  if (in_list(p, L_ABUSER, player_globals.parray[p1].name)) {
    pprintf(p, "--** %s is in the ", player_globals.parray[p1].name);
    pprintf_highlight(p, "abuser");
    pprintf(p, " list **--\n");
  }
}

int com_match(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int adjourned;                /* adjourned game? */
  int g;                        /* more adjourned game junk */
  int p1;
  int bh = 0, partner = 0, pp1 = 0;
  struct pending* pendfrom;
  struct pending* pendto;
  struct pending* pend;
  int wt = -1;                  /* white start time */
  int winc = -1;                /* white increment */
  int bt = -1;                  /* black start time */
  int binc = -1;                /* black increment */
  int rated = -1;               /* 1 = rated, 0 = unrated */
  int white = -1;               /* 1 = want white, 0 = want black */
  char category[100], board[100];
  textlist *clauses = NULL;
  int type = 0;

  category[0] ='\0';
  board[0] ='\0';

  if ((pp->game >= 0) && ((game_globals.garray[pp->game].status == GAME_EXAMINE)
|| (game_globals.garray[pp->game].status == GAME_SETUP))) {

    pprintf(p, "You can't challenge while you are examining a game.\n");
    return COM_OK;
  }

  if (pp->game >= 0) {
    pprintf(p, "You can't challenge while you are playing a game.\n");
    return COM_OK;
  }

  stolower(param[0].val.word);
  p1 = player_find_part_login(param[0].val.word);
  if (p1 < 0) {
    pprintf(p, "No user named \"%s\" is logged in.\n", param[0].val.word);
    return COM_OK;
  }

  if (p1 == p) {                /* Allowing to match yourself to enter
                                   analysis mode */
    ExamineScratch (p, param, 0);
    return COM_OK;
  }

  if (!CheckPFlag(p1, PFLAG_OPEN)) {
    pprintf(p, "Player \"%s\" is not open to match requests.\n", player_globals.parray[p1].name);
    return COM_OK;
  }

  if (player_globals.parray[p1].game >= 0) {
    pprintf(p, "Player \"%s\" is involved in another game.\n", player_globals.parray[p1].name);    return COM_OK;
  }

  if (CheckPFlag(p, PFLAG_TOURNEY) && !CheckPFlag(p1, PFLAG_TOURNEY)) {
    pprintf(p, "You may only match players with their tournament variable set.\n");
    return COM_OK;
  }

  if (!CheckPFlag(p, PFLAG_TOURNEY) && CheckPFlag(p1, PFLAG_TOURNEY)) {
    pprintf(p, "%s is in a tournament, and cannot accept other challenges.\n", player_globals.parray[p1].name);
    return COM_OK;
  }

  if (!CheckPFlag(p, PFLAG_OPEN)) {
    PFlagON(p, PFLAG_OPEN);
    pprintf(p, "Setting you open for matches.\n");
  }


/* look for an adjourned game between p and p1 */
  g = game_new();
  adjourned = (game_read(g, p, p1) >= 0) || (game_read(g, p1, p) >= 0);
  if (adjourned) {
    char *q;
    type = game_globals.garray[g].type;
    wt = game_globals.garray[g].wInitTime / 600;
    bt = game_globals.garray[g].bInitTime / 600;
    winc = game_globals.garray[g].wIncrement / 10;
    binc = game_globals.garray[g].bIncrement / 10;
    rated = game_globals.garray[g].rated;
    strcpy(category, game_globals.garray[g].variant);
    if(q = strchr(category, '/')) {
      *q = 0; strcpy(board, q+1);
    } else strcpy(board, "0");
  }
  game_remove(g);

  pendto = find_pend(p, p1, PEND_MATCH);

  pendfrom = find_pend(p1, p, PEND_MATCH);
 
  if (!adjourned) {
    if (in_list(p1, L_NOPLAY, pp->login)) {
      pprintf(p, "You are on %s's noplay list.\n", player_globals.parray[p1].name);
      return COM_OK;
    }
    if (player_censored(p1, p)) {
      pprintf(p, "Player \"%s\" is censoring you.\n", player_globals.parray[p1].name);
      return COM_OK;
    }
    if (player_censored(p, p1)) {
      pprintf(p, "You are censoring \"%s\".\n", player_globals.parray[p1].name);
      return COM_OK;
    }

    if (param[1].type != TYPE_NULL) {
      if (!parse_match_string(p, &wt,&bt,&winc,&binc,&white,&rated,category,
                                                  board,param[1].val.string))
      return COM_OK; /* couldn't parse */
    }

    if (rated == -1)
      rated = BoolCheckPFlag(p, PFLAG_RATED);
    if (!CheckPFlag(p, PFLAG_REG) || !CheckPFlag(p1, PFLAG_REG))
      rated = 0;

    if (winc == -1)
      winc = (wt == -1) ? pp->d_inc : 0;  /* match 5 == match 5 0 */

    if (wt == -1)
      wt = pp->d_time;

    if (bt == -1)
      bt = wt;

    if (binc == -1)
      binc = winc;

    if (category[0]) {
      if (!board[0] && strcmp(category,"bughouse")) {
        pprintf(p, "You must specify a board and a category.\n");
        return COM_OK;

      } else if (board[0]) { /* not bughouse */
        char fname[MAX_FILENAME_SIZE];

        sprintf(fname, BOARD_DIR "/%s/%s", category, board);
        if (!file_exists(fname)) {
          pprintf(p, "No such category/board: %s/%s\n", category, board);
          return COM_OK;
        }
      }
    }
    type = game_isblitz(wt, winc, bt, binc, category, board);

    if (!strcmp(category, "bughouse")) {
      type = TYPE_BUGHOUSE;
      if (rated && pp->partner >= 0 && player_globals.parray[p1].partner >= 0) {
        if (!CheckPFlag(pp->partner, PFLAG_REG)
              || !CheckPFlag(player_globals.parray[p1].partner, PFLAG_REG))
          rated = 0;
      }
    }
    if (rated && (type == TYPE_NONSTANDARD)) {
      pprintf(p, "Game is non-standard - reverting to unrated\n");
      rated = 0;
    }
    if (rated && (type == TYPE_UNTIMED)) {
      pprintf(p, "Game is untimed - reverting to unrated\n");
      rated = 0;
    }
    if ((pendfrom == NULL) && !CheckPFlag(p1, PFLAG_ROPEN)
        && (rated != BoolCheckPFlag(p1, PFLAG_RATED))) {
      pprintf(p, "%s only wants to play %s games.\n", player_globals.parray[p1].name,
              rstr[!rated]);
      pprintf_highlight(p1, "Ignoring");
      pprintf(p1, " %srated match request from %s.\n",
              (rated ? "" : "un"), pp->name);
      return COM_OK;
    }

    /* Now check formula. */
    if (!adjourned
        && !GameMatchesFormula(p,p1, wt,winc,bt,binc, rated, type, &clauses)) {
      pprintf(p, "Match request does not fit formula for %s:\n",
              player_globals.parray[p1].name);
      pprintf(p, "%s's formula: %s\n", player_globals.parray[p1].name, player_globals.parray[p1].formula);
      ShowClauses (p, p1, clauses);
      ClearTextList(clauses);
      pprintf_highlight(p1, "Ignoring");
      pprintf_prompt(p1, " (formula): %s (%d) %s (%d) %s.\n",
                     pp->name,
                     GetRating(&player_globals.parray[p], type),
                     player_globals.parray[p1].name,
                     GetRating(&player_globals.parray[p1], type),
            game_str(rated, wt * 60, winc, bt * 60, binc, category, board));
      return COM_OK;
    }

    if (type == TYPE_BUGHOUSE) {
      bh = 1;
      partner = pp->partner;
      pp1 = player_globals.parray[p1].partner;

      if (pp < 0) {
        pprintf(p, "You have no partner for bughouse.\n");
        return COM_OK;
      }
      if (pp1 < 0) {
        pprintf(p, "Your opponent has no partner for bughouse.\n");
        return COM_OK;
      }
      if (partner == pp1) { /* should be an impossible case - DAV */
        pprintf(p, "You and your opponent both chose the same partner!\n");
        return COM_OK;
      }
      if (partner == p1 || pp1 == p) {
        pprintf(p, "You and your opponent can't choose each other as partners!\n");
        return COM_OK;
      }
      if (player_globals.parray[partner].partner != p) { /* another impossible case - DAV */
        pprintf(p, "Your partner hasn't chosen you as his partner!\n");
        return COM_OK;
      }
      if (player_globals.parray[pp1].partner != p1) { /* another impossible case - DAV */
        pprintf(p, "Your opponent's partner hasn't chosen your opponent as his partner!\n");
        return COM_OK;
      }
      if (!CheckPFlag(partner, PFLAG_OPEN) || player_globals.parray[partner].game >= 0) {
        pprintf(p, "Your partner isn't open to play right now.\n");
        return COM_OK;
      }
      if (!CheckPFlag(pp1, PFLAG_OPEN) || player_globals.parray[pp1].game >= 0) {
        pprintf(p, "Your opponent's partner isn't open to play right now.\n");
        return COM_OK;
      }

      /* Bypass NOPLAY lists, censored lists, ratedness, privacy, and formula for now */
      /*  Active challenger/ee will determine these. */
    }
    /* Ok match offer will be made */

  }                             /* adjourned games shouldn't have to worry
                                   about that junk? */
				/* keep incase of adjourned bughouse in future*/

  if (pendto != NULL) {
    pprintf(p, "Updating offer already made to \"%s\".\n", player_globals.parray[p1].name);
  }

  if (pendfrom != NULL) {
    if (pendto != NULL) {
      pprintf(p, "Pending list error!.\n");
      d_printf( "CHESSD: This shouldn't happen. You can't have a match pending from and to the same person.\n");
      return COM_OK;
    }

    if (adjourned || ((wt == pendfrom->btime) &&
                      (winc == pendfrom->binc) &&
                      (bt == pendfrom->wtime) &&
                      (binc == pendfrom->winc) &&
                      (rated == pendfrom->rated) &&
                      ((white == -1) || (white + pendfrom->seek_color == 1)) &&
               (!strcmp(category, pendfrom->category)) &&
                 (!strcmp(board, pendfrom->board_type)))) {
      /* Identical match, should accept! */
      accept_match(pendfrom,p, p1);
      return COM_OK;

    } else {
      delete_pending(pendfrom);
    }
  }

  if (pendto == NULL)
    pend = add_pending(p,p1,PEND_MATCH);
  else
    pend = pendto; 

  pend->wtime = wt;
  pend->winc = winc;
  pend->btime = bt;
  pend->binc = binc;
  pend->rated = rated;
  pend->seek_color = white;
  pend->game_type = type;
  pend->category = strdup(category);
  pend->board_type = strdup (board);

  if (pendfrom != NULL) {
    pprintf(p, "Declining offer from %s and offering new match parameters.\n", player_globals.parray[p1].name);
    pprintf(p1, "\n%s declines your match offer a match with these parameters:", pp->name);
  }

  if (pendto != NULL) {
    pprintf(p, "Updating match request to: ");
    pprintf(p1, "\n%s updates the match request.\n", pp->name);
  } else {
    pprintf(p, "Issuing: ");
    pprintf(p1, "\n");
  }

  pprintf(p, "%s (%s) %s", pp->name,
          ratstrii(GetRating(&player_globals.parray[p], type), p),
          colorstr[white + 1]);
  pprintf_highlight(p, "%s", player_globals.parray[p1].name);
  pprintf(p, " (%s) %s%s.\n",
          ratstrii(GetRating(&player_globals.parray[p1], type), p1),
          game_str(rated, wt * 60, winc, bt * 60, binc, category, board),
          adjustr[adjourned]);
  pprintf(p1, "Challenge: ");
  pprintf_highlight(p1, "%s", pp->name);
  pprintf(p1, " (%s) %s",
          ratstrii(GetRating(&player_globals.parray[p], type), p),
          colorstr[white + 1]);
  pprintf(p1, "%s (%s) %s%s.\n", player_globals.parray[p1].name,
          ratstrii(GetRating(&player_globals.parray[p1], type), p1),
          game_str(rated, wt * 60, winc, bt * 60, binc, category, board),
          adjustr[adjourned]);
  Bell (p1);

  if (bh) {

    pprintf(partner, "\nYour bughouse partner issuing %s (%s) %s",
            pp->name, ratstrii(GetRating(&player_globals.parray[p], type), p),
            colorstr[white + 1]);
    pprintf_highlight(partner, "%s", player_globals.parray[p1].name);
    pprintf(partner, " (%s) %s.\n",
            ratstrii(GetRating(&player_globals.parray[p1], type), p1),
            game_str(rated, wt * 60, winc, bt * 60, binc, category, board));
    pprintf(partner, "Your game would be ");
    pprintf_highlight(partner, "%s", player_globals.parray[pp1].name);
    pprintf_prompt(partner, " (%s) %s%s (%s) %s.\n",
          ratstrii(GetRating(&player_globals.parray[pp1], type), pp1),
          colorstr[white + 1], player_globals.parray[partner].name,
          ratstrii(GetRating(&player_globals.parray[partner], type), partner),
          game_str(rated, wt * 60, winc, bt * 60, binc, category, board));
    Bell (partner);

    pprintf(pp1, "\nYour bughouse partner was challenged ");
    pprintf_highlight(pp1, "%s", pp->name);
    pprintf(pp1, " (%s) %s", ratstrii(GetRating(&player_globals.parray[p], type), p),
                             colorstr[white + 1]);
    pprintf(pp1, "%s (%s) %s.\n", player_globals.parray[p1].name,
            ratstrii(GetRating(&player_globals.parray[p1], type), p1),
            game_str(rated, wt * 60, winc, bt * 60, binc, category, board));
    pprintf(pp1, "Your game would be %s (%s) %s", player_globals.parray[pp1].name,
          ratstrii(GetRating(&player_globals.parray[pp1], type), pp1),
          colorstr[white + 1]);
    pprintf_highlight(pp1, "%s", player_globals.parray[partner].name);
    pprintf_prompt(pp1, " (%s) %s.\n",
          ratstrii(GetRating(&player_globals.parray[partner], type), partner),
          game_str(rated, wt * 60, winc, bt * 60, binc, category, board));
    Bell(pp1);
  }

  check_lists_match (p,p1);

  print_match_rating_info (p,p1,type,rated);

  pprintf_prompt(p1, "You can \"accept\" or \"decline\", or propose different parameters.\n");

  return COM_OK;
}


/*
  rmatch is used by mamer to start matches in tournaments
*/
int com_rmatch(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	int p1, p2;

	if (!in_list(p, L_TD, pp->name)) {
		pprintf(p, "Only TD programs are allowed to use this command.\n");
		return COM_OK;
	}

	if ((p1 = player_find_bylogin(param[0].val.word)) < 0 || 
	    !CheckPFlag(p1, PFLAG_REG)) {
		/* can't rmatch this user ... */
		return COM_OK;
	}

	if ((p2 = player_find_bylogin(param[1].val.word)) < 0 || 
	    !CheckPFlag(p2, PFLAG_REG)) {
		/* can't rmatch this user ... */
		return COM_OK;
	}

	return pcommand(p1, "match %s %s", param[1].val.word, param[2].val.string);
}
