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

int GameNumFromParam(int p, int *p1, parameter *param)
{
  if (param->type == TYPE_WORD) {
    *p1 = player_find_part_login(param->val.word);
    if (*p1 < 0) {
      pprintf(p, "No user named \"%s\" is logged in.\n", param->val.word);
      return -1;
    }
    if (player_globals.parray[*p1].game < 0)
      pprintf(p, "%s is not playing a game.\n", player_globals.parray[*p1].name);
    return player_globals.parray[*p1].game;
  } else {			/* Must be an integer */
    *p1 = -1;
    if (param->val.integer <= 0)
      pprintf(p, "%d is not a valid game number.\n", param->val.integer);
    return param->val.integer - 1;
  }
}

static int gamesortfunc(const void *i, const void *j)
{
/* examine mode games moved to top of "games" output */
  return (GetRating(&player_globals.parray[game_globals.garray[*(int *) i].white], game_globals.garray[*(int *) i].type) +
     GetRating(&player_globals.parray[game_globals.garray[*(int *) i].black], game_globals.garray[*(int *) i].type) -
	  (((game_globals.garray[*(int *) i].status == GAME_EXAMINE) || (game_globals.garray[*(int *) i].status == GAME_SETUP)) ? 10000 : 0) -
     GetRating(&player_globals.parray[game_globals.garray[*(int *) j].white], game_globals.garray[*(int *) j].type) -
     GetRating(&player_globals.parray[game_globals.garray[*(int *) j].black], game_globals.garray[*(int *) j].type) +
	  (((game_globals.garray[*(int *) j].status == GAME_EXAMINE) || (game_globals.garray[*(int *) j].status == GAME_SETUP)) ? 10000 : 0));
}


int com_games(int p, param_list param)
{
  int i, j;
  int wp, bp;
  int ws, bs;
  int selected = 0;
  int count = 0;
  int totalcount;
  char *s = NULL;
  int slen = 0;
  int *sortedgames;		/* for qsort */

  totalcount = game_count();
  if (totalcount == 0) {
    pprintf(p, "There are no games in progress.\n");
  } else {
    sortedgames = malloc(totalcount * sizeof(int));	/* for qsort */

    if (param[0].type == TYPE_WORD) {
      s = param[0].val.word;
      slen = strlen(s);
      selected = atoi(s);
      if (selected < 0)
	selected = 0;
    }
    for (i = 0; i < game_globals.g_num; i++) {
      if ((game_globals.garray[i].status != GAME_ACTIVE) && (game_globals.garray[i].status != GAME_EXAMINE) && (game_globals.garray[i].status != GAME_SETUP))
	continue;
      if ((selected) && (selected != i + 1))
	continue;		/* not selected game number */
      wp = game_globals.garray[i].white;
      bp = game_globals.garray[i].black;
      if ((!selected) && s && strncasecmp(s, game_globals.garray[i].white_name, slen) &&
	  strncasecmp(s, game_globals.garray[i].black_name, slen))
	continue;		/* player names did not match */
      sortedgames[count++] = i;
    }
    if (!count)
      pprintf(p, "No matching games were found (of %d in progress).\n", totalcount);
    else {
      qsort(sortedgames, count, sizeof(int), gamesortfunc);
      pprintf(p, "\n");
      for (j = 0; j < count; j++) {
	i = sortedgames[j];
	wp = game_globals.garray[i].white;
	bp = game_globals.garray[i].black;
	board_calc_strength(&game_globals.garray[i].game_state, &ws, &bs);
	if ((game_globals.garray[i].status != GAME_EXAMINE) && (game_globals.garray[i].status != GAME_SETUP)) {
	  pprintf_noformat(p, "%3d %4s %-11.11s %4s %-10.10s [%c%c%c%3d %3d] ",
			   i + 1,
			   ratstrii(GetRating(&player_globals.parray[wp],game_globals.garray[i].type), wp),
			   player_globals.parray[wp].name,
			   ratstrii(GetRating(&player_globals.parray[bp],game_globals.garray[i].type), bp),
			   player_globals.parray[bp].name,
			   (game_globals.garray[i].private) ? 'p' : ' ',
			   *bstr[game_globals.garray[i].type],
			   *rstr[game_globals.garray[i].rated],
			   game_globals.garray[i].wInitTime / 600,
			   game_globals.garray[i].wIncrement / 10);
	  game_update_time(i);
	  pprintf_noformat(p, "%6s -",
		 tenth_str((game_globals.garray[i].wTime > 0 ? game_globals.garray[i].wTime : 0), 0));
	  pprintf_noformat(p, "%6s (%2d-%2d) %c: %2d\n",
		  tenth_str((game_globals.garray[i].bTime > 0 ? game_globals.garray[i].bTime : 0), 0),
			   ws, bs,
			 (game_globals.garray[i].game_state.onMove == WHITE) ? 'W' : 'B',
			   game_globals.garray[i].game_state.moveNum);
	} else {
	  pprintf_noformat(p, "%3d (%s %4d %-11.11s %4d %-10.10s) [%c%c%c%3d %3d] ",
			   i + 1,
                           (game_globals.garray[i].status == GAME_EXAMINE) ? "Exam." : "Setup",
			   game_globals.garray[i].white_rating,
			   game_globals.garray[i].white_name,
			   game_globals.garray[i].black_rating,
			   game_globals.garray[i].black_name,
			   (game_globals.garray[i].private) ? 'p' : ' ',
			   *bstr[game_globals.garray[i].type],
			   *rstr[game_globals.garray[i].rated],
			   game_globals.garray[i].wInitTime / 600,
			   game_globals.garray[i].wIncrement / 10);
	  pprintf_noformat(p, "%c: %2d\n",
			 (game_globals.garray[i].game_state.onMove == WHITE) ? 'W' : 'B',
			   game_globals.garray[i].game_state.moveNum);
	}
      }
      if (count < totalcount)
	pprintf(p, "\n  %d game%s displayed (of %d in progress).\n", count,
		(count == 1) ? "" : "s", totalcount);
      else
	pprintf(p, "\n  %d game%s displayed.\n", totalcount, (totalcount == 1) ? "" : "s");
    }
    free(sortedgames);
  }
  return COM_OK;
}

static int do_observe(int p, int obgame)
{
  struct player *pp = &player_globals.parray[p];
  if ((game_globals.garray[obgame].private) && (pp->adminLevel < ADMIN_ADMIN)) {
    pprintf(p, "Sorry, game %d is a private game.\n", obgame + 1);
    return COM_OK;
  }
  if ((game_globals.garray[obgame].white == p) || (game_globals.garray[obgame].black == p)) {
    if ((game_globals.garray[obgame].status != GAME_EXAMINE) || (game_globals.garray[obgame].status != GAME_SETUP)) {
      pprintf(p, "You cannot observe a game that you are playing.\n");
      return COM_OK;
    }
  }
  if (player_is_observe(p, obgame)) {
    pprintf(p, "Removing game %d from observation list.\n", obgame + 1);
    player_remove_observe(p, obgame);
  } else {
    if (!player_add_observe(p, obgame)) {
      pprintf(p, "You are now observing game %d.\n", obgame + 1);
      send_board_to(obgame, p);
    } else {
      pprintf(p, "You are already observing the maximum number of games.\n");
    }
  }
  return COM_OK;
}

void unobserveAll(int p)
{
	struct player *pp = &player_globals.parray[p];
	int i;
	
	for (i = 0; i < pp->num_observe; i++) {
		pprintf(p, "Removing game %d from observation list.\n", 
			pp->observe_list[i] + 1);
	}
	pp->num_observe = 0;
	return;
}

int com_unobserve(int p, param_list param)
{
  int gNum, p1;

  if (param[0].type == TYPE_NULL) {
    unobserveAll(p);
    return COM_OK;
  }
  gNum = GameNumFromParam(p, &p1, &param[0]);
  if (gNum < 0)
    return COM_OK;
  if (!player_is_observe(p, gNum)) {
    pprintf(p, "You are not observing game %d.\n", gNum);
  } else {
    player_remove_observe(p, gNum);
    pprintf(p, "Removing game %d from observation list.\n", gNum + 1);
  }
  return COM_OK;
}

int com_observe(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int i;
  int p1, obgame;
  
  if (param[0].type == TYPE_NULL)
    return COM_BADPARAMETERS;
  if ((pp->game >=0) &&(game_globals.garray[pp->game].status == GAME_EXAMINE)) {
    pprintf(p, "You are still examining a game.\n");
    return COM_OK;
  }
  if ((pp->game >=0) &&(game_globals.garray[pp->game].status == GAME_SETUP)) {
    pprintf(p, "You are still setting up a position.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    unobserveAll(p);
    return COM_OK;
  }
  obgame = GameNumFromParam(p, &p1, &param[0]);
  if (obgame < 0)
    return COM_OK;

  if ((obgame >= game_globals.g_num) || ((game_globals.garray[obgame].status != GAME_ACTIVE) &&
			    (game_globals.garray[obgame].status != GAME_EXAMINE) &&
			    (game_globals.garray[obgame].status != GAME_SETUP))) {
    pprintf(p, "There is no such game.\n");
    return COM_OK;
  }
  if ((p1 >= 0) && (player_globals.parray[p1].simul_info != NULL) && (player_globals.parray[p1].simul_info->numBoards)) {
    for (i = 0; i < player_globals.parray[p1].simul_info->numBoards; i++)
      if (player_globals.parray[p1].simul_info->boards[i] >= 0)
        do_observe(p, player_globals.parray[p1].simul_info->boards[i]);
    } else
      do_observe(p, obgame);
  return COM_OK;
}

int com_allobservers(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int obgame;
  int p1;
  int start, end;
  int g;
  int first;

  if (param[0].type == TYPE_NULL) {
    obgame = -1;
  } else {
    obgame = GameNumFromParam(p, &p1, &param[0]);
    if (obgame < 0)
      return COM_OK;
  }
  if (obgame == -1) {
    start = 0;
    end = game_globals.g_num;
  } else if ((obgame >= game_globals.g_num) || ((obgame < game_globals.g_num)
				   && ((game_globals.garray[obgame].status != GAME_ACTIVE)
			     && (game_globals.garray[obgame].status != GAME_EXAMINE)
			     && (game_globals.garray[obgame].status != GAME_SETUP)))) {
    pprintf(p, "There is no such game.\n");
    return COM_OK;
  } else {
    start = obgame;
    end = obgame + 1;
  }

  /* list games being played */

  for (g = start; g < end; g++) {
    if ((game_globals.garray[g].status == GAME_ACTIVE) &&
	((pp->adminLevel > 0) || (game_globals.garray[g].private == 0))) {
      for (first = 1, p1 = 0; p1 < player_globals.p_num; p1++) {
	if ((player_globals.parray[p1].status != PLAYER_EMPTY) && (player_is_observe(p1, g))) {
	  if (first) {
	    pprintf(p, "Observing %2d [%s vs. %s]:",
		    g + 1,
		    player_globals.parray[game_globals.garray[g].white].name,
		    player_globals.parray[game_globals.garray[g].black].name);
	    first = 0;
	  }
	  pprintf(p, " %s%s", (player_globals.parray[p1].game >=0) ? "#" : "", player_globals.parray[p1].name);
	}
      }
      if (!first)
	pprintf(p, "\n");
    }
  }

  /* list games being examined last */

  for (g = start; g < end; g++) {
    if (((game_globals.garray[g].status == GAME_EXAMINE) || (game_globals.garray[g].status == GAME_SETUP)) &&
	((pp->adminLevel > 0) || (game_globals.garray[g].private == 0))) {
      for (first = 1, p1 = 0; p1 < player_globals.p_num; p1++) {
	if ((player_globals.parray[p1].status != PLAYER_EMPTY) && (player_is_observe(p1, g) ||
						  (player_globals.parray[p1].game == g))) {
	  if (first) {
	    if (strcmp(game_globals.garray[g].white_name, game_globals.garray[g].black_name)) {
	      pprintf(p, "Examining %2d [%s vs %s]:", g + 1,
		      game_globals.garray[g].white_name, game_globals.garray[g].black_name);
	    } else {
	      pprintf(p, "Examining %2d (scratch):", g + 1);
	    }
	    first = 0;
	  }
	  pprintf(p, " %s%s", (player_globals.parray[p1].game == g) ? "#" : "", player_globals.parray[p1].name);
	}
      }
      if (!first)
	pprintf(p, "\n");
    }
  }
  return COM_OK;
}

int com_unexamine(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int g, p1, flag = 0;

  if ((pp->game <0) || ((game_globals.garray[pp->game].status != GAME_EXAMINE) && (game_globals.garray[pp->game].status != GAME_SETUP))) {
    pprintf(p, "You are not examining any games.\n");
    return COM_OK;
  }
  g = pp->game;
  pp->game = -1;
  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if (player_globals.parray[p1].status != PLAYER_PROMPT)
      continue;
    if ((player_globals.parray[p1].game == g) &&(p != p1)) {
      /* ok - there are other examiners to take over the game */
      flag = 1;
    }
    if ((player_is_observe(p1, g)) || (player_globals.parray[p1].game == g)) {
      pprintf(p1, "%s stopped examining game %d.\n", pp->name, g + 1);
    }
  }
  if (!flag) {
    for (p1 = 0; p1 < player_globals.p_num; p1++) {
      if (player_globals.parray[p1].status != PLAYER_PROMPT)
	continue;
      if (player_is_observe(p1, g)) {
	pprintf(p1, "There are no examiners.\n");
	pcommand(p1, "unobserve %d", g + 1);
      }
    }
    game_remove(g);
  }
  pprintf(p, "You are no longer examining game %d.\n", g + 1);
  announce_avail(p);
  return COM_OK;
}

int com_mexamine(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int g, p1, p2;

  if ((pp->game <0) ||
      ((game_globals.garray[pp->game].status != GAME_EXAMINE) &&
      (game_globals.garray[pp->game].status != GAME_SETUP))) {
    pprintf(p, "You are not examining any games.\n");
    return COM_OK;
  }
  p1 = player_find_part_login(param[0].val.word);
  if (p1 < 0) {
    pprintf(p, "No user named \"%s\" is logged in.\n", param[0].val.word);
    return COM_OK;
  }
  g = pp->game;
  if (!player_is_observe(p1, g)) {
    pprintf(p, "%s must observe the game you are analysing.\n", player_globals.parray[p1].name);
    return COM_OK;
  } else {
    if (player_globals.parray[p1].game >=0) {
      pprintf(p, "%s is already analysing the game.\n", player_globals.parray[p1].name);
      return COM_OK;
    }
    /* if we get here - let's make him examiner of the game */
    unobserveAll(p1);		/* fix for Xboard */
    decline_withdraw_offers(p1, -1, PEND_MATCH,DO_DECLINE | DO_WITHDRAW);
    decline_withdraw_offers(p1, -1, PEND_SIMUL,DO_WITHDRAW);

    player_globals.parray[p1].game = g;	/* yep - it really is that easy :-) */
    pprintf(p1, "You are now examiner of game %d.\n", g + 1);
    send_board_to(g, p1);	/* pos not changed - but fixes Xboard */
    for (p2 = 0; p2 < player_globals.p_num; p2++) {
      if (player_globals.parray[p2].status != PLAYER_PROMPT)
	continue;
      if (p2 == p1)
	continue;
      if ((player_is_observe(p2, g)) || (player_globals.parray[p2].game == g)) {
	pprintf_prompt(p2, "%s is now examiner of game %d.\n", player_globals.parray[p1].name, g + 1);
      }
    }
  }
  if (CheckPFlag(p2, PFLAG_OPEN)) /* was open */
    announce_notavail(p2);
  return COM_OK;
}

int com_moves(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int g;
  int p1;

  if (param[0].type == TYPE_NULL) {
    if (pp->game >=0) {
      g = pp->game;
    } else if (pp->num_observe) {
      for (g = 0; g < pp->num_observe; g++) {
	pprintf_noformat(p, "%s\n", movesToString(pp->observe_list[g], 0));
      }
      return COM_OK;
    } else {
      pprintf(p, "You are neither playing, observing nor examining a game.\n");
      return COM_OK;
    }
  } else {
    g = GameNumFromParam(p, &p1, &param[0]);
    if (g < 0)
      return COM_OK;
  }
  if ((g < 0) || (g >= game_globals.g_num) || ((game_globals.garray[g].status != GAME_ACTIVE) &&
				  (game_globals.garray[g].status != GAME_EXAMINE) &&
				  (game_globals.garray[g].status != GAME_SETUP))) {
    pprintf(p, "There is no such game.\n");
    return COM_OK;
  }
  if ((game_globals.garray[g].white != p) && (game_globals.garray[g].black != p) && (game_globals.garray[g].private) && (pp->adminLevel < ADMIN_ADMIN)) {
    pprintf(p, "Sorry, that is a private game.\n");
    return COM_OK;
  }
  pprintf_noformat(p, "%s\n", movesToString(g, 0));	/* pgn may break interfaces? */
  return COM_OK;
}

int com_mailmoves(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int g;
  int p1;
  char subj[81];

  if (param[1].type == TYPE_NULL) {
    if (!CheckPFlag(p, PFLAG_REG)) {
      pprintf (p,"Unregistered players must specify their e-mail address.\n");
      return COM_OK;
    }
  } else if(!safestring(param[1].val.string)) {
    pprintf (p,"Bad e-mail address.\n");
    return COM_OK;
  }

  if (param[0].type == TYPE_NULL) {
    if (pp->game >=0) {
      g = pp->game;
    } else {
      pprintf(p, "You are neither playing, observing nor examining a game.\n");
      return COM_OK;
    }
  } else {
    g = GameNumFromParam(p, &p1, &param[0]);
    if (g < 0)
      return COM_OK;
  }
  if ((g < 0) || (g >= game_globals.g_num) || ((game_globals.garray[g].status != GAME_ACTIVE) && (game_globals.garray[g].status != GAME_EXAMINE))) {
    pprintf(p, "There is no such game.\n");
    return COM_OK;
  }
  if ((game_globals.garray[g].white != p) && (game_globals.garray[g].black != p) && (game_globals.garray[g].private) && (pp->adminLevel < ADMIN_ADMIN)) {
    pprintf(p, "Sorry, that is a private game.\n");
    return COM_OK;
  }
  sprintf(subj, "FICS game report %s vs %s", game_globals.garray[g].white_name, game_globals.garray[g].black_name);
  if (param[1].type == TYPE_NULL ? mail_string_to_user(p, subj,
                          movesToString(g, CheckPFlag(p, PFLAG_PGN))) :
               mail_string_to_address(param[1].val.string, subj,
                          movesToString(g, CheckPFlag(p, PFLAG_PGN)))) {
    pprintf(p, "Moves NOT mailed, perhaps your address is incorrect.\n");
  } else {
    pprintf(p, "Moves mailed.\n");
  }
  return COM_OK;
}

static int old_mail_moves(int p,int mail, param_list param)
{
  int p1, connected;
  int count;
  FILE *fp;
  char fname[MAX_FILENAME_SIZE];
  char tmp[2048];
  char *ptmp = tmp;

 if (mail && !CheckPFlag(p, PFLAG_REG)) {
    pprintf (p,"Unregistered players cannot use mailoldmoves.\n");
    return COM_OK;
  }

   if (param[0].type == TYPE_WORD) {
    if (!FindPlayer(p, param[0].val.word, &p1, &connected))
      return COM_OK;
  } else {
      p1 = p;
      connected = 1;
  }
 
  sprintf(fname, STATS_DIR "/player_data/%c/%s.%s",
          player_globals.parray[p1].login[0], player_globals.parray[p1].login, STATS_GAMES);
  fp = fopen_s(fname, "r"); /* old moves now looks in history to save mem - DAV */

  if (!fp) {
    pprintf (p,"There is no old game for %s.\n", player_globals.parray[p1].name);
    if (!connected)
      player_remove(p1);
    return COM_OK;
  } 

  while (!feof(fp))
      fgets(tmp, 1024, fp);
  sscanf(ptmp, "%d", &count);
  fclose(fp); /* find the last game played in history */

  pprintf (p,"Last game for %s was history game %d.\n",player_globals.parray[p1].name,count);

  if (mail)
   pcommand (p,"mailstored %s %d",player_globals.parray[p1].name,count);
  else
   pcommand (p,"smoves %s %d",player_globals.parray[p1].name,count);

  if (!connected)
    player_remove(p1); 
  
  return COM_OK;
}

int com_oldmoves(int p, param_list param)
{
  return old_mail_moves(p , 0, param);
}

int com_mailoldmoves(int p, param_list param)
{
  return old_mail_moves(p , 1, param);
}

void ExamineScratch(int p,  param_list param,int setup)
{
  struct player *pp = &player_globals.parray[p];
  char category[100], board[100], parsebuf[100];
  char *val;
  int confused = 0;
  int g;

  category[0] = '\0';
  board[0] = '\0';

  if ((param[0].val.string != pp->name) &&
      (param[1].type == TYPE_WORD)) {
        strcpy(category, param[0].val.string);
        strcpy(board, param[1].val.string);
  } else if (param[1].type != TYPE_NULL) {

      val = param[1].val.string;

      while (!confused && (sscanf(val, " %99s", parsebuf) == 1)) {
        val = eatword(eatwhite(val));
        if ((category[0] != '\0') && (board[0] == '\0'))
          strcpy(board, parsebuf);
        else if (isdigit(*parsebuf)) {
          pprintf(p, "You can't specify time controls.\n");
          return;
        } else if (category[0] == '\0')
          strcpy(category, parsebuf);
        else
          confused = 1;
      }
      if (confused) {
        pprintf(p, "Can't interpret %s in match command.\n", parsebuf);
        return;
      }
  }


  if (category[0] && !board[0]) {
    pprintf(p, "You must specify a board and a category.\n");
    return;
  }

  g = game_new();

  unobserveAll(p);

  decline_withdraw_offers(p, -1, PEND_MATCH,DO_DECLINE | DO_WITHDRAW);
  decline_withdraw_offers(p, -1, PEND_SIMUL,DO_WITHDRAW);

  game_globals.garray[g].wInitTime = game_globals.garray[g].wIncrement = 0;
  game_globals.garray[g].bInitTime = game_globals.garray[g].bIncrement = 0;
  game_globals.garray[g].timeOfStart = tenth_secs();
  game_globals.garray[g].wTime = game_globals.garray[g].bTime = 0;
  game_globals.garray[g].rated = 0;
  game_globals.garray[g].clockStopped = 0;
  game_globals.garray[g].type = TYPE_UNTIMED;
  game_globals.garray[g].white = game_globals.garray[g].black = p;
  game_globals.garray[g].startTime = tenth_secs();
  game_globals.garray[g].lastMoveTime = game_globals.garray[g].startTime;
  game_globals.garray[g].lastDecTime = game_globals.garray[g].startTime;
  game_globals.garray[g].totalHalfMoves = 0;

  pp->side = WHITE;       /* oh well... */
  pp->game = g;


  if (!setup)
    pprintf(p, "Starting a game in examine (scratch) mode.\n");
  else
    pprintf(p, "Starting a game in examine (setup) mode.\n");

  if (category[0]) {
    pprintf(p, "Loading from catagory: %s, board: %s.\n", category, board);
  }

  game_globals.garray[g].FENstartPos[0] = 0; // [HGM] new shuffle game
  if (setup) {
    board_clear(&game_globals.garray[g].game_state);
    game_globals.garray[g].status = GAME_SETUP;
  } else {
    game_globals.garray[g].status = GAME_EXAMINE;
    if (board_init(g,&game_globals.garray[g].game_state, category, board)) {
      pprintf(p, "PROBLEM LOADING BOARD. Examine Aborted.\n");
      d_printf( "CHESSD: PROBLEM LOADING BOARD. Examine Aborted.\n");
      pp->game = -1;
      game_remove(g);
      return;
    }
  }

  game_globals.garray[g].game_state.gameNum = g;
  strcpy(game_globals.garray[g].white_name, pp->name);
  strcpy(game_globals.garray[g].black_name, pp->name);
  game_globals.garray[g].white_rating = game_globals.garray[g].black_rating = pp->s_stats.rating;

  send_boards(g);
  if (CheckPFlag(p, PFLAG_OPEN)) /*was open */
    announce_notavail(p);
}

static int ExamineStored(FILE * fp, int p, char type)
{
  struct player *pp = &player_globals.parray[p];
  int g;
  char category[100], board[100];
  struct game *gg;

  unobserveAll(p);

  decline_withdraw_offers(p, -1, PEND_MATCH,DO_DECLINE | DO_WITHDRAW);
  decline_withdraw_offers(p, -1, PEND_SIMUL,DO_WITHDRAW);

  g = game_new();
  gg = &game_globals.garray[g];
  category[0] = '\0';
  board[0] = '\0';
  game_globals.garray[g].FENstartPos[0] = 0; // [HGM] make new shuffle for now
  if (board_init(g,&gg->game_state, category, board)) {
    pprintf(p, "PROBLEM LOADING BOARD. Examine Aborted.\n");
    d_printf( "CHESSD: PROBLEM LOADING BOARD %s %s. Examine Aborted.\n",
            category, board);
    game_remove(g);
    return -1;
  }

  gg->status = GAME_EXAMINE;

  if (type == 'w' || type == 'n') {
    if (ReadGameAttrs_exam(fp, g) < 0) {
      pprintf(p, "Either this is an old wild/nonstandard game or the gamefile is corrupt.\n");
      game_remove(g);
      return -1;
    }

    // [HGM] OK, we retreived the game info, which includes variant name as "category/board"
    // So now we set up the board again, this time for the proper variant (and proper shuffle)
    sscanf(gg->variant, "%s/%s", category, board);
    if(category[0] && !board[0]) strcpy(board, "0");
    if (board_init(g,&gg->game_state, category, board)) {
      pprintf(p, "PROBLEM LOADING BOARD. Examine Aborted.\n");
      d_printf( "CHESSD: PROBLEM LOADING BOARD %s %s. Examine Aborted.\n",
              category, board);
      game_remove(g);
      return -1;
    }
  } else if (ReadGameAttrs(fp, g) < 0) {
    pprintf(p, "Gamefile is corrupt; please notify an admin.\n");
    game_remove(g);
    return -1;
  }

  gg->totalHalfMoves = gg->numHalfMoves;
  gg->numHalfMoves = 0;
  gg->revertHalfMove = 0;
  gg->white = p;
  gg->black = p;
  gg->game_state.gameNum = g;

  gg->startTime = tenth_secs();
  gg->lastMoveTime = gg->startTime;
  gg->lastDecTime = gg->startTime;

  pp->side = WHITE;	/* oh well... */
  pp->game = g;
  send_boards(g);
  if (CheckPFlag(p, PFLAG_OPEN)) /* was open */
    announce_notavail(p);

  return g;
}

static void ExamineAdjourned(int p, int p1, int p2)
{
	FILE *fp;
	char *p1Login, *p2Login;
	int g;
	
	p1Login = player_globals.parray[p1].login;
	p2Login = player_globals.parray[p2].login;

	fp = fopen_p("%s/%c/%s-%s", "r", ADJOURNED_DIR, *p1Login, p1Login, p2Login);
	if (!fp) fp = fopen_p("%s/%c/%s-%s", "r", ADJOURNED_DIR, *p2Login, p1Login, p2Login);
	if (!fp) fp = fopen_p("%s/%c/%s-%s", "r", ADJOURNED_DIR, *p2Login, p2Login, p1Login);
	if (!fp) fp = fopen_p("%s/%c/%s-%s", "r", ADJOURNED_DIR, *p1Login, p2Login, p1Login);
	if (!fp) {
		pprintf(p, "No stored game between \"%s\" and \"%s\".\n",
			player_globals.parray[p1].name, player_globals.parray[p2].name);
		return;
	}
	/* Assume old wild games are of type blitz - adjudicators should be
	   careful */
	g = ExamineStored(fp, p,'n');
	fclose(fp);
	
	if (g >= 0) {
		if (game_globals.garray[g].white_name[0] == '\0')
			strcpy(game_globals.garray[g].white_name, p1Login);
		if (game_globals.garray[g].black_name[0] == '\0')
			strcpy(game_globals.garray[g].black_name, p2Login);
	}
}

/* type is now returned because prior to V1.7.1 loading a wild game for 
  examining was impossible since the initial position was not saved.
  Only types blitz,standard,lightning and untimed may be loaded for
  examining if the gamefile version is less than 4 */ 

static char *FindHistory(int p, int p1, int game,char* type)
{
  FILE *fpHist;
  static char fileName[MAX_FILENAME_SIZE];
  int index, last = -1;
  long when;
  char typestr[4];

  sprintf(fileName, STATS_DIR "/player_data/%c/%s.%s",
	  player_globals.parray[p1].login[0], player_globals.parray[p1].login, STATS_GAMES);
 again:
  fpHist = fopen_s(fileName, "r");
  if (fpHist == NULL) {
    pprintf(p, "No games in history for %s.\n", player_globals.parray[p1].name);
    return(NULL);
  }
  do {
    fscanf(fpHist, "%d %*c %*d %*c %*d %*s %s %*d %*d %*d %*d %*s %*s %ld",
	   &index, typestr, &when);
    last = index; // [HGM] remember most recent game
  } while (!feof(fpHist) && index != game);

  if(game < 0) { // [HGM] requested game relative to end
    game += last + 1; // calculate absolute game number
    if(game < 0) game += 100; // wrap
    if(game >= 0) { // try again with valid absolute number
      fclose(fpHist);
      goto again;
    }
  }

  if (feof(fpHist)) {
    pprintf(p, "There is no history game %d for %s.\n", game, player_globals.parray[p1].name);
    fclose(fpHist);
    return(NULL);
  }
  fclose(fpHist);

  if (typestr[0] != 'p')
    *type = typestr[0];
  else
    *type = typestr[1];

  sprintf(fileName, "%s/%ld/%ld", HISTORY_DIR, when % 100, when);
  return(fileName);
}

/* I want to know how game ended/ECO code */

static char *FindHistory2(int p, int p1,int game, char* Eco,char* End)
{
  FILE *fpHist;
  static char fileName[MAX_FILENAME_SIZE];
  int index;
  long when;
  
  sprintf(fileName, STATS_DIR "/player_data/%c/%s.%s",
          player_globals.parray[p1].login[0], player_globals.parray[p1].login, STATS_GAMES);
  fpHist = fopen_s(fileName, "r");
  if (fpHist == NULL) {
    pprintf(p, "No games in history for %s.\n", player_globals.parray[p1].name);
    return(NULL);
  }
  do {
    fscanf(fpHist, "%d %*c %*d %*c %*d %*s %*s %*d %*d %*d %*d %s %s %ld",
	   &index, Eco, End, &when);
  } while (!feof(fpHist) && index != game);

  if (feof(fpHist)) {
    pprintf(p, "There is no history game %d for %s.\n", game, player_globals.parray[p1].name);
    fclose(fpHist);
    return(NULL);
  }
  fclose(fpHist);

  sprintf(fileName, "%s/%ld/%ld", HISTORY_DIR, when % 100, when);
  return(fileName);
}

int com_wname(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];

  int g = pp->game;

  if ((g < 0) ||
     ((g >= 0) &&
        !((game_globals.garray[g].status != GAME_EXAMINE) || (game_globals.garray[g].status != GAME_SETUP)))) {
    pprintf (p, "You are not examining or setting up a game.\n");
    return COM_OK;
  }

  if (param[0].type == TYPE_NULL)
    strcpy (game_globals.garray[g].white_name,pp->name);
  else {

    if (strlen (param[0].val.word) > MAX_LOGIN_NAME - 1) {
      pprintf (p,"The maximum length of a name is %d characters.\n",MAX_LOGIN_NAME - 1);
      return COM_OK;
    } else
      strcpy (game_globals.garray[g].white_name,param[0].val.word);
  }

  send_boards(g);
  return COM_OK;
}

int com_bname(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];

  int g = pp->game;

  if ((g < 0) ||
     ((g >= 0) &&
        !((game_globals.garray[g].status != GAME_EXAMINE) || (game_globals.garray[g].status != GAME_SETUP)))) {
    pprintf (p, "You are not examining or setting up a game.\n");
    return COM_OK;
  }

  if (param[0].type == TYPE_NULL)
    strcpy (game_globals.garray[g].black_name,pp->name);
  else {

    if (strlen (param[0].val.word) > MAX_LOGIN_NAME - 1) {
      pprintf (p,"The maximum length of a name is %d characters.\n",MAX_LOGIN_NAME - 1);
      return COM_OK;
    } else
      strcpy (game_globals.garray[g].black_name,param[0].val.word);
  }

  send_boards(g);
  return COM_OK;
}

static void ExamineHistory(int p, int p1, int game)
{
	char *fileName;
	char type;

	fileName = FindHistory(p, p1, game,&type);
	if (fileName) {
		FILE *fpGame = fopen_p("%s", "r", fileName);
		if (fpGame == NULL) {
			pprintf(p, "History game %d not available for %s.\n", 
				game, player_globals.parray[p1].name);
		} else {
			ExamineStored(fpGame, p,type);
			fclose(fpGame);
		}
	}
}

static void ExamineJournal(int p,int p1,char slot)
{
	struct player *pp = &player_globals.parray[p];
	char* name_from = player_globals.parray[p1].login;
	char type;
	FILE *fpGame;

	if (CheckPFlag(p1, PFLAG_JPRIVATE) && (p != p1)
	    && (pp->adminLevel < ADMIN_ADMIN)) {
		pprintf (p,"Sorry, this journal is private.\n");
		return;
	}

	if ((slot - 'a' + 1) > MAX_JOURNAL && 
	    !check_admin(p1, ADMIN_ADMIN) && 
	    !titled_player(p,player_globals.parray[p1].login)) {
		pprintf(p,"%s's maximum journal entry is %c\n",
			player_globals.parray[p1].name,
			toupper((char)(MAX_JOURNAL + 'A' - 1)));
		return;
	}

	fpGame = fopen_p("%s/%c/%s.%c", "r", JOURNAL_DIR, name_from[0],name_from,slot);
	if (fpGame == NULL) {
		pprintf(p, "Journal entry %c is not available for %s.\n", toupper (slot),
			player_globals.parray[p1].name);
	} else {
		char *fname;
		asprintf(&fname, STATS_DIR "/player_data/%c/%s.journal",
			 name_from[0],name_from);
		slot = toupper(slot);

		if ((type = get_journalgame_type (p,fname,slot)) == '\0') {
			pprintf(p, "Journal entry %c is not available for %s or is corrupt.\n",
				slot, player_globals.parray[p1].name);
		} else {
			ExamineStored(fpGame, p,type);
		}

		free(fname);
		fclose (fpGame);
	}
}

int com_examine(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1, p2 = p, p1conn, p2conn = 1;
  char* wincstring;
  char fname[MAX_FILENAME_SIZE];

  if (pp->game >=0) {
     if ((game_globals.garray[pp->game].status == GAME_EXAMINE) || 
        (game_globals.garray[pp->game].status == GAME_SETUP))
       pprintf(p, "You are already examining a game.\n");
     else
       pprintf(p, "You are playing a game.\n");
  } else if (param[0].type == TYPE_NULL) {
    ExamineScratch(p, param, 0);
  } else if (param[0].type != TYPE_NULL) {
    if ((param[1].type == TYPE_NULL) && (!strcmp(param[0].val.word,"setup"))) {
        ExamineScratch(p, param, 1);
        return COM_OK;
    } else if (param[1].type == TYPE_WORD) {
      sprintf(fname, "%s/%s/%s", BOARD_DIR, param[0].val.word, param[1].val.word);
      if (file_exists(fname)) {
        ExamineScratch(p, param, 0);
        return COM_OK;
      }
    }
    if (!FindPlayer(p, param[0].val.word, &p1, &p1conn))
      return COM_OK;

    if (param[1].type == TYPE_INT)
      ExamineHistory(p, p1, param[1].val.integer); 
    else {
      if (param[1].type == TYPE_WORD) {

        /* Lets check the journal */
        wincstring = param[1].val.word;
        if ((strlen(wincstring) == 1) && (isalpha(wincstring[0]))) {
          ExamineJournal(p,p1,wincstring[0]);
          if (!p1conn)
            player_remove(p1);
          return COM_OK;
        } else {
          if (!FindPlayer(p, param[1].val.word, &p2, &p2conn)) {
            if (!p1conn)
              player_remove(p1);
            return COM_OK;
          }
        }
      } 
      ExamineAdjourned(p, p1, p2);
      if (!p2conn)
       player_remove(p2);
    }
    if (!p1conn)
     player_remove(p1);
  }
  return COM_OK;
}

int com_stored(int p, param_list param)
{
  int count = 0;
  DIR *dirp;
  struct dirent *dp;

  int p1, connected;
  char dname[MAX_FILENAME_SIZE];

  if (param[0].type == TYPE_WORD) {
    if (!FindPlayer(p, param[0].val.word, &p1, &connected))
      return COM_OK;
  } else {
      p1 = p;
      connected = 1;
  }

  sprintf(dname, "%s/%c", ADJOURNED_DIR, player_globals.parray[p1].login[0]);
  dirp = opendir(dname);
  if (!dirp) {
    pprintf(p, "Player %s has no games stored.\n", player_globals.parray[p1].name);
    if (!connected)
      player_remove(p1);
    return COM_OK;
  }
  pprintf(p, "Stored games for %s:\n", player_globals.parray[p1].name);
  for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
    if (file_has_pname(dp->d_name, player_globals.parray[p1].login)) {
      pprintf(p, "   %s vs. %s\n", file_wplayer(dp->d_name), file_bplayer(dp->d_name));
      count++;
    }
  }

  closedir(dirp);
  pprintf (p,"%d stored game%sfound.\n",count,(count == 1 ? " " : "s "));
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

static void stored_mail_moves(int p, int mail, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int wp, wconnected, bp, bconnected, gotit = 0;
  int g = -1;
  char type; /* dummy */
  char* wincstring = NULL;
  char* name_from = NULL;
  char* fileName = NULL;
  char fileName2[MAX_FILENAME_SIZE];
  FILE* fpGame;
  
  if (param[2].type == TYPE_NULL) {
    if (!CheckPFlag(p, PFLAG_REG)) {
      pprintf (p,"Unregistered players must specify their e-mail address.\n");
      return COM_OK;
    }
  } else if(!safestring(param[2].val.string)) {
    pprintf (p,"Bad e-mail address.\n");
    return COM_OK;
  }

  if (!FindPlayer(p, param[0].val.word, &wp, &wconnected))
    return;

  if (param[1].type == TYPE_WORD && sscanf(param[1].val.word, "%d", &(param[1].val.integer)) == 1 && param[1].val.integer < 0)
    param[1].type = TYPE_INT; /* [HGM] allow negative number */

  if (param[1].type == TYPE_INT) { /* look for a game from history */
    fileName = FindHistory(p, wp, param[1].val.integer,&type);
    if (fileName != NULL) {
      fpGame = fopen_s(fileName, "r");
      if (fpGame == NULL) {
        pprintf(p, "History game %d not available for %s.\n", param[1].val.integer, player_globals.parray[wp].name);
      } else {
        g = game_new();
        if (ReadGameAttrs(fpGame, g) < 0)
          pprintf(p, "Gamefile is corrupt; please notify an admin.\n");
        else
          gotit = 1;
        fclose(fpGame);
      }
    }
  } else { /* Let's test for journal */
    name_from = param[0].val.word;
    wincstring = param[1].val.word;
    if ((strlen(wincstring) == 1) && (isalpha(wincstring[0]))) {
      if (CheckPFlag(wp, PFLAG_JPRIVATE)
          && (pp->adminLevel < ADMIN_ADMIN) && (p != wp)) {
        pprintf (p,"Sorry, the journal from which you are trying to fetch is private.\n");
      } else {
        if (((wincstring[0] - 'a' + 1) > MAX_JOURNAL) && (player_globals.parray[wp].adminLevel < ADMIN_ADMIN) && (!titled_player(p,player_globals.parray[wp].login))) {
          pprintf (p,"%s's maximum journal entry is %c\n",player_globals.parray[wp].name,toupper((char)(MAX_JOURNAL + 'A' - 1)));
        } else {
          sprintf(fileName2, "%s/%c/%s.%c", JOURNAL_DIR, name_from[0],name_from,wincstring[0]);
          fpGame = fopen_s(fileName2, "r");
          if (fpGame == NULL) {
            pprintf(p, "Journal entry %c is not available for %s.\n", toupper(wincstring[0]),
            player_globals.parray[wp].name);
          } else {
            g = game_new();
            if (ReadGameAttrs(fpGame, g) < 0)
              pprintf(p, "Journal entry is corrupt; please notify an admin.\n");
            else
              gotit = 1;
            fclose(fpGame);
          }
        }
      }
    } else {
  
       /* look for a stored game between the players */

      if (FindPlayer(p, param[1].val.word, &bp, &bconnected)) {
        g = game_new();
        if (game_read(g, wp, bp) >= 0) {  /* look for a game white-black, */
          gotit = 1;
        } else if (game_read(g, bp, wp) >= 0) {   /* or black-white */
          gotit = 1;
        } else {
          pprintf(p, "There is no stored game %s vs. %s\n", player_globals.parray[wp].name, player_globals.parray[bp].name);
        }
        if (!bconnected)
          player_remove(bp);
      }
    }
  }
  if (gotit) {
    if (strcasecmp(pp->name, game_globals.garray[g].white_name) && strcasecmp(player_globals.parray[p]
.name, game_globals.garray[g].black_name) && game_globals.garray[g].private && (pp->adminLevel < ADMIN_ADMIN)) {
      pprintf(p, "Sorry, that is a private game.\n");
    } else {
      if (mail == 1) { /*Do mailstored */
        char subj[81];
        if (param[1].type == TYPE_INT)
          sprintf(subj, "FICS history game: %s %d", player_globals.parray[wp].name, param[1].val.integer);
        else
          if ((strlen(wincstring) == 1) && (isalpha(wincstring[0]))) {
            sprintf(subj, "FICS journal game %s vs %s", game_globals.garray[g].white_name, game_globals.garray[g].black_name); 
          } else {
            sprintf(subj, "FICS adjourned game %s vs %s", game_globals.garray[g].white_name, game_globals.garray[g].black_name);
          }
        if (param[2].type == TYPE_NULL ? mail_string_to_user(p, subj,
                                movesToString(g, CheckPFlag(p, PFLAG_PGN))) :
                        mail_string_to_address(param[2].val.string, subj,
                                movesToString(g, CheckPFlag(p, PFLAG_PGN))))
	  pprintf(p, "Moves NOT mailed, perhaps your address is incorrect.\n");
        else
	  pprintf(p, "Moves mailed.\n");
      } else {
        pprintf_noformat(p, "%s\n", movesToString(g, 0));
      } /* Do smoves */
    }
  }
  if (!wconnected)
    player_remove(wp);
  if (g != -1)
    game_remove(g);
}

/* Tidied up a bit but still messy */

int com_mailstored(int p, param_list param)
{
  stored_mail_moves(p, 1, param);
  return COM_OK;
}

int com_smoves(int p, param_list param)
{
  stored_mail_moves(p, 0, param);
  return COM_OK;
}

int com_sposition(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int wp, wconnected, bp, bconnected, confused = 0;
  int g;

  if (!FindPlayer(p, param[0].val.word, &wp, &wconnected))
    return (COM_OK);
  if (!FindPlayer(p, param[1].val.word, &bp, &bconnected)) {
    if (!wconnected)
      player_remove(wp);
    return (COM_OK);
  }

  g = game_new();
  if (game_read(g, wp, bp) < 0) {	/* if no game white-black, */
    if (game_read(g, bp, wp) < 0) {	/* look for black-white */
      confused = 1;
      pprintf(p, "There is no stored game %s vs. %s\n", player_globals.parray[wp].name, player_globals.parray[bp].name);
    } else {
      int tmp;
      tmp = wp;
      wp = bp;
      bp = tmp;
      tmp = wconnected;
      wconnected = bconnected;
      bconnected = tmp;
    }
  }
  if (!confused) {
    if ((wp != p) && (bp != p) && (game_globals.garray[g].private) && (pp->adminLevel < ADMIN_ADMIN)) {
      pprintf(p, "Sorry, that is a private game.\n");
    } else {
      game_globals.garray[g].white = wp;
      game_globals.garray[g].black = bp;
      game_globals.garray[g].startTime = tenth_secs();
      game_globals.garray[g].lastMoveTime = game_globals.garray[g].startTime;
      game_globals.garray[g].lastDecTime = game_globals.garray[g].startTime;
      pprintf(p, "Position of stored game %s vs. %s\n", player_globals.parray[wp].name, player_globals.parray[bp].name);
      send_board_to(g, p);
    }
  }
  game_remove(g);
  if (!wconnected)
    player_remove(wp);
  if (!bconnected)
    player_remove(bp);
  return COM_OK;
}

int com_forward(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int nHalfMoves = 1;
  int g, i;
  int p1;
  unsigned now;

  if (!((pp->game >=0) && ((game_globals.garray[pp->game].status == GAME_EXAMINE) || (game_globals.garray[pp->game].status == GAME_SETUP)))) {
    pprintf(p, "You are not examining any games.\n");
    return COM_OK;
  }
  if (game_globals.garray[pp->game].status == GAME_SETUP) {
    pprintf (p,"You can't move forward yet, the position is still being set up.\n");
    return COM_OK;
  }
  g = pp->game;
  if (!strcmp(game_globals.garray[g].white_name, game_globals.garray[g].black_name)) {
    pprintf(p, "You cannot go forward; no moves are stored.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_INT) {
    nHalfMoves = param[0].val.integer;
  }
  if (game_globals.garray[g].numHalfMoves > game_globals.garray[g].revertHalfMove) {
    pprintf(p, "Game %u: No more moves.\n", g);
    return COM_OK;
  }
  if (game_globals.garray[g].numHalfMoves < game_globals.garray[g].totalHalfMoves) {
    for (p1 = 0; p1 < player_globals.p_num; p1++) {
      if (player_globals.parray[p1].status != PLAYER_PROMPT)
	continue;
      if (player_is_observe(p1, g) || player_globals.parray[p1].game == g) {
	pprintf(p1, "Game %u: %s goes forward %d move%s.\n",
		g,
		pp->name, nHalfMoves, (nHalfMoves == 1) ? "" : "s");
      }
    }
  }
  for (i = 0; i < nHalfMoves; i++) {
    if (game_globals.garray[g].numHalfMoves < game_globals.garray[g].totalHalfMoves) {
      execute_move(&game_globals.garray[g].game_state, &game_globals.garray[g].moveList[game_globals.garray[g].numHalfMoves], 1);
      if (game_globals.garray[g].numHalfMoves + 1 > game_globals.garray[g].examMoveListSize) {
	game_globals.garray[g].examMoveListSize += 20;	/* Allocate 20 moves at a
						   time */
	game_globals.garray[g].examMoveList = (struct move_t *) realloc(game_globals.garray[g].examMoveList, sizeof(struct move_t) * game_globals.garray[g].examMoveListSize);
      }
      game_globals.garray[g].examMoveList[game_globals.garray[g].numHalfMoves] = game_globals.garray[g].moveList[game_globals.garray[g].numHalfMoves];
      game_globals.garray[g].revertHalfMove++;
      game_globals.garray[g].numHalfMoves++;
    } else {
      for (p1 = 0; p1 < player_globals.p_num; p1++) {
	if (player_globals.parray[p1].status != PLAYER_PROMPT)
	  continue;
	if (player_is_observe(p1, g) || player_globals.parray[p1].game == g) {
	  pprintf(p1, "Game %u: End of game.\n", g);
	}
      }
      break;
    }
  }
  /* roll back time */
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

  if (game_globals.garray[g].revertHalfMove == game_globals.garray[g].totalHalfMoves) {
	  for (p1 = 0; p1 < player_globals.p_num; p1++) {
		  if (player_globals.parray[p1].status != PLAYER_PROMPT) continue;
		  if (player_is_observe(p1, g) || player_globals.parray[p1].game == g) {
			  pprintf(p1, "Game %u: %s %s\n", g, EndString(g,0), EndSym(g));
		  }
	  }
  }

  return COM_OK;
}

int com_backward(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int nHalfMoves = 1;
  int g, i;
  int p1;
  unsigned now;

  if (!((pp->game >=0) && ((game_globals.garray[pp->game].status == GAME_EXAMINE) || (game_globals.garray[pp->game].status == GAME_SETUP)))) {
    pprintf(p, "You are not examining any games.\n");
    return COM_OK;
  }
  if (game_globals.garray[pp->game].status == GAME_SETUP) {
    pprintf (p,"You can't move backward yet, the postion is still being set up.\n");
    return COM_OK;
  }

  g = pp->game;
  if (param[0].type == TYPE_INT) {
    nHalfMoves = param[0].val.integer;
  }
  if (game_globals.garray[g].numHalfMoves != 0) {
    for (p1 = 0; p1 < player_globals.p_num; p1++) {
      if (player_globals.parray[p1].status != PLAYER_PROMPT)
	continue;
      if (player_is_observe(p1, g) || player_globals.parray[p1].game == g) {
	pprintf(p1, "Game %u: %s backs up %d move%s.\n",
		g,
		pp->name, nHalfMoves, (nHalfMoves == 1) ? "" : "s");
      }
    }
  }
  for (i = 0; i < nHalfMoves; i++) {
    if (backup_move(g, REL_EXAMINE) != MOVE_OK) {
      for (p1 = 0; p1 < player_globals.p_num; p1++) {
	if (player_globals.parray[p1].status != PLAYER_PROMPT)
	  continue;
	if (player_is_observe(p1, g) || player_globals.parray[p1].game == g) {
	  pprintf(p1, "Game %u: Beginning of game.\n", g);
	}
      }

      break;
    }
  }
  if (game_globals.garray[g].numHalfMoves < game_globals.garray[g].revertHalfMove) {
    game_globals.garray[g].revertHalfMove = game_globals.garray[g].numHalfMoves;
  }
  /* roll back time */
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

int com_revert(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int nHalfMoves = 1;
  int g, i;
  int p1;
  unsigned now;

  if (!((pp->game >=0) && ((game_globals.garray[pp->game].status == GAME_EXAMINE)|| (game_globals.garray[pp->game].status == GAME_SETUP)))) {
    pprintf(p, "You are not examining any games.\n");
    return COM_OK;
  }
  if (game_globals.garray[pp->game].status == GAME_SETUP) {
    pprintf (p,"You can't move revert yet, the position is still being set up.\n");
    return COM_OK;
  }
  g = pp->game;
  nHalfMoves = game_globals.garray[g].numHalfMoves - game_globals.garray[g].revertHalfMove;
  if (nHalfMoves == 0) {
    pprintf(p, "Game %u: Already at mainline.\n", g);
    return COM_OK;
  }
  if (nHalfMoves < 0) {		/* eek - should NEVER happen! */
    d_printf( "OUCH! in com_revert: nHalfMoves < 0\n");
    return COM_OK;
  }
  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if (player_globals.parray[p1].status != PLAYER_PROMPT)
      continue;
    if (player_is_observe(p1, g) || player_globals.parray[p1].game == g) {
      pprintf(p1, "Game %u: %s reverts to mainline.\n", 
	      g, pp->name);
    }
  }
  for (i = 0; i < nHalfMoves; i++) {
    backup_move(g, REL_EXAMINE);/* should never return error */
  }
  /* roll back time */
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

int com_history(int p, param_list param)
{
  int p1, connected;
  char fname[MAX_FILENAME_SIZE];

  if (param[0].type == TYPE_WORD) {
    if (!FindPlayer(p, param[0].val.word, &p1, &connected))
      return COM_OK;
  } else {
      p1 = p;
      connected = 1;
  }

  sprintf(fname, STATS_DIR "/player_data/%c/%s.%s", player_globals.parray[p1].login[0],
          player_globals.parray[p1].login, STATS_GAMES);
  pgames(p, p1, fname);
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

int com_journal(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1, connected;
  char fname[MAX_FILENAME_SIZE];

    if (param[0].type == TYPE_WORD) {
    if (!FindPlayer(p, param[0].val.word, &p1, &connected))
      return COM_OK;
  } else {
      p1 = p;
      connected = 1;
  }

  if (!CheckPFlag(p1, PFLAG_REG)) {
    pprintf (p,"Only registered players may keep a journal.\n");
    if (!connected)
      player_remove(p1);
    return COM_OK;
  }
  if (CheckPFlag(p1, PFLAG_JPRIVATE) && (p != p1)
      && (pp->adminLevel < ADMIN_ADMIN)) {
    pprintf (p,"Sorry, this journal is private.\n");
    if (!connected)
      player_remove(p1);
    return COM_OK;
  }
  sprintf(fname, STATS_DIR "/player_data/%c/%s.%s", player_globals.parray[p1].login[0],
	  player_globals.parray[p1].login, STATS_JOURNAL);
  pjournal(p, p1, fname);
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/* Remove a journal item */

int com_jkill(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  FILE* Journal;
  char* kill = param[0].val.word;
  char fname[MAX_FILENAME_SIZE];
  char fname_new[MAX_FILENAME_SIZE];
  int empty;

  if (!CheckPFlag(p, PFLAG_REG)) {
     pprintf (p,"Only registered players may keep a journal.\n");
     return COM_OK;
  }

  if ((strlen(kill) != 1) || (!(isalpha(kill[0])))) {
    pprintf (p,"Journal entries are referenced by single letters.\n");
    return COM_OK;
  }

  if (((kill[0] - 'a' + 1) > MAX_JOURNAL) && (pp->adminLevel < ADMIN_ADMIN)
  && (!titled_player(p,pp->login))) {
    pprintf (p,"Your maximum journal entry is %c\n",toupper ((char)(MAX_JOURNAL
+ 'A' - 1)));
    return COM_OK;
  }

  sprintf(fname, STATS_DIR "/player_data/%c/%s.journal",
      pp->login[0],pp->login);
  sprintf (fname_new,"%s.w",fname);

  Journal = fopen_s(fname, "r");
  if (Journal == NULL) {
     pprintf(p, "You don't have a journal.\n");
     return COM_OK;
     }

  kill[0] = toupper(kill[0]);
 
  if (removejournalitem(p, kill[0], Journal,fname_new,&empty)) {
    fclose (Journal);
    rename (fname_new,fname);
    if (empty)
       unlink (fname);
    sprintf(fname, "%s/%c/%s.%c",
         JOURNAL_DIR, pp->login[0],pp->login,tolower(kill[0]));
    unlink(fname);
    pprintf (p,"Journal entry %c deleted.\n",kill[0]);
  } else { 
    pprintf (p,"You have no journal entry %c.\n",kill[0]);
    fclose (Journal);
    unlink (fname_new);
  }
  return COM_OK;
}

static void jsave_journalentry(int p,char save_spot,int p1,char from_spot,char* to_file)
{
  struct player *pp = &player_globals.parray[p];
  FILE *Game;
  char fname[MAX_FILENAME_SIZE], fname2[MAX_FILENAME_SIZE];
  char* name_from = player_globals.parray[p1].login;
  char* name_to = pp->login;
  struct journal* j;

  sprintf(fname, "%s/%c/%s.%c", JOURNAL_DIR, name_from[0],name_from,from_spot);
  Game = fopen_s(fname, "r");
  if (Game == NULL) {
     pprintf(p, "Journal entry %c not available for %s.\n", toupper(from_spot), player_globals.parray[p1].name);
     return;
     }
  fclose (Game);
  
  sprintf(fname2, "%s/%c/%s.%c", JOURNAL_DIR, name_to[0],name_to,save_spot);

  if (file_copy(fname, fname2) != 0) {
    pprintf (p,"Copy in jsave_journalentry failed!\n");
    pprintf (p,"Please report this to an admin.\n");
    d_printf("CHESSD: Copy failed in jsave_journalentry\n");
    return;
  }

 sprintf(fname, STATS_DIR "/player_data/%c/%s.%s", name_from[0],
          name_from, STATS_JOURNAL);

 j = (struct journal*) malloc (sizeof(struct journal));
 if (!journal_get_info(p,tolower(from_spot),j,fname)) {
   free (j);
   return;
 }

 j->slot = toupper(save_spot);
 addjournalitem(p, j, to_file);

 pprintf(p,"Journal entry %s %c saved in slot %c in journal.\n",player_globals.parray[p1].name, toupper(from_spot), toupper(save_spot));
 free (j);
}

static void jsave_history(int p,char save_spot,int p1,int from,char* to_file)
{
	struct player *pp = &player_globals.parray[p];
	char Eco[100];
	char End[100];
	char jfname[MAX_FILENAME_SIZE];
	char* HistoryFname = FindHistory2(p, p1, from, Eco, End);
	/* End symbol Mat Res, etc is the only thing we can't find out */
	char command[MAX_FILENAME_SIZE*2+3];
	char* name_to = pp->login;
	FILE *Game;
	int g;
	struct journal* j;
	
	if (HistoryFname == NULL) {
		return;
	}
	Game = fopen_s(HistoryFname, "r");
	if (Game == NULL) {
		pprintf(p, "History game %d not available for %s.\n", 
			from, player_globals.parray[p1].name);
		return;
	} 

	sprintf(jfname, "%s/%c/%s.%c", JOURNAL_DIR, name_to[0],name_to,save_spot);
	unlink(jfname); /* necessary if cp is hard aliased to cp -i */
	sprintf(command, "cp %s %s",HistoryFname,jfname);
	if (file_copy(HistoryFname, jfname) != 0) {
		pprintf (p,"Copy in jsave_history failed!\n");
		pprintf (p,"Please report this to an admin.\n");
		d_printf("CHESSD: Copy failed in jsave_journalentry\n");
		return;
	}
	g = game_new(); /* Open a dummy game */
	
	if (ReadGameAttrs(Game, g) < 0) {
		pprintf (p,"Gamefile is corrupt. Please tell an admin.\n");
		game_remove(g);
		fclose (Game);
		return;
	}
	fclose (Game);
	
	j = (struct journal*) malloc (sizeof(struct journal));
	
	if (game_globals.garray[g].private) {
		j->type[0] = 'p';
	} else {
		j->type[0] = ' ';
	}
	if (game_globals.garray[g].type == TYPE_BLITZ) {
		j->type[1] = 'b';
	} else if (game_globals.garray[g].type == TYPE_WILD) {
		j->type[1] = 'w';
	} else if (game_globals.garray[g].type == TYPE_STAND) {
		j->type[1] = 's';
	} else if (game_globals.garray[g].type == TYPE_LIGHT) {
		j->type[1] = 'l';
	} else if (game_globals.garray[g].type == TYPE_BUGHOUSE) {
		j->type[1] = 'B';
	} else {
		if (game_globals.garray[g].type == TYPE_NONSTANDARD)
			j->type[1] = 'n';
		else
			j->type[1] = 'u';
	}
	if (game_globals.garray[g].rated) {
		j->type[2] = 'r';
	} else {
		j->type[2] = 'u';
	}
	j->type[3] = '\0';
	
	j->slot = toupper(save_spot);
	strcpy (j->WhiteName, game_globals.garray[g].white_name);
	j->WhiteRating = game_globals.garray[g].white_rating;
	strcpy (j->BlackName, game_globals.garray[g].black_name);
	j->BlackRating = game_globals.garray[g].black_rating;
	j->t = game_globals.garray[g].wInitTime;
	j->i = game_globals.garray[g].wIncrement;
	strcpy (j->eco, Eco);
	strcpy (j->ending, End);
	strcpy (j->result, EndSym(g));
	
	addjournalitem(p, j, to_file);
	game_remove(g);
	pprintf(p,"Game %s %d saved in slot %c in journal.\n",
		player_globals.parray[p1].name, from, toupper(save_spot));
	free(j);
}

int com_jsave(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1, p1conn;
  char* to = param[0].val.word;
  char* from;
  char fname[MAX_FILENAME_SIZE];

  if (!CheckPFlag(p, PFLAG_REG)) {
     pprintf (p,"Only registered players may keep a journal.\n");
     return COM_OK;
  }

  if ((strlen(to) != 1) || (!(isalpha(to[0])))) {
    pprintf (p,"Journal entries are referenced by single letters.\n");
    return COM_OK;
  }

  if (((to[0] - 'a' + 1) > MAX_JOURNAL) && (pp->adminLevel < ADMIN_ADMIN) && (!titled_player(p,pp->login))) {
    pprintf (p,"Your maximum journal entry is %c\n",toupper ((char)(MAX_JOURNAL + 'A' - 1)));
    return COM_OK;
  }

  if (!FindPlayer(p, param[1].val.word, &p1, &p1conn))
    return COM_OK;

  if (param[2].type == TYPE_INT) {

  /* grab from a history */
    sprintf (fname,STATS_DIR"/player_data/%c/%s.%s",pp->login[0],pp->login, STATS_JOURNAL);
    jsave_history(p, to[0], p1, param[2].val.integer,fname); 

  } else {

  from = param[2].val.word;

  if ((strlen(from) != 1) || (!(isalpha(from[0])))) {
    pprintf (p,"Journal entries are referenced by single letters.\n");
    if (!p1conn)
      player_remove(p1);
    return COM_OK;
    }

  if (CheckPFlag(p1, PFLAG_JPRIVATE)
      && (pp->adminLevel < ADMIN_ADMIN) && (p != p1)) {
    pprintf (p,"Sorry, the journal from which you are trying to fetch is private.\n");

    if (!p1conn)
       player_remove(p1);
    return COM_OK;
    }

  if (((to[0] - 'a' + 1) > MAX_JOURNAL) && (player_globals.parray[p1].adminLevel < ADMIN_ADMIN) && (!titled_player(p,player_globals.parray[p1].login))) {
    pprintf (p,"%s's maximum journal entry is %c\n",player_globals.parray[p1].name,toupper((char)(MAX_JOURNAL + 'A' - 1)));
    if (!p1conn)
       player_remove(p1);
    return COM_OK;
  }
  if (( p == p1) && (to[0] == from [0])) {
    pprintf (p,"Source and destination entries are the same.\n");
    return COM_OK;
  }
  
  /* grab from a journal */

  sprintf(fname, STATS_DIR "/player_data/%c/%s.%s", pp->login[0],
          pp->login, STATS_JOURNAL);
  jsave_journalentry(p,to[0],p1, from[0], fname);

  }
  if (!p1conn)
     player_remove(p1);
  return COM_OK;
}

int com_refresh(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int g, p1;

  if (param[0].type == TYPE_NULL) {
    if (pp->game >= 0) {
      send_board_to(pp->game, p);
    } else {                    /* Do observing in here */
      if (pp->num_observe) {
        for (g = 0; g < pp->num_observe; g++) {
          send_board_to(pp->observe_list[g], p);
        }
      } else {
        pprintf(p, "You are neither playing, observing nor examining a game.\n");
        return COM_OK;
      }
    }
  } else {
    g = GameNumFromParam (p, &p1, &param[0]);
    if (g < 0)
      return COM_OK;
    if ((g >= game_globals.g_num) || ((game_globals.garray[g].status != GAME_ACTIVE)
                        && ((game_globals.garray[g].status != GAME_EXAMINE)
		        || (game_globals.garray[g].status != GAME_SETUP)))) {
      pprintf(p, "No such game.\n");
    } else {

      int link = game_globals.garray[g].link;

      if ((game_globals.garray[g].private && pp->adminLevel==ADMIN_USER) &&
        (game_globals.garray[g].white != p) && (game_globals.garray[g].white != p1)) {
        if (link != -1) {
          if ((game_globals.garray[link].white != p) && (game_globals.garray[link].black != p)) {
            pprintf (p, "Sorry, game %d is a private game.\n", g+1);
            return COM_OK;
          }
        }
      }

      if (game_globals.garray[g].private)
        pprintf(p, "Refreshing static game %d\n", g+1);
      send_board_to(g, p);
    }
  }
  return COM_OK;
}

int com_prefresh(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int retval, part = pp->partner;

  if (part < 0) {
    pprintf(p, "You do not have a partner.\n");
    return COM_OK;
  }
  retval = pcommand (p, "refresh %s", player_globals.parray[part].name);
  if (retval == COM_OK)
    return COM_OK_NOPROMPT;
  else
    return retval;
}
