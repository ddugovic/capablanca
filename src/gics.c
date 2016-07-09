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

/*
 * FICS-Extensions used at GICS chess.unix-ag.uni-kl.de 5000
 * (c) 1995/1996 by Ulrich Schlechte <Ulrich.Schlechte@ipp.tu-clausthal.de>
 *		    Klaus Knopper <Knopper@unix-ag.uni-kl.de>
 *
 *
*/

#include "includes.h"


void init_userstat(void)
{
	FILE *fp;
	int i;

	if ((fp = fopen_s(STATS_DIR "/user.stats", "r")) == NULL) {
		bzero((char *) &gics_globals.userstat, sizeof(gics_globals.userstat));
		return;
	}

	for (i = 0; i < 48; i++) {
		fscanf(fp, "%d\n", &gics_globals.userstat.users[i]);
	}
	fscanf(fp, "%d\n", &gics_globals.userstat.usermax);
	fscanf(fp, "%d\n", (int *) &gics_globals.userstat.usermaxtime);
	fscanf(fp, "%d\n", &gics_globals.userstat.logins);
	fscanf(fp, "%d\n", &gics_globals.userstat.games);
	fscanf(fp, "%d\n", &gics_globals.userstat.gamemax);
	fscanf(fp, "%d\n", (int *) &gics_globals.userstat.gamemaxtime);
	fclose(fp);
}

void save_userstat(void)
{
	FILE *fp;
	int i;
	
	if ((fp = fopen_s(STATS_DIR "/user.stats", "w")) == NULL)
		return;
	for (i = 0; i < 48; i++)
		fprintf(fp, "%d\n", gics_globals.userstat.users[i]);
	fprintf(fp, "%d\n%d\n%d\n", 
		gics_globals.userstat.usermax, gics_globals.userstat.usermaxtime, gics_globals.userstat.logins);
	fprintf(fp, "%d\n%d\n%d\n", 
		gics_globals.userstat.games, gics_globals.userstat.gamemax, gics_globals.userstat.gamemaxtime);
	fclose(fp);
}

#if 0
int com_ping(int p, param_list param)
{
  int p1;
  struct timeval timeState;
  struct timezone tz;

  if ((p1 = player_find_bylogin(param[0].val.word)) < 0) {
    pprintf(p, "Player \"%s\" not logged on!\n", param[0].val.word);
    return COM_OK;
  }
  if ((!con[player_globals.parray[p1].socket].timeseal) || (con[player_globals.parray[p1].socket].method)) {
    pprintf(p, "%s is not using timeseal or uses an old client.\n", player_globals.parray[p1].name);
    return COM_OK;
  }
  pprintf(p1, "\n[P]\n");
  gettimeofday(&timeState, &tz);
  con[player_globals.parray[p1].socket].pingtime = (((int) timeState.tv_sec % 10000) * 1000) + ((int) timeState.tv_usec / 1000);
  con[player_globals.parray[p1].socket].pingplayer = p;
  return COM_OK;
}
#endif

void game_save_playerratio(char *file, char *Opponent, int Result, int rated)
{
  FILE *fp, *fp1;
  char Name[32], file1[256];
  int win, loss, draw, flag = 0, inc = 1;

  if (!rated)
    inc = 100000;

  sprintf(file1, "%s.new", file);
  if ((fp = fopen_s(file, "r")) != NULL) {
    fp1 = fopen_s(file1, "w");
    while (!feof(fp)) {
      if (fscanf(fp, "%s %d %d %d\n", Name, &win, &draw, &loss) != 4) {
	break;
      }
      if (strcasecmp(Name, Opponent) == 0) {
	if (Result == 1)
	  win += inc;
	if (Result == 0)
	  loss += inc;
	if (Result == -1)
	  draw += inc;
	flag++;
      }
      fprintf(fp1, "%s %d %d %d\n", Name, win, draw, loss);
    }
    fclose(fp);
    if (!flag) {
      win = loss = draw = 0;
      if (Result == 1)
	win += inc;
      if (Result == 0)
	loss += inc;
      if (Result == -1)
	draw += inc;
      fprintf(fp1, "%s %d %d %d\n", Opponent, win, draw, loss);
    }
    fclose(fp1);
    unlink(file);
    if (rename(file1, file) != 0) {
      d_printf("CHESSD: Problems rename %s to %s : %d\n", file1, file, errno);
      return;
    }
    return;
  } else {
    fp = fopen_s(file, "w");
    if(fp == NULL) {
	perror("fopen");
	d_printf("Unable to open %s - very bad, info not saved\n",file);
	return;
    }
    win = loss = draw = 0;
    if (Result == 1)
      win += inc;
    if (Result == 0)
      loss += inc;
    if (Result == -1)
      draw += inc;
    fprintf(fp, "%s %d %d %d\n", Opponent, win, draw, loss);
    fclose(fp);
  }
}

int com_pstat(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1 = p, p2, ptemp, pflag=0;
  int p1_connected = 1, p2_connected = 1;
  int win = 0, draw = 0, loss = 0, flag = 0;
  FILE *fp;
  char Name[32], fname[256];

  if (param[0].type == TYPE_NULL) {
    if (pp->game < 0) {
      pprintf(p, "You are not playing a game.\n");
      return COM_OK;
    }
    if (game_globals.garray[pp->game].status == GAME_EXAMINE) {
      pprintf(p, "You are examining a game please specify variables.\n");
      return COM_OK;
    }
    p2 = pp->opponent;
  } else {
    if (!FindPlayer(p, param[0].val.word, &p2, &p2_connected)) {
      pprintf(p, "No user named \"%s\" is logged in.\n", param[0].val.word);
      return COM_OK;
    }
    if (param[1].type != TYPE_NULL) {
      if (!FindPlayer(p, param[1].val.word, &p1, &p1_connected)) {
	pprintf(p, "No user named \"%s\" is logged in.\n", param[1].val.word);
	if (!p2_connected)
	  player_remove(p2);
	return COM_OK;
      }
      ptemp = p2;
      p2 = p1;
      p1 = ptemp;
      pflag = 1;
    }
  }
  if (p1 == p2) {
    pprintf(p, "You have not played against yourself ;-)\n");
    if (!p1_connected)
      player_remove(p1);
    if (!p2_connected)
      player_remove(p2);
    return COM_OK;
  }
  sprintf(fname, "%s/player_data/%c/%s.gstats", STATS_DIR, player_globals.parray[p1].login[0], player_globals.parray[p1].login);
  if ((fp = fopen_s(fname, "r")) != NULL) {
    while (!feof(fp)) {
      if (fscanf(fp, "%s %d %d %d\n", Name, &win, &draw, &loss) != 4) {
	win = loss = draw = 0;
	break;
      }
      if (strcasecmp(Name, player_globals.parray[p2].name) == 0) {
	flag++;
	break;
      }
    }
    fclose(fp);
  }
  if (!flag)
    win = loss = draw = 0;
  pprintf(p, "Record for %s vs. %s:\n", player_globals.parray[p1].name, player_globals.parray[p2].name);
  pprintf(p, "                            wins     losses     draws\n");
  pprintf(p, "                rated       %4d       %4d      %4d\n", win % 100000, loss % 100000, draw % 100000);
  pprintf(p, "              unrated       %4d       %4d      %4d\n", win / 100000, loss / 100000, draw / 100000);
  /*  pprintf(p, "Ratio of %s vs. %s (r/u): %d/%d wins %d/%d draws %d/%d losses\n", player_globals.parray[p1].name, player_globals.parray[p2].name,
   *	  win % 100000, win / 100000, draw % 100000, draw / 100000, loss % 100000, loss / 100000);
   */
  if (pflag) {
    ptemp = p1;
    p1 = p2;
    p2 = ptemp;
  }
  if (!p1_connected)
    player_remove(p1);
  if (!p2_connected)
    player_remove(p2);
  return COM_OK;
}









