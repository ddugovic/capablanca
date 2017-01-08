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

static int player_zero(int p);

static int get_empty_slot(void)
{
	int i;

	for (i = 0; i < player_globals.p_num; i++) {
		if (player_globals.parray[i].status == PLAYER_EMPTY) 
			return i;
	}

	if (i < player_globals.parray_size) {
		player_globals.p_num++;
		ZERO_STRUCT(player_globals.parray[i]);
		player_globals.parray[i].status = PLAYER_EMPTY;
		return i;
	}

	if (player_globals.parray_size >= config_get_int("MAX_PLAYERS", DEFAULT_MAX_PLAYER)) {
		d_printf("Too many players connected!\n");
		return -1;
	}

	player_globals.parray_size += 100;
	player_globals.parray = (struct player *)realloc(player_globals.parray,
							 player_globals.parray_size * sizeof(struct player));
	
	player_globals.p_num++;
	ZERO_STRUCT(player_globals.parray[i]);
	player_globals.parray[i].status = PLAYER_EMPTY;
	return i;
}

int player_new(void)
{
	int new = get_empty_slot();
  
	player_zero(new);
	return new;
}

static void ResetStats(struct statistics *ss)
{
	ss->num = 0;
	ss->win = 0;
	ss->los = 0;
	ss->dra = 0;
	ss->rating = config_get_int("DEFAULT_RATING", DEFAULT_RATING);
	ss->sterr = config_get_int("DEFAULT_RD", DEFAULT_RD);
	ss->ltime = 0;
	ss->best = 0;
	ss->whenbest = 0;
}

static int player_zero(int p)
{
	struct player *pp = &player_globals.parray[p];

	ZERO_STRUCTP(pp);

	pp->prompt = config_get("DEFAULT_PROMPT");
	pp->partner = -1;
	pp->socket = -1;
	pp->status = PLAYER_NEW;
	
	ResetStats(&pp->s_stats);
	ResetStats(&pp->b_stats);
	ResetStats(&pp->l_stats);
	ResetStats(&pp->w_stats);
	ResetStats(&pp->bug_stats);
	
	pp->d_time = config_get_int("DEFAULT_TIME", DEFAULT_TIME);
	pp->d_inc = config_get_int("DEFAULT_INCREMENT", DEFAULT_INCREMENT);
	pp->d_height = 24;
	pp->d_width = 79;
	pp->language = LANG_DEFAULT;
	pp->promote = QUEEN;
	pp->game = -1;
	pp->last_channel = -1;
	pp->ftell = -1;
	pp->Flags = PFLAG_DEFAULT;
	pp->opponent = -1;
	return 0;
}

void player_free(struct player *pp)
{
	int i;
	
	FREE(pp->login);
	FREE(pp->more_text);
	FREE(pp->name);
	FREE(pp->passwd);
	FREE(pp->fullName);
	FREE(pp->emailAddress);
	FREE(pp->prompt);
	FREE(pp->formula);
	FREE(pp->busy);
	FREE(pp->last_tell);
	FREE(pp->last_opponent);
	FREE(pp->simul_info);
	for (i = 0; i < pp->num_plan; i++)
		FREE(pp->planLines[i]);
	for (i = 0; i < pp->num_formula; i++)
		FREE(pp->formulaLines[i]);
	list_free(pp->lists);
	for (i = 0; i < pp->numAlias; i++) {
		FREE(pp->alias_list[i].comm_name);
		FREE(pp->alias_list[i].alias);
	}
}

int player_clear(int p)
{
	player_free(&player_globals.parray[p]);
	player_zero(p);
	return 0;
}

int player_remove(int p)
{
	struct player *pp = &player_globals.parray[p];
	int i;

	decline_withdraw_offers(p, -1, -1, DO_DECLINE | DO_WITHDRAW);
	if (pp->simul_info != NULL)
		/* Player disconnected in middle of simul */
		for (i = 0; pp->simul_info != NULL
			     && i < pp->simul_info->numBoards; i++)
			if (pp->simul_info->boards[i] >= 0) 
				game_disconnect(pp->simul_info->boards[i], p);
	
	if ((pp->game >=0) && (pIsPlaying(p))) {  
		/* Player disconnected in the middle of a game! */
		pprintf(pp->opponent, "Your opponent has lost contact or quit.");
		game_disconnect(pp->game, p);
	}
	for (i = 0; i < player_globals.p_num; i++) {
		if (player_globals.parray[i].status == PLAYER_EMPTY)
			continue;
		if (player_globals.parray[i].partner == p) {
			pprintf_prompt (i, "Your partner has disconnected.\n");
			decline_withdraw_offers(i, -1, PEND_BUGHOUSE, DO_DECLINE | DO_WITHDRAW);
			player_globals.parray[i].partner = -1;
		}
	}
	player_clear(p);
	pp->status = PLAYER_EMPTY;

	return 0;
}

/*
  write a player file using the new generic marshalling format
*/
static int WritePlayerFile_v100(int p)
{
	struct player pp = player_globals.parray[p];
	const char *s;
	FILE *fp;

	/* zero any elements we don't want to save */
	memset(&pp, 0, offsetof(struct player, not_saved_marker));

	/* marshall it into a string */
	s = marshall_player(&pp);
	if (!s) {
		d_printf("Unable to marshall player structure for %s\n", 
			 pp.name);
		return -1;
	}

	fp = fopen_p(PLAYER_DIR "/%c/%s", "w",
		     player_globals.parray[p].login[0], 
		     player_globals.parray[p].login);
	if (!fp) {
		d_printf("CHESSD: Problem opening player file '%s' for write\n", 
			 player_globals.parray[p].login);
		free(s);
		return -1;
	}

	/* and save it */
	fprintf(fp, "v 100\n%s\n", s);
	free(s);
	fclose(fp);
	return 0;
}

/*
  read a player file using the new generic and extensible format 
*/
static int ReadPlayerFile_v100(FILE *fp, int p)
{
	char *s, *s2;       
	struct player *pp = &player_globals.parray[p];

	s = fd_load(fileno(fp), NULL);
	if (!s) {
		d_printf("Error reading player file for '%s'\n", pp->login);
		return -1;
	}

	/* skip first line */
	s2 = strchr(s, '\n');

	/* the marshaller doesn't save zero elements, but some elements don't
	   default to zero. Make sure they get the right value */
	memset(&pp->not_saved_marker, 0, 
	       sizeof(struct player) - offsetof(struct player, not_saved_marker));

	if (!s2 || unmarshall_player(pp, s2) != 0) {
		d_printf("Error unmarshalling player data for '%s'!\n", pp->login);
		free(s);
		return -1;
	}
	free(s);

	lists_validate(p);
	
	return 0;
}

int player_read(int p, char *name)
{
	struct player *pp = &player_globals.parray[p];
	char fname[MAX_FILENAME_SIZE];
	char line[MAX_LINE_SIZE];
	FILE *fp;
	int version = 0;
	int ret;

	pp->login = stolower(strdup(name));

	sprintf(fname, PLAYER_DIR "/%c/%s", pp->login[0], pp->login);
	fp = fopen_s(fname, "r");
  
	if (pp->name)
		free(pp->name);

	if (!fp) { /* unregistered player */
		pp->name = stolower(strdup(name));
		PFlagOFF(p, PFLAG_REG);
		return -1;
	}

	PFlagON(p, PFLAG_REG); /* lets load the file */
	if (fgets(line, MAX_LINE_SIZE, fp) != NULL && line[0] == 'v') { /* ok so which version file? */
		sscanf(line, "%*c %d", &version);
	}

	if (version == 9 || version == 100) {
		/* its the new style */
		ret = ReadPlayerFile_v100(fp, p);
	} else {
		/* fall back to the old style */
		ret = player_read_old(p, fp, name, fname, version);
		player_save(p); /* update to the new generic format */
	}

	fclose(fp);

	return ret;
}

int player_save(int p)
{
	struct player *pp = &player_globals.parray[p];

	if (!CheckPFlag(p, PFLAG_REG)) { /* Player not registered */
		return -1;
	}
	if (pp->name == NULL) { /* fixes a bug if name is null */
		pprintf(p, "WARNING: Your player file could not be updated, due to corrupt data.\n");
		return -1;
	}
	if (strcasecmp(pp->login, pp->name)) {
		pprintf(p, "WARNING: Your player file could not be updated, due to corrupt data.\n");
		return -1;
	}

	return WritePlayerFile_v100(p);
}

int player_find(int fd)
{
	int i;
	
	for (i = 0; i < player_globals.p_num; i++) {
		if (player_globals.parray[i].status == PLAYER_EMPTY)
			continue;
		if (player_globals.parray[i].socket == fd)
			return i;
	}
	return -1;
}

/* incorrectly named */
int player_find_bylogin(const char *name)
{
	int i;
	
	for (i = 0; i < player_globals.p_num; i++) {
		if ((player_globals.parray[i].status == PLAYER_EMPTY) ||
		    (player_globals.parray[i].status == PLAYER_LOGIN) ||
		    (player_globals.parray[i].status == PLAYER_PASSWORD))
			continue;
		if (!player_globals.parray[i].login)
			continue;
		if (!strcasecmp(player_globals.parray[i].name, name))
			return i;
	}
	return -1;
}

/* Now incorrectly named */
int player_find_part_login(const char *name)
{
	int i;
	int found = -1;
	
	i = player_find_bylogin(name);
	if (i >= 0)
		return i;
	for (i = 0; i < player_globals.p_num; i++) {
		if ((player_globals.parray[i].status == PLAYER_EMPTY) ||
		    (player_globals.parray[i].status == PLAYER_LOGIN) ||
		    (player_globals.parray[i].status == PLAYER_PASSWORD))
			continue;
		if (!player_globals.parray[i].name)
			continue;
		if (!strncasecmp(player_globals.parray[i].name, name, strlen(name))) {
			found = i;
			break;
		}
	}

	if(found < 0)
		return -1;
	else
		return found;
}

int player_censored(int p, int p1)
{
	if (in_list(p, L_CENSOR, player_globals.parray[p1].login))
		return 1;
	else
		return 0;
}

/* is p1 on p's notify list? */
int player_notified(int p, int p1)
{
	struct player *pp = &player_globals.parray[p];
	if (!CheckPFlag(p1, PFLAG_REG))
		return 0;

	/* possible bug: p has just arrived! */
	if (!pp->name)
		return 0;
	
	return (in_list(p, L_NOTIFY, player_globals.parray[p1].login));
}

void player_notify_departure(int p)
/* Notify those with notifiedby set on a departure */
{
  struct player *pp = &player_globals.parray[p];
  int p1;

  if (!CheckPFlag(p, PFLAG_REG))
    return;
  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if (CheckPFlag(p1, PFLAG_NOTIFYBY) && !player_notified(p1, p)
        && player_notified(p, p1) && (player_globals.parray[p1].status == PLAYER_PROMPT)) {
      Bell (p1);
      pprintf(p1, "\nNotification: ");
      pprintf_highlight(p1, "%s", pp->name);
      pprintf_prompt(p1, " has departed and isn't on your notify list.\n");
    }
  }
}

int player_notify_present(int p)
/* output Your arrival was notified by..... */
/* also notify those with notifiedby set if necessary */
{
  struct player *pp = &player_globals.parray[p];
  int p1;
  int count = 0;

  if (!CheckPFlag(p, PFLAG_REG))
    return count;
  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if ((player_notified(p, p1)) && (player_globals.parray[p1].status == PLAYER_PROMPT)) {
      if (!count) {
	pprintf(p, "Present company includes:");
      }
      count++;
      pprintf(p, " %s", player_globals.parray[p1].name);
      if (CheckPFlag(p1, PFLAG_NOTIFYBY) && (!player_notified(p1, p))
                         && (player_globals.parray[p1].status == PLAYER_PROMPT)) {
	Bell (p1);
	pprintf(p1, "\nNotification: ");
	pprintf_highlight(p1, "%s", pp->name);
	pprintf_prompt(p1, " has arrived and isn't on your notify list.\n");
      }
    }
  }
  if (count)
    pprintf(p, ".\n");
  return count;
}

int player_notify(int p, char *note1, char *note2)
/* notify those interested that p has arrived/departed */
{
  struct player *pp = &player_globals.parray[p];
  int p1, count = 0;

  if (!CheckPFlag(p, PFLAG_REG))
    return count;
  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if ((player_notified(p1, p)) && (player_globals.parray[p1].status == PLAYER_PROMPT)) {
      Bell (p1);
      pprintf(p1, "\nNotification: ");
      pprintf_highlight(p1, "%s", pp->name);
      pprintf_prompt(p1, " has %s.\n", note1);
      if (!count) {
	pprintf(p, "Your %s was noted by:", note2);
      }
      count++;
      pprintf(p, " %s", player_globals.parray[p1].name);
    }
  }
  if (count)
    pprintf(p, ".\n");
  return count;
}

int showstored(int p)
{
  struct player *pp = &player_globals.parray[p];
  DIR *dirp;
  struct dirent *dp;

  int c=0,p1;
  char dname[MAX_FILENAME_SIZE];
  const int N = config_get_int("MAX_STORED", DEFAULT_MAX_STORED);
  multicol *m = multicol_start(N); /* Limit to 50, should be enough */
  
  sprintf(dname, ADJOURNED_DIR "/%c", pp->login[0]);
  dirp = opendir(dname);
  if (!dirp) {
    multicol_end(m);
    return COM_OK;
  }
  for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
    if (file_has_pname(dp->d_name, pp->login)) {
      if (strcmp(file_wplayer(dp->d_name),pp->login) != 0) {
      	p1=player_find_bylogin(file_wplayer(dp->d_name));
      } else {
      	p1=player_find_bylogin(file_bplayer(dp->d_name));
      }
      if (p1>=0) {
      	if (c<N)
      		multicol_store(m,player_globals.parray[p1].name);
      	pprintf(p1,"\nNotification: ");
      	pprintf_highlight(p1,"%s",pp->name);
      	pprintf_prompt(p1,", who has an adjourned game with you, has arrived.\n");
      	c++;
      } 
    }
  }
  closedir(dirp);
  if (c == 1) {
        pprintf(p, "1 player, who has an adjourned game with you, is online:\007");
  } else if (c > 1) {
  	pprintf(p, "\n%d players, who have an adjourned game with you, are online:\007",c);
  }
  if (c != 0)
  	multicol_pprint(m,p,pp->d_width,2);
  multicol_end(m);

  return COM_OK;
}

int player_count(int CountAdmins)
{
  int count;
  int i;

  for (count = 0, i = 0; i < player_globals.p_num; i++) {
    if ((player_globals.parray[i].status == PLAYER_PROMPT) &&
        (CountAdmins || !in_list(i, L_ADMIN, player_globals.parray[i].name)))
      count++;
  }
  if (count > command_globals.player_high)
    command_globals.player_high = count;

  return count;
}

int player_idle(int p)
{
	struct player *pp = &player_globals.parray[p];
	if (pp->status != PLAYER_PROMPT)
		return time(0) - pp->logon_time;
	else
		return time(0) - pp->last_command_time;
}

int player_ontime(int p)
{
	struct player *pp = &player_globals.parray[p];
	return time(0) - pp->logon_time;
}

static void write_p_inout(int inout, int p, char *file, int maxlines)
{
	struct player *pp = &player_globals.parray[p];
	FILE *fp;

	fp = fopen_s(file, "a");
	if (!fp)
		return;
	fprintf(fp, "%d %s %d %d %s\n", inout, pp->name, (int) time(0),
		BoolCheckPFlag(p, PFLAG_REG),
		dotQuad(pp->thisHost));
	fclose(fp);
	if (maxlines)
		truncate_file(file, maxlines);
}

void player_write_login(int p)
{
	struct player *pp = &player_globals.parray[p];
	char fname[MAX_FILENAME_SIZE];
	
	if (CheckPFlag(p, PFLAG_REG)) {
		sprintf(fname, STATS_DIR "/player_data/%c/%s.%s", pp->login[0], pp->login, STATS_LOGONS);
		write_p_inout(P_LOGIN, p, fname, 8);
	}
	sprintf(fname, STATS_DIR "/%s", STATS_LOGONS);
	write_p_inout(P_LOGIN, p, fname, 30);
	/* added complete login/logout log to "logons.log" file */
	sprintf(fname, STATS_DIR "/%s", "logons.log");
	write_p_inout(P_LOGIN, p, fname, 0);
}

void player_write_logout(int p)
{
	struct player *pp = &player_globals.parray[p];
	char fname[MAX_FILENAME_SIZE];
	
	if (CheckPFlag(p, PFLAG_REG)) {
		sprintf(fname, STATS_DIR "/player_data/%c/%s.%s", pp->login[0], pp->login, STATS_LOGONS);
		write_p_inout(P_LOGOUT, p, fname, 8);
	}
	sprintf(fname, STATS_DIR "/%s", STATS_LOGONS);
	write_p_inout(P_LOGOUT, p, fname, 30);
	/* added complete login/logout log to "logons.log" file */
	sprintf(fname, STATS_DIR "/%s", "logons.log");
	write_p_inout(P_LOGOUT, p, fname, 0);
}

int player_lastdisconnect(int p)
{
  struct player *pp = &player_globals.parray[p];
  char fname[MAX_FILENAME_SIZE];
  FILE *fp;
  int inout, thetime, registered;
  int last = 0;
  char ipstr[20];
  char loginName[MAX_LOGIN_NAME];

  sprintf(fname, STATS_DIR "/player_data/%c/%s.%s", pp->login[0], pp->login, STATS_LOGONS);
  fp = fopen_s(fname, "r");
  if (!fp)
    return 0;
  while (!feof(fp)) {
    if (fscanf(fp, "%d %s %d %d %s\n", &inout, loginName, &thetime, &registered, ipstr) != 5) {
      d_printf( "CHESSD: Error in login info format. %s\n", fname);
      fclose(fp);
      return 0;
    }
    if (inout == P_LOGOUT)
      last = thetime;
  }
  fclose(fp);
  return last;
}

int player_is_observe(int p, int g)
{
  struct player *pp = &player_globals.parray[p];
  int i;

  for (i = 0; i < pp->num_observe; i++) {
    if (pp->observe_list[i] == g)
      break;
  }
  if (i == pp->num_observe)
    return 0;
  else
    return 1;
}

int player_add_observe(int p, int g)
{
  struct player *pp = &player_globals.parray[p];
  if (pp->num_observe == MAX_OBSERVE)
    return -1;
  pp->observe_list[pp->num_observe] = g;
  pp->num_observe++;
  return 0;
}

int player_remove_observe(int p, int g)
{
  struct player *pp = &player_globals.parray[p];
  int i;

  for (i = 0; i < pp->num_observe; i++) {
    if (pp->observe_list[i] == g)
      break;
  }
  if (i == pp->num_observe)
    return -1;			/* Not found! */
  for (; i < pp->num_observe - 1; i++) {
    pp->observe_list[i] = pp->observe_list[i + 1];
  }
  pp->num_observe--;
  return 0;
}

int player_game_ended(int g)
{
	int p;
	
	for (p = 0; p < player_globals.p_num; p++) {
		struct player *pp = &player_globals.parray[p];
		if (pp->status == PLAYER_EMPTY)
			continue;
		player_remove_observe(p, g);
	}
	remove_request(game_globals.garray[g].white, game_globals.garray[g].black, -1);
	remove_request(game_globals.garray[g].black, game_globals.garray[g].white, -1);
	player_save(game_globals.garray[g].white);	/* Hawk: Added to save finger-info after each
							   game */
	player_save(game_globals.garray[g].black);
	return 0;
}

int player_goto_board(int p, int board_num)
{
  struct player *pp = &player_globals.parray[p];
  int start, count = 0, on, g;

  if (pp->simul_info == NULL)
    return -1;

  if (board_num < 0 || board_num >= pp->simul_info->numBoards)
    return -1;
  if (pp->simul_info->boards[board_num] < 0)
    return -1;
  pp->simul_info->onBoard = board_num;
  pp->game = pp->simul_info->boards[board_num];
  pp->opponent = game_globals.garray[pp->game].black;
  if (pp->simul_info->numBoards == 1)
    return 0;
  send_board_to(pp->game, p);
  start = pp->game;
  on = pp->simul_info->onBoard;
  do {
    g = pp->simul_info->boards[on];
    if (g >= 0) {
      if (count == 0) {
	Bell (game_globals.garray[g].black);
	pprintf(game_globals.garray[g].black, "\n");
	pprintf_highlight(game_globals.garray[g].black, "%s", pp->name);
	pprintf_prompt(game_globals.garray[g].black, " is at your board!\n");
      } else if (count == 1) {
	Bell (game_globals.garray[g].black);
	pprintf(game_globals.garray[g].black, "\n");
	pprintf_highlight(game_globals.garray[g].black, "%s", pp->name);
	pprintf_prompt(game_globals.garray[g].black, " will be at your board NEXT!\n");
      } else {
	pprintf(game_globals.garray[g].black, "\n");
	pprintf_highlight(game_globals.garray[g].black, "%s", pp->name);
	pprintf_prompt(game_globals.garray[g].black, " is %d boards away.\n", count);
      }
      count++;
    }
    on++;
    if (on >= pp->simul_info->numBoards)
      on = 0;
  } while (start != pp->simul_info->boards[on]);
  return 0;
}

int player_goto_next_board(int p)
{
  struct player *pp = &player_globals.parray[p];
  int on;
  int start;
  int g;

  if (pp->simul_info == NULL)
    return -1;

  on = pp->simul_info->onBoard;
  start = on;
  g = -1;
  do {
    on++;
    if (on >= pp->simul_info->numBoards)
      on = 0;
    g = pp->simul_info->boards[on];
    if (g >= 0)
      break;
  } while (start != on);
  if (g == -1) {
    pprintf(p, "\nMajor Problem! Can't find your next board.\n");
    return -1;
  }
  return player_goto_board(p, on);
}

int player_goto_prev_board(int p)
{
  struct player *pp = &player_globals.parray[p];
  int on;
  int start;
  int g;

  if (pp->simul_info == NULL)
    return -1;

  on = pp->simul_info->onBoard;
  start = on;
  g = -1;
  do {
    --on;
    if (on < 0)
      on = (pp->simul_info->numBoards) - 1;
    g = pp->simul_info->boards[on];
    if (g >= 0)
      break;
  } while (start != on);
  if (g == -1) {
    pprintf(p, "\nMajor Problem! Can't find your previous board.\n");
    return -1;
  }
  return player_goto_board(p, on);
}

int player_goto_simulgame_bynum(int p, int num)
{
  struct player *pp = &player_globals.parray[p];
  int on;
  int start;
  int g;

  if (pp->simul_info == NULL)
    return -1;

  on = pp->simul_info->onBoard;
  start = on;
  do {
    on++;
    if (on >= pp->simul_info->numBoards)
      on = 0;
    g = pp->simul_info->boards[on];
    if (g == num)
      break;
  } while (start != on);
  if (g != num) {
    pprintf(p, "\nYou aren't playing that game!!\n");
    return -1;
  }
  return player_goto_board(p, on);
}

int player_num_active_boards(int p)
{
  struct player *pp = &player_globals.parray[p];
  int count = 0, i;

  if (pp->simul_info == NULL)
    return 0;

  if (!pp->simul_info->numBoards)
    return 0;
  for (i = 0; i < pp->simul_info->numBoards; i++)
    if (pp->simul_info->boards[i] >= 0)
      count++;
  return count;
}

static int player_num_results(int p, int result)
{
  struct player *pp = &player_globals.parray[p];
  switch (result) {
  case RESULT_WIN:
    return pp->simul_info->num_wins;
  case RESULT_DRAW:
    return pp->simul_info->num_draws;
  case RESULT_LOSS:
    return pp->simul_info->num_losses;
  default:
    return -1;
  }
}

static void new_simul_result(struct simul_info_t *sim, int result)
{
  switch (result) {
  case RESULT_WIN:
    sim->num_wins++;
    return;
  case RESULT_DRAW:
    sim->num_draws++;
    return;
  case RESULT_LOSS:
    sim->num_losses++;
    return;
  }
}

int player_simul_over(int p, int g, int result)
{
  struct player *pp = &player_globals.parray[p];
  int on, ong, p1, which;
  char tmp[1024];

  if (pp->simul_info == NULL)
    return -1;

  for (which = 0; which < pp->simul_info->numBoards; which++) {
    if (pp->simul_info->boards[which] == g) {
      break;
    }
  }
  if (which == pp->simul_info->numBoards) {
    pprintf(p, "I can't find that game!\n");
    return -1;
  }
  pprintf(p, "\nBoard %d has completed.\n", which + 1);
  on = pp->simul_info->onBoard;
  ong = pp->simul_info->boards[on];
  pp->simul_info->boards[which] = -1;
  new_simul_result(pp->simul_info, result);
  if (player_num_active_boards(p) == 0) {
    sprintf(tmp, "\n{Simul (%s vs. %d) is over.}\nResults: %d Wins, %d Losses, %d Draws, %d Aborts\n",
	    pp->name,
	    pp->simul_info->numBoards,
	    player_num_results(p, RESULT_WIN),
	    player_num_results(p, RESULT_LOSS),
	    player_num_results(p, RESULT_DRAW),
	    player_num_results(p, RESULT_ABORT));
    for (p1 = 0; p1 < player_globals.p_num; p1++) {
      if (pp->status != PLAYER_PROMPT)
	continue;
      if (!CheckPFlag(p1, PFLAG_GIN) && !player_is_observe(p1, g) && (p1 != p))
	continue;
      pprintf_prompt(p1, "%s", tmp);
    }
    pp->simul_info->numBoards = 0;
    pprintf_prompt(p, "\nThat was the last board, thanks for playing.\n");
    free(pp->simul_info);
    pp->simul_info = NULL;
    return 0;
  }
  if (ong == g) {		/* This game is over */
    player_goto_next_board(p);
  } else {
    player_goto_board(p, pp->simul_info->onBoard);
  }
  pprintf_prompt(p, "\nThere are %d boards left.\n",
		 player_num_active_boards(p));
  return 0;
}

static void GetMsgFile (int p, char *fName)
{
	struct player *pp = &player_globals.parray[p];
	sprintf(fName, STATS_DIR "/player_data/%c/%s.%s", pp->login[0],
		pp->login, STATS_MESSAGES);
}

int player_num_messages(int p)
{
  char fname[MAX_FILENAME_SIZE];

  if (!CheckPFlag(p, PFLAG_REG))
    return 0;
  GetMsgFile (p, fname);
  return lines_file(fname);
}

int player_add_message(int top, int fromp, char *message)
{
  char fname[MAX_FILENAME_SIZE];
  FILE *fp;
  char subj[256];
  char messbody[1024];
  time_t t = time(0);

  if (!CheckPFlag(top, PFLAG_REG))
    return -1;
  if (!CheckPFlag(fromp, PFLAG_REG))
    return -1;
  GetMsgFile (top, fname);
  if ((lines_file(fname) >= MAX_MESSAGES) && (player_globals.parray[top].adminLevel == 0))
    return -1;
  fp = fopen_s(fname, "a");
  if (!fp)
    return -1;
  fprintf(fp, "%s at %s: %s\n", player_globals.parray[fromp].name, strltime(&t), message);
  fclose(fp);
  pprintf(fromp, "\nThe following message was sent ");
  if (CheckPFlag(top, PFLAG_MAILMESS)) {
      sprintf(subj, "FICS message from %s at FICS %s (Do not reply by mail)", 
	      player_globals.parray[fromp].name, 
	      config_get_tmp("SERVER_HOSTNAME"));
      sprintf(messbody, "%s at %s: %s\n", player_globals.parray[fromp].name, strltime(&t), message);
      mail_string_to_user(top, subj, messbody);
      pprintf(fromp, "(and emailed) ");
  }
  pprintf(fromp, "to %s: \n   %s\n", player_globals.parray[top].name, message);
  return 0;
}

static int player_forward_message(int top, int fromp, char *message)
{
  char fname[MAX_FILENAME_SIZE];
  FILE *fp;
  char subj[256];
  char messbody[1024];

  if (!CheckPFlag(top, PFLAG_REG))
    return -1;
  if (!CheckPFlag(fromp, PFLAG_REG))
    return -1;
  GetMsgFile (top, fname);
  if ((lines_file(fname) >= MAX_MESSAGES) && (player_globals.parray[top].adminLevel == 0))
    return -1;
  fp = fopen_s(fname, "a");
  if (!fp)
    return -1;
  fprintf(fp, "FORWARDED MSG FROM %s : %s", player_globals.parray[fromp].name, message);
  fclose(fp);
  pprintf(fromp, "\nThe following message was forwarded ");
  if (CheckPFlag(top, PFLAG_MAILMESS)) {
      sprintf(subj, "FICS message from %s at FICS %s (Do not reply by mail)", 
	      player_globals.parray[fromp].name, 
	      config_get_tmp("SERVER_HOSTNAME"));
      sprintf(messbody, "FORWARDED MSG FROM %s: %s\n", player_globals.parray[fromp].name, message);
      mail_string_to_user(top, subj, messbody);
      pprintf(fromp, "(and emailed) ");
  }
  pprintf(fromp, "to %s: \n   %s\n", player_globals.parray[top].name, message);
  return 0;
}
void SaveTextListEntry(textlist **Entry, char *string, int n)
{
  *Entry = (textlist *) malloc(sizeof(textlist));
  (*Entry)->text = strdup(string);
  (*Entry)->index = n;
  (*Entry)->next = NULL;
}

static textlist *ClearTextListEntry(textlist *entry)
{
  textlist *ret = entry->next;
  free(entry->text);
  free(entry);
  return ret;
}

void ClearTextList(textlist *head)
{
  textlist *cur;

  for (cur = head; cur != NULL; cur = ClearTextListEntry(cur));
}

static int SaveThisMsg (int which, char *line)
{
  char Sender[MAX_LOGIN_NAME];
  int p1;

  if (which == 0) return 1;

  sscanf (line, "%s", Sender);
  if (which < 0) {
    p1 = -which - 1;
    return strcmp(Sender, player_globals.parray[p1].name);
  }
  else {
    p1 = which - 1;
    return !strcmp(Sender, player_globals.parray[p1].name);
  }
}

static int LoadMsgs(int p, int which, textlist **Head)
{
  FILE *fp;
  textlist **Cur = Head;
  char fName[MAX_FILENAME_SIZE];
  char line[MAX_LINE_SIZE];
  int n=0, nSave=0;

  *Head = NULL;
  GetMsgFile (p, fName);
  fp = fopen_s(fName, "r");
  if (fp == NULL) {
    return -1;
  }
  while (!feof(fp)) {
    if (fgets(line, MAX_LINE_SIZE, fp) == NULL || feof(fp))
      break;
    if (SaveThisMsg(which, line)) {
      SaveTextListEntry(Cur, line, ++n);
      Cur = &(*Cur)->next;
      nSave++;
    }
    else n++;
  }
  fclose (fp);
  return nSave;
}

/* start > 0 and end > start (or end = 0) to save messages in range;
   start < 0 and end < start (or end = 0) to clear messages in range;
   if end = 0, range goes to end of file (not tested yet). */
static int LoadMsgRange(int p, int start, int end, textlist **Head)
{
  FILE *fp;
  char fName[MAX_FILENAME_SIZE];
  char line[MAX_LINE_SIZE];
  textlist **Cur = Head;
  int n=1, nSave=0, nKill=0;

  *Head = NULL;
  GetMsgFile (p, fName);
  fp = fopen  (fName, "r");
  if (fp == NULL) {
    pprintf (p, "You have no messages.\n");
    return -1;
  }
  for (n=1; n <= end || end <= 0; n++) {
    if (fgets(line, MAX_LINE_SIZE, fp) == NULL || feof(fp))
      break;
    if ((start < 0 && (n < -start || n > -end)) || (start >= 0 && n >= start)) {
      SaveTextListEntry (Cur, line, n);
      Cur = &(*Cur)->next;
      nSave++;
    }
    else nKill++;
  }
  fclose (fp);
  if (start < 0) {
    if (n <= -start)
      pprintf (p, "You do not have a message %d.\n", -start);
    return nKill;
  } else {
    if (n <= start)
      pprintf (p, "You do not have a message %d.\n", start);
    return nSave;
  }
}

int ForwardMsgRange(char *p1, int p, int start, int end)
{
  /* p1 is player to   and p is player from */
  FILE *fp;
  char fName[MAX_FILENAME_SIZE];
  char line[MAX_LINE_SIZE];
  int n=1, top, connected;

  if (!FindPlayer(p, p1, &top, &connected))
    return -1;
  GetMsgFile (p, fName);
  fp = fopen  (fName, "r");
  if (fp == NULL) {
    pprintf (p, "You have no messages.\n");
    return -1;
  }
  for (n=1; n <= end || end <= 0; n++) {
    if (fgets(line, MAX_LINE_SIZE, fp) == NULL || feof(fp))
      break;
    if ((start < 0 && (n < -start || n > -end)) || (start >= 0 && n >= start)) {
      player_forward_message(top, p, line);
    }
  }
  fclose (fp);
  if (start < 0) {
    if (n <= -start)
      pprintf (p, "You do not have a message %d.\n", -start);
    return 0;
  } else {
    if (n <= start)
      pprintf (p, "You do not have a message %d.\n", start);
    return 0;
  }
}

static int WriteMsgFile (int p, textlist *Head)
{
  char fName[MAX_FILENAME_SIZE];
  FILE *fp;
  textlist *Cur;

  GetMsgFile (p, fName);
  fp = fopen_s(fName, "w");
  if (fp == NULL)
    return 0;
  for (Cur = Head; Cur != NULL; Cur = Cur->next)
    fprintf(fp, "%s", Cur->text);
  fclose(fp);
  return 1;
}

int ClearMsgsBySender(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	textlist *Head;
	int p1, connected;
	int nFound;
	
	if (!FindPlayer(p, param[0].val.word, &p1, &connected))
		return -1;
	
	nFound = LoadMsgs(p, -(p1+1), &Head);
	if (nFound < 0) {
		pprintf(p, "You have no messages.\n");
	} else if (nFound == 0) {
		pprintf(p, "You have no messages from %s.\n", player_globals.parray[p1].name);
	} else {
		if (WriteMsgFile (p, Head))
			pprintf(p, "Messages from %s cleared.\n", player_globals.parray[p1].name); 
		else {
			pprintf(p, "Problem writing message file; please contact an admin.\n");
			d_printf( "Problem writing message file for %s.\n", pp->name);
		}
		ClearTextList(Head);
	}
	if (!connected)
		player_remove(p1);
	return nFound;
}

static void ShowTextList (int p, textlist *Head, int ShowIndex)
{
  textlist *CurMsg;

  if (ShowIndex) {
    for (CurMsg = Head; CurMsg != NULL; CurMsg = CurMsg->next)
      pprintf(p, "%2d. %s", CurMsg->index, CurMsg->text);
  }
  else {
    for (CurMsg = Head; CurMsg != NULL; CurMsg = CurMsg->next)
      pprintf(p, "%s", CurMsg->text);
  }
}

int player_show_messages(int p)
{
  textlist *Head;
  int n;

  n = LoadMsgs (p, 0, &Head);
  if (n <= 0) {
    pprintf (p, "You have no messages.\n");
    return -1;
  } else {
    pprintf (p, "Messages:\n");
    ShowTextList (p, Head, 1);
    ClearTextList (Head);
    return 0;
  }
}

int ShowMsgsBySender(int p, param_list param)
{
  textlist *Head;
  int p1, connected;
  int nFrom;
  int nTo = 0;

  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return -1;

  if (!CheckPFlag(p1, PFLAG_REG)) {
    pprintf(p, "Player \"%s\" is unregistered and cannot send or receive messages.\n",
       player_globals.parray[p1].name);
    return -1; /* no need to disconnect */
  }
 
  if (p != p1) {
    nTo = LoadMsgs(p1, p+1, &Head);
    if (nTo <= 0) {
      pprintf(p, "%s has no messages from you.\n", player_globals.parray[p1].name);
    } else {
      pprintf(p, "Messages to %s:\n", player_globals.parray[p1].name);
      ShowTextList (p, Head, 0);
      ClearTextList(Head);
    }
  }
  nFrom = LoadMsgs(p, p1+1, &Head);
  if (nFrom <= 0) {
    pprintf(p, "\nYou have no messages from %s.\n", player_globals.parray[p1].name);
  } else {
    pprintf(p, "Messages from %s:\n", player_globals.parray[p1].name);
    ShowTextList (p, Head, 1);
    ClearTextList(Head);
  }
  if (!connected)
    player_remove(p1);
  return (nFrom > 0 || nTo > 0);
}

int ShowMsgRange (int p, int start, int end)
{
  textlist *Head;
  int n;

  n = LoadMsgRange (p, start, end, &Head);
  if (n > 0) {
    ShowTextList (p, Head, 1);
    ClearTextList (Head);
  }
  return n;
}

int ClrMsgRange (int p, int start, int end)
{
  textlist *Head;
  int n;

  n = LoadMsgRange (p, -start, -end, &Head);
  if (n > 0)
    if (WriteMsgFile (p, Head))
      pprintf (p, "Message %d cleared.\n", start);
  if (n >= 0)
    ClearTextList (Head);

  return n;
}

int player_clear_messages(int p)
{
  char fname[MAX_FILENAME_SIZE];

  if (!CheckPFlag(p, PFLAG_REG))
    return -1;
  GetMsgFile (p, fname);
  unlink(fname);
  return 0;
}

int player_search(int p, char *name)
/*
 * Find player matching the given string. First looks for exact match
 *  with a logged in player, then an exact match with a registered player,
 *  then a partial unique match with a logged in player, then a partial
 *  match with a registered player.
 *  Returns player number if the player is connected, negative (player number)
 *  if the player had to be connected, and 0 if no player was found
 */
{
  int p1, count;
  char *buffer[1000];
  char pdir[MAX_FILENAME_SIZE];

  /* exact match with connected player? */
  if ((p1 = player_find_bylogin(name)) >= 0) {
    return p1 + 1;
  }
  /* exact match with registered player? */
  sprintf(pdir, PLAYER_DIR "/%c", name[0]);
  count = search_directory(pdir, name, buffer, 1000);
  if (count > 0 && !strcmp(name, *buffer)) {
    goto ReadPlayerFromFile;	/* found an unconnected registered player */
  }
  /* partial match with connected player? */
  if ((p1 = player_find_part_login(name)) >= 0) {
    return p1 + 1;
  } else if (p1 == -2) {
    /* ambiguous; matches too many connected players. */
    pprintf (p, "Ambiguous name '%s'; matches more than one player.\n", name);
    return 0;
  }
  /* partial match with registered player? */
  if (count < 1) {
    pprintf(p, "There is no player matching that name.\n");
    return 0;
  }
  if (count > 1) {
    pprintf(p, "-- Matches: %d names --", count);
    display_directory(p, buffer, count);
    return(0);
  }
ReadPlayerFromFile:
  p1 = player_new();
  if (player_read(p1, *buffer)) {
    player_remove(p1);
    pprintf(p, "ERROR: a player named %s was expected but not found!\n",
	    *buffer);
    pprintf(p, "Please tell an admin about this incident. Thank you.\n");
    return 0;
  }
  return (-p1) - 1;		/* negative to indicate player was not
				   connected */
}


int player_kill(char *name)
{
  char fname[MAX_FILENAME_SIZE], fname2[MAX_FILENAME_SIZE];

  sprintf(fname, PLAYER_DIR "/%c/%s", name[0], name);
  sprintf(fname2, PLAYER_DIR "/%c/.rem.%s", name[0], name);
  rename(fname, fname2);
  RemHist (name);
  sprintf(fname, STATS_DIR "/player_data/%c/%s.games", name[0], name);
  sprintf(fname2, STATS_DIR "/player_data/%c/.rem.%s.games", name[0], name);
  rename(fname, fname2);
  sprintf(fname, STATS_DIR "/player_data/%c/%s.comments", name[0], name);
  sprintf(fname2, STATS_DIR "/player_data/%c/.rem.%s.comments", name[0], name);
  rename(fname, fname2);

  sprintf(fname, STATS_DIR "/player_data/%c/%s.logons", name[0], name);
  sprintf(fname2, STATS_DIR "/player_data/%c/.rem.%s.logons", name[0], name);
  rename(fname, fname2);
  sprintf(fname, STATS_DIR "/player_data/%c/%s.messages", name[0], name);
  sprintf(fname2, STATS_DIR "/player_data/%c/.rem.%s.messages", name[0], name);
  rename(fname, fname2);
  return 0;
}

int player_rename(char *name, char *newname)
{
  char fname[MAX_FILENAME_SIZE], fname2[MAX_FILENAME_SIZE];

  sprintf(fname, PLAYER_DIR "/%c/%s", name[0], name);
  sprintf(fname2, PLAYER_DIR "/%c/%s", newname[0], newname);
  rename(fname, fname2);
  sprintf(fname, STATS_DIR "/player_data/%c/%s.games", name[0], name);
  sprintf(fname2, STATS_DIR "/player_data/%c/%s.games", newname[0], newname);
  rename(fname, fname2);
  sprintf(fname, STATS_DIR "/player_data/%c/%s.comments", name[0], name);
  sprintf(fname2, STATS_DIR "/player_data/%c/%s.comments", newname[0], newname);
  rename(fname, fname2);
  sprintf(fname, STATS_DIR "/player_data/%c/%s.logons", name[0], name);
  sprintf(fname2, STATS_DIR "/player_data/%c/%s.logons", newname[0], newname);
  rename(fname, fname2);
  sprintf(fname, STATS_DIR "/player_data/%c/%s.messages", name[0], name);
  sprintf(fname2, STATS_DIR "/player_data/%c/%s.messages", newname[0], newname);
  rename(fname, fname2);
  return 0;
}

int player_reincarn(char *name, char *newname)
{
  char fname[MAX_FILENAME_SIZE], fname2[MAX_FILENAME_SIZE];

  sprintf(fname, PLAYER_DIR "/%c/%s", newname[0], newname);
  sprintf(fname2, PLAYER_DIR "/%c/.rem.%s", name[0], name);
  rename(fname2, fname);
  sprintf(fname, STATS_DIR "/player_data/%c/%s.games", newname[0], newname);
  sprintf(fname2, STATS_DIR "/player_data/%c/.rem.%s.games", name[0], name);
  rename(fname2, fname);
  sprintf(fname, STATS_DIR "/player_data/%c/%s.comments", newname[0], newname);
  sprintf(fname2, STATS_DIR "/player_data/%c/.rem.%s.comments", name[0], name);
  rename(fname2, fname);
  sprintf(fname, STATS_DIR "/player_data/%c/%s.logons", newname[0], newname);
  sprintf(fname2, STATS_DIR "/player_data/%c/.rem.%s.logons", name[0], name);
  rename(fname2, fname);
  sprintf(fname, STATS_DIR "/player_data/%c/%s.messages", newname[0], newname);
  sprintf(fname2, STATS_DIR "/player_data/%c/.rem.%s.messages", name[0], name);
  rename(fname2, fname);
  return 0;
}

int player_num_comments(int p)
{
	struct player *pp = &player_globals.parray[p];
	char fname[MAX_FILENAME_SIZE];

	if (!CheckPFlag(p, PFLAG_REG))
		return 0;
	sprintf(fname, STATS_DIR "/player_data/%c/%s.%s", pp->login[0],
		pp->login, "comments");
	return lines_file(fname);
}

int player_add_comment(int p_by, int p_to, char *comment)
{
  char fname[MAX_FILENAME_SIZE];
  FILE *fp;
  time_t t = time(0);

  if (!CheckPFlag(p_to, PFLAG_REG))
    return -1;
  sprintf(fname, STATS_DIR "/player_data/%c/%s.%s", player_globals.parray[p_to].login[0],
	  player_globals.parray[p_to].login, "comments");
  fp = fopen_s(fname, "a");
  if (!fp)
    return -1;
  fprintf(fp, "%s at %s: %s\n", player_globals.parray[p_by].name, strltime(&t), comment);
  fclose(fp);
  player_globals.parray[p_to].num_comments = player_num_comments(p_to);
  return 0;
}

int player_show_comments(int p, int p1)
{
  char fname[MAX_FILENAME_SIZE];

  if (CheckPFlag(p1, PFLAG_REG)) {
    sprintf(fname, STATS_DIR "/player_data/%c/%s.%s", player_globals.parray[p1].login[0],
  	  player_globals.parray[p1].login, "comments");
    if (psend_file(p, NULL, fname))
      pprintf(p, "There are no comments to show for %s.\n", player_globals.parray[p1].name);
    if (player_globals.parray[p1].passwd[0] == '*')
      pprintf(p, "%s's account is LOCKED.\n", player_globals.parray[p1].name);
    if (in_list(p1, L_BAN, player_globals.parray[p1].name))
      pprintf(p, "%s has been BANNED.\n", player_globals.parray[p1].name);
  } else
    pprintf(p, "Player \"%s\" is unregistered and cannot have comments.\n",
               player_globals.parray[p1].name);
  return 0;
}

/* returns 1 if player is head admin, 0 otherwise */
int player_ishead(int p)
{
	struct player *pp = &player_globals.parray[p];
	return (strcasecmp(pp->name, config_get_tmp("HEAD_ADMIN")) == 0);
}

/* GetRating chooses between blitz, standard and other ratings. */
int GetRating(struct player *p, int gametype)
{
    if (gametype == TYPE_BLITZ) return (p->b_stats.rating);
    else if (gametype == TYPE_STAND) return (p->s_stats.rating);
    else if (gametype == TYPE_WILD) return (p->w_stats.rating);
    else if (gametype == TYPE_LIGHT) return (p->l_stats.rating);
    else if (gametype == TYPE_BUGHOUSE) return (p->bug_stats.rating);
    else return 0;
}    /* end of function GetRating. */

/* GetRD chooses between blitz, standard and other RD's. */
double GetRD(struct player *p, int gametype)
{
  struct statistics *s;

  switch(gametype) {
    case TYPE_BLITZ:   s = &p->b_stats;   break;
    case TYPE_STAND:   s = &p->s_stats;   break;
    case TYPE_WILD:   s = &p->w_stats;   break;
    case TYPE_LIGHT:   s = &p->l_stats;   break;
    case TYPE_BUGHOUSE:   s = &p->bug_stats;   break;
    default:   return 0.0;
  }
  return (current_sterr(s->sterr, time(NULL)-(s->ltime)));
}    /* end of function GetRD. */

