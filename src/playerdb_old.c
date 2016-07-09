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

/*
  support for reading the old player formats
  thank goodness I don't need to update this any more! It sure was a complex format.
*/


/* return 0 if corrupt */

static int ReadV1PlayerFmt(int p,struct player *pp, FILE * fp, const char *file, int version)
{

 int i,size_cens, size_noplay, size_not, size_gnot, size_chan, len;
 int bs,ss,ws,ls,bugs;
 int bell, pgn, notifiedby, pin, gin, avail, private, jprivate;
 int open, ropen, rated, tell, kib, shout, cshout, automail, mailmess;
 int numAlias = 0;

 char* tmp;
 char tmp2[MAX_STRING_LENGTH];

 fgets(tmp2, MAX_STRING_LENGTH, fp);
 if (strcmp(tmp2,"NONE\n")) {
   tmp2[strlen(tmp2)-1] = '\0';
   pp->name = strdup (tmp2);
 } else
   pp->name = NULL;
 fgets(tmp2, MAX_STRING_LENGTH, fp);
 if (strcmp(tmp2,"NONE\n")) {
   tmp2[strlen(tmp2)-1] = '\0';
   pp->fullName = strdup (tmp2);
 } else
   pp->fullName = NULL;
 fgets(tmp2, MAX_STRING_LENGTH, fp);
 if (strcmp(tmp2,"NONE\n")) {
   tmp2[strlen(tmp2)-1] = '\0';
   pp->passwd = strdup (tmp2);
 } else
   pp->passwd = NULL;
 fgets(tmp2, MAX_STRING_LENGTH, fp);
 if (strcmp(tmp2,"NONE\n")) {
   tmp2[strlen(tmp2)-1] = '\0';
   pp->emailAddress = strdup (tmp2);
 } else
   pp->emailAddress = NULL;
 if (fscanf(fp, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %d\n",

 &pp->s_stats.num, &pp->s_stats.win, &pp->s_stats.los,
 &pp->s_stats.dra, &pp->s_stats.rating, &ss,
 &pp->s_stats.ltime, &pp->s_stats.best, &pp->s_stats.whenbest,

 &pp->b_stats.num, &pp->b_stats.win, &pp->b_stats.los,
 &pp->b_stats.dra, &pp->b_stats.rating, &bs,
 &pp->b_stats.ltime, &pp->b_stats.best, &pp->b_stats.whenbest,

 &pp->w_stats.num, &pp->w_stats.win, &pp->w_stats.los,
 &pp->w_stats.dra, &pp->w_stats.rating, &ws,
 &pp->w_stats.ltime, &pp->w_stats.best, &pp->w_stats.whenbest,

 &pp->l_stats.num, &pp->l_stats.win, &pp->l_stats.los,
 &pp->l_stats.dra, &pp->l_stats.rating, &ls,
 &pp->l_stats.ltime, &pp->l_stats.best, &pp->l_stats.whenbest,

 &pp->bug_stats.num, &pp->bug_stats.win, &pp->bug_stats.los,
 &pp->bug_stats.dra, &pp->bug_stats.rating, &bugs,
 &pp->bug_stats.ltime, &pp->bug_stats.best, &pp->bug_stats.whenbest,
 &pp->lastHost.s_addr) != 46) {
  d_printf("Player %s is corrupt\n",player_globals.parray[p].name);
    free(pp->fullName);
    free(pp->passwd);
    free(pp->emailAddress);
    pp->fullName = NULL;
    pp->passwd = NULL;
    pp->emailAddress = NULL;
  return 0; 
 }

 pp->b_stats.sterr = bs / 10.0;
 pp->s_stats.sterr = ss / 10.0;
 pp->w_stats.sterr = ws / 10.0;
 pp->l_stats.sterr = ls / 10.0;
 pp->bug_stats.sterr = bugs / 10.0;

 fgets (tmp2, MAX_STRING_LENGTH, fp);
   tmp2[strlen(tmp2)-1] = '\0';
   pp->prompt = strdup(tmp2);
 if (version == 1) {
   if (fscanf (fp, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
   &open, &rated, &ropen, &pp->timeOfReg,
   &pp->totalTime, &bell, &pgn, &notifiedby,
   &pin, &gin, &shout, &cshout,
   &tell, &kib, &private, &jprivate,
   &automail, &mailmess, &pp->style, &pp->d_time,
   &pp->d_inc, &pp->d_height, &pp->d_width, &pp->language,
   &pp->adminLevel, &pp->num_white, &pp->num_black, &pp->highlight,
   &pp->num_comments,
   &pp->num_plan, &pp->num_formula,&size_cens,
   &size_not, &size_noplay,
   &size_gnot, &numAlias, &size_chan) != 37) {
    d_printf("Player %s is corrupt.\n",player_globals.parray[p].name);
    free(pp->prompt);
    pp->prompt = config_get("DEFAULT_PROMPT");
    free(pp->fullName);
    free(pp->passwd);
    free(pp->emailAddress);
    pp->fullName = NULL;
    pp->passwd = NULL;
    pp->emailAddress = NULL;
    pp->num_plan = 0;
    pp->num_formula = 0;
    pp->adminLevel = 0;
    return 0;
   }
 } else if (version == 2) {
   if (fscanf (fp, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
   &open, &rated, &ropen, &pp->timeOfReg,
   &pp->totalTime, &bell, &pgn, &notifiedby,
   &pin, &gin, &avail, &shout, &cshout,
   &tell, &kib, &private, &jprivate,
   &automail, &mailmess, &pp->style, &pp->d_time,
   &pp->d_inc, &pp->d_height, &pp->d_width, &pp->language,
   &pp->adminLevel, &pp->num_white, &pp->num_black, &pp->highlight,
   &pp->num_comments,
   &pp->num_plan, &pp->num_formula,&size_cens,
   &size_not, &size_noplay,
   &size_gnot, &numAlias, &size_chan) != 38) {
    d_printf("Player %s is corrupt.\n",player_globals.parray[p].name);
    free(pp->prompt);
    pp->prompt = config_get("DEFAULT_PROMPT");
    free(pp->fullName);
    free(pp->passwd);
    free(pp->emailAddress);
    pp->fullName = NULL;
    pp->passwd = NULL;
    pp->emailAddress = NULL;
    pp->num_plan = 0;
    pp->num_formula = 0;
    pp->adminLevel = 0;
    return 0;
   }
 } else if (version > 2) {
   if (fscanf (fp, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
   &open, &rated, &ropen, &pp->timeOfReg,
   &pp->totalTime, &bell, &pgn, &notifiedby,
   &pin, &gin, &avail, &shout, &cshout,
   &tell, &kib, &private, &jprivate,
   &automail, &mailmess, &pp->style, &pp->d_time,
   &pp->d_inc, &pp->d_height, &pp->d_width, &pp->language,
   &pp->adminLevel, &pp->num_white, &pp->num_black, &pp->highlight,
   &pp->num_comments,
   &pp->num_plan, &pp->num_formula,&size_cens,
   &size_not, &size_noplay,
   &size_gnot, &numAlias, &size_chan, &pp->availmin, &pp->availmax) != 40) {
    d_printf("Player %s is corrupt.\n",player_globals.parray[p].name);
    free(pp->prompt);
    pp->prompt = config_get("DEFAULT_PROMPT");
    free(pp->fullName);
    free(pp->passwd);
    free(pp->emailAddress);
    pp->fullName = NULL;
    pp->passwd = NULL;
    pp->emailAddress = NULL;
    pp->num_plan = 0;
    pp->num_formula = 0;
    pp->adminLevel = 0;
    return 0;
   }
 }
 SetFlag(pp->Flags, PFLAG_OPEN, open);
 SetFlag(pp->Flags, PFLAG_ROPEN, ropen);
 SetFlag(pp->Flags, PFLAG_RATED, rated);
 SetFlag(pp->Flags, PFLAG_BELL, bell);
 SetFlag(pp->Flags, PFLAG_PGN, pgn);
 SetFlag(pp->Flags, PFLAG_NOTIFYBY, notifiedby);
 SetFlag(pp->Flags, PFLAG_PIN, pin);
 SetFlag(pp->Flags, PFLAG_GIN, gin);
 SetFlag(pp->Flags, PFLAG_AVAIL, avail);
 SetFlag(pp->Flags, PFLAG_PRIVATE, private);
 SetFlag(pp->Flags, PFLAG_JPRIVATE, jprivate);
 SetFlag(pp->Flags, PFLAG_SHOUT, shout);
 SetFlag(pp->Flags, PFLAG_CSHOUT, cshout);
 SetFlag(pp->Flags, PFLAG_TELL, tell);
 SetFlag(pp->Flags, PFLAG_KIBITZ, kib);
 SetFlag(pp->Flags, PFLAG_AUTOMAIL, automail);
 SetFlag(pp->Flags, PFLAG_MAILMESS, mailmess);

 if (version > 3)
   getc (fp); /* skip a dot */

   if (pp->num_plan > 0) {
     for (i = 0; i < pp->num_plan; i++) {
       fgets(tmp2, MAX_LINE_SIZE, fp);
       if (!(len = strlen(tmp2))) {
         d_printf( "CHESSD: Error bad plan in file %s\n", file);
         i--;
         pp->num_plan--; 
       } else {
         tmp2[len - 1] = '\0';  /* Get rid of '\n' */
         pp->planLines[i] = (len > 1) ? strdup(tmp2) : NULL;
       }
     }
   }
   if (pp->num_formula > 0) {
     for (i = 0; i < pp->num_formula; i++) {
       fgets(tmp2, MAX_LINE_SIZE, fp);
       if (!(len = strlen(tmp2))) {
         d_printf( "CHESSD: Error bad formula in file %s\n", file);
         i--;
         pp->num_formula--;
       } else {
         tmp2[len - 1] = '\0';  /* Get rid of '\n' */
         pp->formulaLines[i] = (len > 1) ? strdup(tmp2) : NULL;
       }
     }
   }
  fgets(tmp2, MAX_LINE_SIZE, fp);
  tmp2[strlen(tmp2) - 1] = '\0';
  if (!strcmp (tmp2,"NONE"))
    pp->formula = NULL;
  else
    pp->formula = strdup(tmp2);

  if (numAlias > 0) {
    for (i = 0; i < numAlias; i++) {
      fgets(tmp2, MAX_LINE_SIZE, fp);
      if (!(len = strlen(tmp2))) {
        d_printf( "CHESSD: Error bad alias in file %s\n", file);
        i--;
      } else {
        tmp2[len - 1] = '\0';  /* Get rid of '\n' */
        tmp = tmp2;
        tmp = eatword(tmp2);
        *tmp = '\0';
        tmp++;
        tmp = eatwhite(tmp);
	alias_add(p, tmp2, tmp);
      }
    }
  }

    while (size_cens--) {
      fscanf(fp,"%s",tmp2);
      list_add(p, L_CENSOR, tmp2);
      }

    while(size_not--) {
      fscanf(fp,"%s",tmp2);
      list_add(p, L_NOTIFY, tmp2);
      }

    while(size_noplay--) {
      fscanf(fp,"%s",tmp2);
      list_add(p, L_NOPLAY, tmp2);
      }

    while(size_gnot--) {
      fscanf(fp,"%s",tmp2);
      list_add(p, L_GNOTIFY, tmp2);
      }

    while(size_chan--) {
      fscanf(fp,"%s",tmp2);
      list_add(p, L_CHANNEL, tmp2);
      }
  return 1;
}

static int got_attr_value_player(int p, char *attr, char *value, FILE * fp, const char *file)
{
  int i, len;
  char tmp[MAX_LINE_SIZE], *tmp1;
  int numAlias = 0;

  if (!strcmp(attr, "name:")) {
    player_globals.parray[p].name = strdup(value);
  } else if (!strcmp(attr, "password:")) {
    player_globals.parray[p].passwd = strdup(value);
  } else if (!strcmp(attr, "fullname:")) {
    player_globals.parray[p].fullName = strdup(value);
  } else if (!strcmp(attr, "email:")) {
    player_globals.parray[p].emailAddress = strdup(value);
  } else if (!strcmp(attr, "prompt:")) {
    player_globals.parray[p].prompt = strdup(value);
  } else if (!strcmp(attr, "s_num:")) {
    player_globals.parray[p].s_stats.num = atoi(value);
  } else if (!strcmp(attr, "s_win:")) {
    player_globals.parray[p].s_stats.win = atoi(value);
  } else if (!strcmp(attr, "s_loss:")) {
    player_globals.parray[p].s_stats.los = atoi(value);
  } else if (!strcmp(attr, "s_draw:")) {
    player_globals.parray[p].s_stats.dra = atoi(value);
  } else if (!strcmp(attr, "s_rating:")) {
    player_globals.parray[p].s_stats.rating = atoi(value);
  } else if (!strcmp(attr, "s_sterr:")) {
    player_globals.parray[p].s_stats.sterr = (atoi(value) / 10.0);
  } else if (!strcmp(attr, "s_ltime:")) {
    player_globals.parray[p].s_stats.ltime = atoi(value);
  } else if (!strcmp(attr, "s_best:")) {
    player_globals.parray[p].s_stats.best = atoi(value);
  } else if (!strcmp(attr, "s_wbest:")) {
    player_globals.parray[p].s_stats.whenbest = atoi(value);
  } else if (!strcmp(attr, "b_num:")) {
    player_globals.parray[p].b_stats.num = atoi(value);
  } else if (!strcmp(attr, "b_win:")) {
    player_globals.parray[p].b_stats.win = atoi(value);
  } else if (!strcmp(attr, "b_loss:")) {
    player_globals.parray[p].b_stats.los = atoi(value);
  } else if (!strcmp(attr, "b_draw:")) {
    player_globals.parray[p].b_stats.dra = atoi(value);
  } else if (!strcmp(attr, "b_rating:")) {
    player_globals.parray[p].b_stats.rating = atoi(value);
  } else if (!strcmp(attr, "b_sterr:")) {
    player_globals.parray[p].b_stats.sterr = (atoi(value) / 10.0);
  } else if (!strcmp(attr, "b_ltime:")) {
    player_globals.parray[p].b_stats.ltime = atoi(value);
  } else if (!strcmp(attr, "b_best:")) {
    player_globals.parray[p].b_stats.best = atoi(value);
  } else if (!strcmp(attr, "b_wbest:")) {
    player_globals.parray[p].b_stats.whenbest = atoi(value);
  } else if (!strcmp(attr, "w_num:")) {
    player_globals.parray[p].w_stats.num = atoi(value);
  } else if (!strcmp(attr, "w_win:")) {
    player_globals.parray[p].w_stats.win = atoi(value);
  } else if (!strcmp(attr, "w_loss:")) {
    player_globals.parray[p].w_stats.los = atoi(value);
  } else if (!strcmp(attr, "w_draw:")) {
    player_globals.parray[p].w_stats.dra = atoi(value);
  } else if (!strcmp(attr, "w_rating:")) {
    player_globals.parray[p].w_stats.rating = atoi(value);
  } else if (!strcmp(attr, "w_sterr:")) {
    player_globals.parray[p].w_stats.sterr = (atoi(value) / 10.0);
  } else if (!strcmp(attr, "w_ltime:")) {
    player_globals.parray[p].w_stats.ltime = atoi(value);
  } else if (!strcmp(attr, "w_best:")) {
    player_globals.parray[p].w_stats.best = atoi(value);
  } else if (!strcmp(attr, "w_wbest:")) {
    player_globals.parray[p].w_stats.whenbest = atoi(value);
  } else if (!strcmp(attr, "open:")) {
    SetPFlag(p, PFLAG_OPEN, atoi(value));
  } else if (!strcmp(attr, "rated:")) {
    SetPFlag(p, PFLAG_RATED, atoi(value));
  } else if (!strcmp(attr, "ropen:")) {
    SetPFlag(p, PFLAG_ROPEN, atoi(value));
  } else if (!strcmp(attr, "bell:")) {
    SetPFlag(p, PFLAG_BELL, atoi(value));
  } else if (!strcmp(attr, "pgn:")) {
    SetPFlag(p, PFLAG_PGN, atoi(value));
  } else if (!strcmp(attr, "timeofreg:")) {
    player_globals.parray[p].timeOfReg = atoi(value);
  } else if (!strcmp(attr, "totaltime:")) {
    player_globals.parray[p].totalTime = atoi(value);
  } else if (!strcmp(attr, "notifiedby:")) {
    SetPFlag(p, PFLAG_NOTIFYBY, atoi(value));
  } else if (!strcmp(attr, "i_login:")) {
    SetPFlag(p, PFLAG_PIN, atoi(value));
  } else if (!strcmp(attr, "i_game:")) {
    SetPFlag(p, PFLAG_GIN, atoi(value));
  } else if (!strcmp(attr, "i_shout:")) {
    SetPFlag(p, PFLAG_SHOUT, atoi(value));
  } else if (!strcmp(attr, "i_cshout:")) {
    SetPFlag(p, PFLAG_CSHOUT, atoi(value));
  } else if (!strcmp(attr, "i_tell:")) {
    SetPFlag(p, PFLAG_TELL, atoi(value));
  } else if (!strcmp(attr, "i_kibitz:")) {
    SetPFlag(p, PFLAG_KIBITZ, atoi(value));
  } else if (!strcmp(attr, "kiblevel:")) {
    player_globals.parray[p].kiblevel = atoi(value);
  } else if (!strcmp(attr, "private:")) {
    SetPFlag(p, PFLAG_PRIVATE, atoi(value));
  } else if (!strcmp(attr, "jprivate:")) {
    SetPFlag(p, PFLAG_JPRIVATE, atoi(value));
  } else if (!strcmp(attr, "automail:")) {
    SetPFlag(p, PFLAG_AUTOMAIL, atoi(value));
  } else if (!strcmp(attr, "i_mailmess:")) {
    SetPFlag(p, PFLAG_MAILMESS, atoi(value));
  } else if (!strcmp(attr, "style:")) {
    player_globals.parray[p].style = atoi(value);
  } else if (!strcmp(attr, "d_time:")) {
    player_globals.parray[p].d_time = atoi(value);
  } else if (!strcmp(attr, "d_inc:")) {
    player_globals.parray[p].d_inc = atoi(value);
  } else if (!strcmp(attr, "d_height:")) {
    player_globals.parray[p].d_height = atoi(value);
  } else if (!strcmp(attr, "d_width:")) {
    player_globals.parray[p].d_width = atoi(value);
  } else if (!strcmp(attr, "language:")) {
    player_globals.parray[p].language = atoi(value);
  } else if (!strcmp(attr, "admin_level:")) {
    player_globals.parray[p].adminLevel = atoi(value);
    if (player_globals.parray[p].adminLevel >= ADMIN_ADMIN)
      PFlagON(p, PFLAG_ADMINLIGHT);
  } else if (!strcmp(attr, "i_admin:")) {
/*    player_globals.parray[p].i_admin = atoi(value);  */
  } else if (!strcmp(attr, "computer:")) {
/*    player_globals.parray[p].computer = atoi(value); */
  } else if (!strcmp(attr, "black_games:")) {
    player_globals.parray[p].num_black = atoi(value);
  } else if (!strcmp(attr, "white_games:")) {
    player_globals.parray[p].num_white = atoi(value);
  } else if (!strcmp(attr, "uscf:")) {
/*    player_globals.parray[p].uscfRating = atoi(value); */
  } else if (!strcmp(attr, "muzzled:")) {	/* ignore these: obsolete */
  } else if (!strcmp(attr, "cmuzzled:")) {	/* ignore these: obsolete */
  } else if (!strcmp(attr, "highlight:")) {
    player_globals.parray[p].highlight = atoi(value);
  } else if (!strcmp(attr, "network:")) {
/*    player_globals.parray[p].network_player = atoi(value); */
  } else if (!strcmp(attr, "lasthost:")) {
    player_globals.parray[p].lastHost.s_addr = atoi(value);
  } else if (!strcmp(attr, "channel:")) {
    list_addsub(p,"channel",value, 1);
  } else if (!strcmp(attr, "num_comments:")) {
    player_globals.parray[p].num_comments = atoi(value);
  } else if (!strcmp(attr, "num_plan:")) {
    player_globals.parray[p].num_plan = atoi(value);
    if (player_globals.parray[p].num_plan > 0) {
      for (i = 0; i < player_globals.parray[p].num_plan; i++) {
	fgets(tmp, MAX_LINE_SIZE, fp);
	if (!(len = strlen(tmp))) {
	  d_printf( "CHESSD: Error bad plan in file %s\n", file);
          i--;
          player_globals.parray[p].num_plan--;
	} else {
	  tmp[len - 1] = '\0';	/* Get rid of '\n' */
	  player_globals.parray[p].planLines[i] = (len > 1) ? strdup(tmp) : NULL;
	}
      }
    }
  } else if (!strcmp(attr, "num_formula:")) {
    player_globals.parray[p].num_formula = atoi(value);
    if (player_globals.parray[p].num_formula > 0) {
      for (i = 0; i < player_globals.parray[p].num_formula; i++) {
	fgets(tmp, MAX_LINE_SIZE, fp);
	if (!(len = strlen(tmp))) {
	  d_printf( "CHESSD: Error bad formula in file %s\n", file);
          i--;
          player_globals.parray[p].num_formula--;
	} else {
	  tmp[len - 1] = '\0';	/* Get rid of '\n' */
	  player_globals.parray[p].formulaLines[i] = (len > 1) ? strdup(tmp) : NULL;
	}
      }
    }
  } else if (!strcmp(attr, "formula:")) {
    player_globals.parray[p].formula = strdup(value);
  } else if (!strcmp(attr, "num_alias:")) {
    numAlias = atoi(value);
    if (numAlias > 0) {
      for (i = 0; i < numAlias; i++) {
	fgets(tmp, MAX_LINE_SIZE, fp);
	if (!(len = strlen(tmp))) {
	  d_printf( "CHESSD: Error bad alias in file %s\n", file);
	  i--;
	} else {
	  tmp[len - 1] = '\0';	/* Get rid of '\n' */
	  tmp1 = tmp;
	  tmp1 = eatword(tmp1);
	  *tmp1 = '\0';
	  tmp1++;
	  tmp1 = eatwhite(tmp1);
	  alias_add(p, tmp, tmp1);
	}
      }
      player_globals.parray[p].alias_list[i].comm_name = NULL;
    }
  } else if (!strcmp(attr, "num_censor:")) {
    i = atoi(value);
    while (i--) {
      fgets(tmp, MAX_LINE_SIZE, fp);
      if ((!(len = strlen(tmp))) || (len == 1)) { /* blank lines do occur!! */
        d_printf( "CHESSD: Error bad censor in file %s\n", file);
      } else {
        tmp[len - 1] = '\0';    /* Get rid of '\n' */
        list_add(p, L_CENSOR, tmp);
      }
    }
  } else if (!strcmp(attr, "num_notify:")) {
    i = atoi(value);
    while(i--) {
      fgets(tmp, MAX_LINE_SIZE, fp);
      if ((!(len = strlen(tmp))) || (len == 1)) { /* blank lines do occur!! */
        d_printf( "CHESSD: Error bad notify in file %s\n", file);
      } else {
        tmp[len - 1] = '\0';    /* Get rid of '\n' */
        list_add(p, L_NOTIFY, tmp);
      }
    }
  } else if (!strcmp(attr, "num_noplay:")) {
    i = atoi(value);
    while(i--) {
      fgets(tmp, MAX_LINE_SIZE, fp);
      if ((!(len = strlen(tmp))) || (len == 1)) { /* blank lines do occur!! */
        d_printf( "CHESSD: Error bad noplay in file %s\n", file);
      } else {
        tmp[len - 1] = '\0';    /* Get rid of '\n' */
        list_add(p, L_NOPLAY, tmp);
      }
    }
  } else if (!strcmp(attr, "num_gnotify:")) {
    i = atoi(value);
    while(i--) {
      fgets(tmp, MAX_LINE_SIZE, fp);
      if ((!(len = strlen(tmp)))  || (len == 1)) { /* blank lines do occur!! */
        d_printf( "CHESSD: Error bad gnotify in file %s\n", file);
      } else {
        tmp[len - 1] = '\0';    /* Get rid of '\n' */
        list_add(p, L_GNOTIFY, tmp);
      }
    }
  } else if (!strcmp(attr, "num_follow:")) {
    i = atoi(value);
    while(i--) {
      fgets(tmp, MAX_LINE_SIZE, fp);
      if ((!(len = strlen(tmp)))  || (len == 1)) { /* blank lines do occur!! */
        d_printf( "CHESSD: Error bad follow in file %s\n", file);
      } else {
        tmp[len - 1] = '\0';    /* Get rid of '\n' */
        list_add(p, L_FOLLOW, tmp);
      }
    }
  } else {
    d_printf( "CHESSD: Error bad attribute >%s< from file %s\n", attr, file);
  }
  return 0;
}




int player_read_old(int p, FILE *fp, const char *pname, const char *fname, int version)
{
	char line[MAX_LINE_SIZE];
	char *attr, *value;
	int len;

	if (version > 4) { /* eg if two version of code running at diff sites */
		pprintf(p, "Your account's version number is newer than that of the version running.\n"); 
		pprintf(p, "Please see an admin.\n");
		d_printf("Player %s's version number is incorrect for this version.\n",player_globals.parray[p].name);
		PFlagOFF(p, PFLAG_REG);
		player_globals.parray[p].name = strdup(pname);
		return -1;
	}
	if (version > 0) {
		if (!ReadV1PlayerFmt(p,&player_globals.parray[p], fp, fname, version)) {/* Quick method */
			/* error in reading; make unreg */
			PFlagOFF(p, PFLAG_REG);
			PFlagOFF(p, PFLAG_RATED);
			pprintf(p, "\n*** WARNING: Your Data file is corrupt. Please tell an admin ***\n");
			return -1; /* other data has already been freed */
		}
	} else {
		/* do it the old SLOW way */
		do {
			if (feof(fp))
				break;
			if ((len = strlen(line)) <= 1)
				continue;
			line[len - 1] = '\0';
			attr = eatwhite(line);
			if (attr[0] == '#')
				continue;			/* Comment */
			value = eatword(attr);
			if (!*value) {
				d_printf( "CHESSD: Error reading file %s\n", fname);
				continue;
			}
			*value = '\0';
			value++;
			value = eatwhite(value);
			stolower(attr);
			got_attr_value_player(p, attr, value, fp, fname);
			fgets(line, MAX_LINE_SIZE, fp);
		} while (!feof(fp));
	}
	
	if (!player_globals.parray[p].name) {
		player_globals.parray[p].name = strdup(pname);
		pprintf(p, "\n*** WARNING: Your Data file is corrupt. Please tell an admin ***\n");
	}
	
	return 0;
}


#if 0
/* the old and tedious way of writing the player data */
static void WritePlayerFile_old(FILE* fp, int p)
{
 int i;
 struct player *pp = &player_globals.parray[p];

 fprintf (fp, "v %d\n", 4);
 if (pp->name == NULL) /* This should never happen! */
   fprintf (fp,"NONE\n");
 else
   fprintf(fp, "%s\n", pp->name);
 if (pp->fullName == NULL)
   fprintf (fp,"NONE\n");
 else
   fprintf(fp, "%s\n", pp->fullName);
 if (pp->passwd == NULL)
   fprintf (fp,"NONE\n");
 else
   fprintf(fp, "%s\n", pp->passwd);
 if (pp->emailAddress == NULL)
   fprintf (fp,"NONE\n");
 else
   fprintf(fp, "%s\n", pp->emailAddress); 
 fprintf(fp, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %d\n",

 pp->s_stats.num, pp->s_stats.win, pp->s_stats.los, 
 pp->s_stats.dra, pp->s_stats.rating, (int) (pp->s_stats.sterr * 10.0),
 pp->s_stats.ltime, pp->s_stats.best, pp->s_stats.whenbest,

 pp->b_stats.num, pp->b_stats.win, pp->b_stats.los,
 pp->b_stats.dra, pp->b_stats.rating, (int) (pp->b_stats.sterr * 10.0),
 pp->b_stats.ltime, pp->b_stats.best, pp->b_stats.whenbest,

 pp->w_stats.num, pp->w_stats.win, pp->w_stats.los,
 pp->w_stats.dra, pp->w_stats.rating, (int) (pp->w_stats.sterr * 10.0),
 pp->w_stats.ltime, pp->w_stats.best, pp->w_stats.whenbest,

 pp->l_stats.num, pp->l_stats.win, pp->l_stats.los,
 pp->l_stats.dra, pp->l_stats.rating, (int) (pp->l_stats.sterr * 10.0),
 pp->l_stats.ltime, pp->l_stats.best, pp->l_stats.whenbest,

 pp->bug_stats.num, pp->bug_stats.win, pp->bug_stats.los,
 pp->bug_stats.dra, pp->bug_stats.rating, (int) (pp->bug_stats.sterr * 10.0),
 pp->bug_stats.ltime, pp->bug_stats.best, pp->bug_stats.whenbest,
 pp->lastHost);

 fprintf (fp, "%s\n", pp->prompt);
 fprintf (fp, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
 BoolCheckPFlag(p, PFLAG_OPEN), BoolCheckPFlag(p, PFLAG_RATED), 
 BoolCheckPFlag(p, PFLAG_ROPEN), pp->timeOfReg,
 pp->totalTime, BoolCheckPFlag(p, PFLAG_BELL),
 BoolCheckPFlag(p, PFLAG_PGN), BoolCheckPFlag(p, PFLAG_NOTIFYBY),
 BoolCheckPFlag(p, PFLAG_PIN), BoolCheckPFlag(p, PFLAG_GIN),
 BoolCheckPFlag(p, PFLAG_AVAIL), BoolCheckPFlag(p, PFLAG_SHOUT),
 BoolCheckPFlag(p, PFLAG_CSHOUT), BoolCheckPFlag(p, PFLAG_TELL),
 BoolCheckPFlag(p, PFLAG_KIBITZ), BoolCheckPFlag(p, PFLAG_PRIVATE),
 BoolCheckPFlag(p, PFLAG_JPRIVATE), BoolCheckPFlag(p, PFLAG_AUTOMAIL),
 BoolCheckPFlag(p, PFLAG_MAILMESS), pp->style,
 pp->d_time, pp->d_inc, pp->d_height, pp->d_width, pp->language,
 pp->adminLevel, pp->num_white, pp->num_black, pp->highlight,
 pp->num_comments,
 pp->num_plan, pp->num_formula,list_size(p, L_CENSOR),
 list_size(p, L_NOTIFY), list_size(p, L_NOPLAY),
 list_size(p, L_GNOTIFY), pp->numAlias, list_size(p, L_CHANNEL ), pp->availmin, pp->availmax);

  fprintf (fp,"."); /* Dot before plan - stop ill affects of scanf */

  for (i = 0; i < pp->num_plan; i++)
    fprintf(fp, "%s\n", (pp->planLines[i] ? pp->planLines[i] : ""));
  for (i = 0; i < pp->num_formula; i++)
    fprintf(fp, "%s\n", (pp->formulaLines[i] ? pp->formulaLines[i] : ""));
  if (player_globals.parray[p].formula != NULL)
    fprintf(fp, "%s\n", pp->formula);
  else
    fprintf(fp, "NONE\n");
  for (i = 0; i < pp->numAlias; i++)
    fprintf(fp, "%s %s\n", pp->alias_list[i].comm_name,
	    pp->alias_list[i].alias);

  list_print(fp, p, L_CENSOR);
  list_print(fp, p, L_NOTIFY);
  list_print(fp, p, L_NOPLAY);
  list_print(fp, p, L_GNOTIFY);
  list_print(fp, p, L_CHANNEL);
}
#endif

