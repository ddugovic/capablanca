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

static int on_channel(int ch, int p);

static int CheckShoutQuota(int p)
{
	struct player *pp = &player_globals.parray[p];
	int timeleft = time(0) - pp->lastshout_a;
	
	if ((timeleft < seek_globals.quota_time) && (pp->adminLevel == 0))
		return (seek_globals.quota_time - timeleft);

	return 0;
}

int com_shout(int p, param_list param)
{
  int p1, count = 0, timeleft = CheckShoutQuota(p); 	
  struct player *pp = &player_globals.parray[p];

  if (!CheckPFlag(p, PFLAG_REG)) {
    pprintf(p, "Only registered players can use the shout command.\n");
    return COM_OK;
  }
  if (in_list(p, L_MUZZLE, pp->login)) {
    pprintf(p, "You are muzzled.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    if (timeleft)
      pprintf(p, "Next shout available in %d seconds.\n", timeleft);
    else
      pprintf(p, "Your next shout is ready for use.\n");
    return COM_OK;
  }
  if (timeleft) {
    pprintf(p, "Shout not sent. Next shout in %d seconds.\n", timeleft);
    return COM_OK;
  }

  pp->lastshout_a = pp->lastshout_b;
  pp->lastshout_b = time(0);
  if (!printablestring(param[0].val.string)) {
    pprintf(p, "Your message contains some unprintable character(s).\n");
    return COM_OK;
  }

  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if ((p1 == p) || (player_globals.parray[p1].status != PLAYER_PROMPT) ||
	(!CheckPFlag(p1, PFLAG_SHOUT)) || (player_censored(p1, p)))
      continue;
    count++;
    pprintf_prompt(p1, "\n%s shouts: %s\n",pp->name,param[0].val.string);
  }

  pprintf(p, "(%d) %s shouts: %s\n", count, pp->name, 
	  param[0].val.string);

  if (!CheckPFlag(p, PFLAG_SHOUT))
    pprintf (p, "You are not listening to shouts.\n");
  if ((timeleft = CheckShoutQuota(p))) 
    pprintf(p, "Next shout in %d second(s).\n", timeleft);
  
  return COM_OK;
}

int com_cshout(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1, count = 0;

  if (!CheckPFlag(p, PFLAG_REG)) {
    pprintf(p, "Only registered players can use the cshout command.\n");
    return COM_OK;
  }
  if (in_list(p, L_CMUZZLE, pp->login)) {
    pprintf(p, "You are c-muzzled.\n");
    return COM_OK;
  }
  if (!printablestring(param[0].val.string)) {
    pprintf(p, "Your message contains some unprintable character(s).\n");
    return COM_OK;
  }

  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if ((p1 == p) || (player_globals.parray[p1].status != PLAYER_PROMPT) || 
	(!CheckPFlag(p1, PFLAG_CSHOUT)) || (player_censored(p1, p)))
      continue;
    count++;
    pprintf_prompt(p1, "\n%s c-shouts: %s\n", pp->name,
		   param[0].val.string);
  }
  pprintf(p, "(%d) %s c-shouts: %s\n", count, pp->name,
	  param[0].val.string);
  if (!CheckPFlag(p, PFLAG_CSHOUT))
    pprintf (p, "You are not listening to c-shouts.\n");
  
  return COM_OK;
}

int com_it(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1, count = 0, timeleft = CheckShoutQuota(p);

  if (!CheckPFlag(p, PFLAG_REG)) {
    pprintf(p, "Only registered players can use the it command.\n");
    return COM_OK;
  }
  if (in_list(p, L_MUZZLE, pp->login)) {
    pprintf(p, "You are muzzled.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    if (timeleft)
      pprintf(p, "Next shout available in %d seconds.\n", timeleft);
    else
      pprintf(p, "Your next shout is ready for use.\n");

    return COM_OK;
  }
  if (timeleft) {
    pprintf(p, "Shout not sent. Next shout in %d seconds.\n", timeleft);
    return COM_OK;
  }
  pp->lastshout_a = pp->lastshout_b;
  pp->lastshout_b = time(0);

  if (!printablestring(param[0].val.string)) {
    pprintf(p, "Your message contains some unprintable character(s).\n");
    return COM_OK;
  }

  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if ((p1 == p) || (player_globals.parray[p1].status != PLAYER_PROMPT) || 
	(!CheckPFlag(p1, PFLAG_SHOUT)) || (player_censored(p1, p)))
      continue;
    count++;
    if ((!strncmp(param[0].val.string, "\'", 1)) ||
	(!strncmp(param[0].val.string, ",", 1)) ||
	(!strncmp(param[0].val.string, ".", 1)))
      pprintf_prompt(p1, "\n--> %s%s\n", pp->name,
		     param[0].val.string);
    else
      pprintf_prompt(p1, "\n--> %s %s\n", pp->name,
		     param[0].val.string);
  }
  if ((!strncmp(param[0].val.string, "\'", 1)) ||
      (!strncmp(param[0].val.string, ",", 1)) ||
      (!strncmp(param[0].val.string, ".", 1))) 
    pprintf(p, "--> %s%s\n", pp->name, param[0].val.string);
  else
    pprintf(p, "--> %s %s\n", pp->name, param[0].val.string);
  
  if ((timeleft = CheckShoutQuota(p)))
    pprintf(p, "Next shout in %d second(s).\n", timeleft);

  return COM_OK;
}

static int tell(int p, int p1, const char *msg, int why, int ch)
{
  struct player *pp = &player_globals.parray[p];
  char tmp[MAX_LINE_SIZE];
  int rating, rating1, p2;

  if (!printablestring(msg)) {
    pprintf(p, "Your message contains some unprintable character(s).\n");
    return COM_OK;
  }

  if (!CheckPFlag(p1, PFLAG_TELL) && !CheckPFlag(p, PFLAG_REG)) {
    pprintf(p, "Player \"%s\" isn't listening to unregistered tells.\n",
	    player_globals.parray[p1].name);
    return COM_OK;
  }
  if (player_censored(p1, p)) {
    if (pp->adminLevel == 0) {
      if (why != TELL_KIBITZ && why != TELL_WHISPER && why != TELL_CHANNEL)
	pprintf(p, "Player \"%s\" is censoring you.\n", player_globals.parray[p1].name);
      return COM_OK;
    } else
      pprintf(p, "Told \"%s\", who is censoring you.\n", player_globals.parray[p1].name);
  }

  switch (why) {
  case TELL_SAY:
    pprintf_highlight(p1, "\n%s", pp->name);
    pprintf_prompt(p1, " says: %s\n", msg);
    break;
  case TELL_WHISPER:
  case TELL_KIBITZ:
    rating = GetRating(&player_globals.parray[p], TYPE_BLITZ);
    if (rating < (rating1 = (GetRating(&player_globals.parray[p], TYPE_STAND))))
      rating = rating1;
    if (in_list(p, L_FM, pp->name))
      pprintf(p1, "\n%s(FM)", pp->name);
    else if (in_list(p, L_IM, pp->name))
      pprintf(p1, "\n%s(IM)", pp->name);
    else if (in_list(p, L_GM, pp->name))
      pprintf(p1, "\n%s(GM)", pp->name);
    else if (in_list(p, L_WGM, pp->name))
      pprintf(p1, "\n%s(WGM)", pp->name);
    else if ((pp->adminLevel >= 10)
	     && CheckPFlag(p, PFLAG_ADMINLIGHT))
      pprintf(p1, "\n%s(*)", pp->name);
    else if ((rating >= player_globals.parray[p1].kiblevel) || 
             ((pp->adminLevel >= 10)
              && CheckPFlag(p, PFLAG_ADMINLIGHT)))
      if (!CheckPFlag(p, PFLAG_REG))
	pprintf(p1, "\n%s(++++)", pp->name);
      else if (rating != 0)
	if (in_list(p, L_COMPUTER, pp->name))
	  pprintf(p1, "\n%s(%d)(C)", pp->name, rating);
	else
	  pprintf(p1, "\n%s(%d)", pp->name, rating);
      else 
	pprintf(p1, "\n%s(----)", pp->name);
    else break;
    
    if (why == TELL_WHISPER) 
      pprintf_prompt(p1, " whispers: %s\n", msg);
    else 
      pprintf_prompt(p1, " kibitzes: %s\n", msg);

    break;
  case TELL_CHANNEL:
    pprintf(p1, "\n%s", pp->name);
    if ((pp->adminLevel >= 10)
        && CheckPFlag(p, PFLAG_ADMINLIGHT) && (ch == 1))
      pprintf(p1, "(*)");
    pprintf_prompt(p1, "(%d): %s\n", ch, msg);
    break;
  case TELL_LTELL:
      pprintf_prompt(p1, "\n%s tells you: %s\n", pp->name, msg);
      break;

  case TELL_TELL:
  default:
    if (player_globals.parray[p1].highlight)
      pprintf_highlight(p1, "\n%s", pp->name);
    else
      pprintf(p1, "\n%s", pp->name);
    
    if ((pp->adminLevel >= 10) && CheckPFlag(p, PFLAG_ADMINLIGHT))
      pprintf(p1,"(*)");
    
    pprintf_prompt(p1, " tells you: %s\n", msg);

/* let's test for forwarding of tells */

    if ((pp->ftell == p1) || (player_globals.parray[p1].ftell == p)) {
      for (p2 = 0; p2 < player_globals.p_num; p2++) {
        if ((p2 == p) || (p2 == p1) || (player_globals.parray[p2].status != PLAYER_PROMPT))
          continue;
        if (on_channel(0, p2))
          pprintf (p2,"\nFwd tell: %s told %s: %s\n", pp->name,
                 player_globals.parray[p1].name, msg);
      }
    }

    break;
  }
  
  tmp[0] = '\0';
  
  if (!(player_globals.parray[p1].busy == NULL))
    sprintf(tmp, ", who %s (idle: %s)", player_globals.parray[p1].busy, 
	    hms_desc(player_idle(p1)));
  else
    if (((player_idle(p1) % 3600) / 60) > 2)
      sprintf(tmp, ", who has been idle %s", hms_desc(player_idle(p1)));
  
  if ((why == TELL_SAY) || (why == TELL_TELL) || (why == TELL_LTELL)) {
    pprintf(p, "(told %s%s)\n", player_globals.parray[p1].name,
            (((player_globals.parray[p1].game>=0) && (game_globals.garray[player_globals.parray[p1].game].status == GAME_EXAMINE)) 
	     ? ", who is examining a game" : 
	     ((player_globals.parray[p1].game>=0) && (game_globals.garray[player_globals.parray[p1].game].status == GAME_SETUP))
	     ? ", who is setting up a position" :
	     (player_globals.parray[p1].game >= 0 && (player_globals.parray[p1].game != pp->game))
	     ? ", who is playing" : tmp));
  }

  if (why == TELL_TELL || why == TELL_SAY) {
	  FREE(pp->last_tell);
	  pp->last_tell = strdup(player_globals.parray[p1].login);
  }
  
  return COM_OK;
}

int com_ptell(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  char tmp[MAX_LINE_SIZE];
  int p1;

   if (pp->partner < 0) {
     pprintf (p, "You do not have a partner at present.\n");
     return COM_OK;
   }

   p1 = pp->partner;
   if ((p1 < 0) || (player_globals.parray[p1].status == PLAYER_PASSWORD)
       || (player_globals.parray[p1].status == PLAYER_LOGIN)) {
     pprintf(p, "Your partner is not logged in.\n");
     return COM_OK;
   }

   if (player_globals.parray[p1].highlight)
     pprintf_highlight(p1, "\n%s", pp->name);
   else
     pprintf(p1, "\n%s", pp->name);
   
   pprintf_prompt(p1, " (your partner) tells you: %s\n", param[0].val.string);
   tmp[0] = '\0';
   if (!(player_globals.parray[p1].busy == NULL))
     sprintf(tmp, ", who %s (idle: %s)", player_globals.parray[p1].busy,
	     hms_desc(player_idle(p1)));
   else
     if (((player_idle(p1) % 3600) / 60) > 2) 
       sprintf(tmp, ", who has been idle %s", hms_desc(player_idle(p1)));
   
   pprintf(p, "(told %s%s)\n", player_globals.parray[p1].name,
	   (((player_globals.parray[p1].game>=0) && (game_globals.garray[player_globals.parray[p1].game].status == GAME_EXAMINE))
            ? ", who is examining a game" :
            ((player_globals.parray[p1].game>=0) && (game_globals.garray[player_globals.parray[p1].game].status == GAME_SETUP))
            ? ", who is setting up a position" :
            (player_globals.parray[p1].game >= 0 && (player_globals.parray[p1].game != pp->game))
            ? ", who is playing" : tmp));
   
   return COM_OK;
}

static int chtell(int p, int ch, char *msg)
{
  struct player *pp = &player_globals.parray[p];
  int p1, count = 0;

  if ((ch == 0) && (pp->adminLevel < ADMIN_ADMIN)) {
    pprintf(p, "Only admins may send tells to channel 0.\n");
    return COM_OK;
  }

  if (ch < 0) {
    pprintf(p, "The lowest channel number is 0.\n");
    return COM_OK;
  }

  if (ch >= MAX_CHANNELS) {
    pprintf(p, "The maximum channel number is %d.\n", MAX_CHANNELS - 1);
    return COM_OK;
  }

  if ((ch == 1) && (in_list(p, L_C1MUZZLE, pp->login))) {
    pprintf (p, "You have been prevented from sending tells to channel 1.\n");
    return COM_OK;
  }

  if ((ch == 24) && (in_list(p, L_C24MUZZLE, pp->login))) {
    pprintf (p, "You have been prevented from sending tells to channel 24.\n");
    return COM_OK;
  }

  if ((ch == 46) && (in_list(p, L_C46MUZZLE, pp->login))) {
    pprintf (p, "You have been prevented from sending tells to channel 46.\n");
    return COM_OK;
  }

  if ((ch == 49) && (in_list(p, L_C49MUZZLE, pp->login))) {
    pprintf (p, "You have been prevented from sending tells to channel 49.\n");
    return COM_OK;
  }

  if ((ch == 50) && (in_list(p, L_C50MUZZLE, pp->login))) {
    pprintf (p, "You have been prevented from sending tells to channel 50.\n");
    return COM_OK;
  }

  if ((ch == 51) && (in_list(p, L_C51MUZZLE, pp->login))) {
    pprintf (p, "You have been prevented from sending tells to channel 51.\n");
    return COM_OK;
  }

  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if ((p1 == p) || (player_globals.parray[p1].status != PLAYER_PROMPT)) 
      continue;
    if ((on_channel(ch, p1)) && (!player_censored(p1, p))
        && (CheckPFlag(p, PFLAG_REG) || CheckPFlag(p1, PFLAG_TELL))) {
      tell(p, p1, msg, TELL_CHANNEL, ch);
      if (!player_censored(p1, p))
	count++;
    }
  }

  if (count) 
    pp->last_channel = ch;
  
  pprintf(p, "%s(%d): %s\n", pp->name, ch, msg);
  if (!on_channel(ch, p))
    pprintf(p, " (You're not listening to channel %d.)\n", ch);
  
  return COM_OK;
}

int com_whisper(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int g, p1, count = 0;

  if (!pp->num_observe && pp->game < 0) {
    pprintf(p, "You are not playing or observing a game.\n");
    return COM_OK;
  }
  if (!CheckPFlag(p, PFLAG_REG) && (pp->game == -1)) {
    pprintf(p, "You must be registered to whisper other people's games.\n");
    return COM_OK;
  }
  if (pp->game >= 0)
    g = pp->game;
  else
    g = pp->observe_list[0];
  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if ((p1 == p) || (player_globals.parray[p1].status != PLAYER_PROMPT))
      continue;
    if (player_is_observe(p1, g) ||
        (game_globals.garray[g].link >= 0 && player_is_observe(p1, game_globals.garray[g].link)
         && player_globals.parray[p1].game != game_globals.garray[g].link)) {
      tell(p, p1, param[0].val.string, TELL_WHISPER, 0);
      if ((pp->adminLevel >= ADMIN_ADMIN) || !game_globals.garray[g].private || !player_censored(p1, p))
	count++;
    }
  }
  pprintf(p, "whispered to %d.\n", count);
  return COM_OK;
}

int com_kibitz(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int g, p1, count = 0;

  if (!pp->num_observe && pp->game < 0) {
    pprintf(p, "You are not playing or observing a game.\n");
    return COM_OK;
  }
  if (!CheckPFlag(p, PFLAG_REG) && (pp->game == -1)) {
    pprintf(p, "You must be registered to kibitz other people's games.\n");
    return COM_OK;
  }
  if (pp->game >= 0)
    g = pp->game;
  else
    g = pp->observe_list[0];
  if(pp->game == g) { // [HGM] capture engine kibitz of player
    float score; int depth;
    if(sscanf(param[0].val.string, "!!! %f/%d", &score, &depth) == 2) {
      struct game *gg = &game_globals.garray[g];
      if(gg->numHalfMoves && gg->status == GAME_ACTIVE) {
	int n = gg->numHalfMoves-1;
	if(n && (gg->game_state.onMove == WHITE && gg->black != p ||
	         gg->game_state.onMove == BLACK && gg->white != p   )) n--;
	gg->moveList[n].score = score;
	gg->moveList[n].depth = depth;
      }
    }
  }
  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if ((p1 == p) || (player_globals.parray[p1].status != PLAYER_PROMPT))
      continue;
    if ((player_is_observe(p1, g) || player_globals.parray[p1].game == g
	 || (game_globals.garray[g].link >= 0
	     && (player_is_observe(p1, game_globals.garray[g].link)
		 || player_globals.parray[p1].game == game_globals.garray[g].link)))
        && CheckPFlag(p1, PFLAG_KIBITZ)) {
      tell(p, p1, param[0].val.string, TELL_KIBITZ, 0);
      if ((pp->adminLevel >= ADMIN_ADMIN) || 
	  !game_globals.garray[g].private || (player_globals.parray[p1].game == g) || !player_censored(p1, p))
	count++;
    }
  }
  pprintf(p, "kibitzed to %d.\n", count);
  return COM_OK;
}

/* 
   like tell() but takes a player name
*/
static int tell_s(int p, const char *who, const char *msg, int why, int ch)
{
	int p1;

	p1 = player_find_part_login(who);
	if ((p1 < 0) || (player_globals.parray[p1].status == PLAYER_PASSWORD)
	    || (player_globals.parray[p1].status == PLAYER_LOGIN)) {
		if (why != TELL_LTELL) {
			pprintf(p, "No user named '%s' is logged in.\n", who);
		}
		return COM_OK;
	}

	return tell(p, p1, msg, why, ch);
}

int com_tell(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	char *who;

	if (param[0].type == TYPE_NULL)
		return COM_BADPARAMETERS;
	if (param[0].type != TYPE_WORD) {
		return chtell(p, param[0].val.integer, param[1].val.string);
	}

	who = param[0].val.word;
	stolower(who);

	if (strcmp(who, ".") == 0) {
		if (!pp->last_tell) {
			pprintf(p, "No previous tell.\n");
			return COM_OK;
		}
		who = pp->last_tell;
	}

	if (strcmp(who, ",") == 0) {
		if (pp->last_channel == -1) {
			pprintf(p, "No previous channel.\n");
			return COM_OK;
		}
		return chtell(p, pp->last_channel, param[1].val.string);
	}

	return tell_s(p, who, param[1].val.string, TELL_TELL, 0);
}


/* tell all players in a named list */
int com_ltell(int p, param_list param)
{
	struct List *gl;
	int i;

	gl = list_findpartial(p, param[0].val.word, 0);
	if (!gl) {
		pprintf(p,"Can't find that list\n");
		return COM_OK;
	}
	
	for (i=0;i<gl->numMembers;i++) {
		tell_s(p, gl->m_member[i], param[1].val.string, TELL_LTELL, 0);
	}
	
	return COM_OK;
}


int com_xtell(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1;
  char *msg, tmp[2048];
  
  msg = param[1].val.string;
  p1 = player_find_part_login(param[0].val.word);
  if ((p1 < 0) || (player_globals.parray[p1].status == PLAYER_PASSWORD)
      || (player_globals.parray[p1].status == PLAYER_LOGIN)) {
    pprintf(p, "No user named \"%s\" is logged in.\n", param[0].val.word);
    return COM_OK;
  }
  if (!printablestring(msg)) {
    pprintf(p, "Your message contains some unprintable character(s).\n");
    return COM_OK;
  }
  if (!CheckPFlag(p1, PFLAG_TELL) && !CheckPFlag(p, PFLAG_REG)) {
    pprintf(p, "Player \"%s\" isn't listening to unregistered tells.\n",
	    player_globals.parray[p1].name);
    return COM_OK;
  }
  if ((player_censored(p1, p)) && (pp->adminLevel == 0)) {
    pprintf(p, "Player \"%s\" is censoring you.\n", player_globals.parray[p1].name);
    return COM_OK;
  }
  if (player_globals.parray[p1].highlight)
    pprintf_highlight(p1, "\n%s", pp->name);
  else
    pprintf(p1, "\n%s", pp->name);
  
  pprintf_prompt(p1, " tells you: %s\n", msg);

  tmp[0] = '\0';
  if (!(player_globals.parray[p1].busy == NULL))
    sprintf(tmp, ", who %s (idle: %s)", player_globals.parray[p1].busy,
	    hms_desc(player_idle(p1)));
  else
    if (((player_idle(p1) % 3600) / 60) > 2)
      sprintf(tmp, ", who has been idle %s", hms_desc(player_idle(p1)));
  
  pprintf(p, "(told %s%s)\n", player_globals.parray[p1].name,
          (((player_globals.parray[p1].game>=0) && (game_globals.garray[player_globals.parray[p1].game].status == GAME_EXAMINE)) 
	   ? ", who is examining a game" : 
	   ((player_globals.parray[p1].game>=0) && (game_globals.garray[player_globals.parray[p1].game].status == GAME_SETUP))
	   ? ", who is setting up a position" :
	   (player_globals.parray[p1].game >= 0 && (player_globals.parray[p1].game != pp->game))
	   ? ", who is playing" : tmp));
  return COM_OK;
}

int com_say(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	if (pp->opponent < 0) {
		if (!pp->last_opponent) {
			pprintf(p, "No one to say anything to, try tell.\n");
			return COM_OK;
		}
		return tell_s(p, pp->last_opponent, param[0].val.string, TELL_SAY, 0);
	}
	return tell(p, pp->opponent, param[0].val.string, TELL_SAY, 0);
}

int com_inchannel(int p, param_list param)
{
  int p1,count = 0;
  char tmp[18];

  if (param[0].type == TYPE_NULL) {
    pprintf (p,"inchannel [no params] has been removed\n");
    pprintf (p,"Please use inchannel [name] or inchannel [number]\n");
    return COM_OK;
  }

  if (param[0].type == TYPE_WORD) {
    p1 = player_find_part_login(param[0].val.word);
    if ((p1 < 0) || (player_globals.parray[p1].status != PLAYER_PROMPT)) {
      pprintf(p, "No user named \"%s\" is logged in.\n", param[0].val.word);
      return COM_OK;
    }
    pprintf (p,"%s is in the following channels:",player_globals.parray[p1].name); 
    if (list_channels (p,p1))
      pprintf (p," No channels found for %s.\n",player_globals.parray[p1].name);
    return COM_OK;
  } else if (param[0].val.integer >= MAX_CHANNELS) {
    pprintf (p, "No such channel; the largest channel number is %d.\n",
                 MAX_CHANNELS - 1);
    return COM_OK;
  } else {
    sprintf (tmp,"%d",param[0].val.integer);
    for (p1 = 0; p1 < player_globals.p_num; p1++) {
      if (player_globals.parray[p1].status != PLAYER_PROMPT)
	continue;
      if (in_list(p1,L_CHANNEL,tmp)) {
        if (!count)
          pprintf(p,"Channel %d:",param[0].val.integer);
        pprintf (p," %s", player_globals.parray[p1].name);
        if ((player_globals.parray[p1].adminLevel >= 10)
            && CheckPFlag(p1, PFLAG_ADMINLIGHT)
            && (param[0].val.integer < 2))
          pprintf(p, "(*)");
        count++;
      } 
    }
    if (!count)
      pprintf(p,"Channel %d is empty.\n",param[0].val.integer);
    else
      pprintf (p,"\n%d %s in channel %d.\n",count,(count == 1 ? "person is" : "people are"),param[0].val.integer);
    return COM_OK;
  }
}

static int com_sendmessage(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1, connected = 1;
  
  if (!CheckPFlag(p, PFLAG_REG)) {
    pprintf(p, "You are not registered and cannot send messages.\n");
    return COM_OK;
  }
  
  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;
  
  if (!CheckPFlag(p1, PFLAG_REG)) {
    pprintf(p, "Player \"%s\" is unregistered and cannot receive messages.\n",
            player_globals.parray[p1].name);
    return COM_OK;
  }
  
  if ((player_censored(p1, p)) && (pp->adminLevel == 0)) {
    pprintf(p, "Player \"%s\" is censoring you.\n", player_globals.parray[p1].name);
    if (!connected)
      player_remove(p1);
    return COM_OK;
  }
  if (player_add_message(p1, p, param[1].val.string)) {
    pprintf(p, "Couldn't send message to %s. Message buffer full.\n",
	    player_globals.parray[p1].name);
  } else {
    if (connected) {
      pprintf(p1, "\n%s just sent you a message:\n", pp->name);
      pprintf_prompt(p1, "    %s\n", param[1].val.string);
    } 
  }
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

int com_messages(int p, param_list param)
{
  int start;
  
  if (!CheckPFlag(p, PFLAG_REG)) {
    pprintf (p, "Unregistered players may not send or receive messages.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    player_show_messages (p);
  } else if (param[0].type == TYPE_WORD) {
    if (param[1].type != TYPE_NULL)
      return com_sendmessage(p, param);
    else ShowMsgsBySender(p, param);
  } else {
    start = param[0].val.integer;
    ShowMsgRange (p, start, start);
  }
  return COM_OK;
}

int com_forwardmess(int p, param_list param)
{
  if (!CheckPFlag(p, PFLAG_REG)) {
    pprintf (p, "Unregistered players may not send or receive messages.\n");
    return COM_OK;
  }
  
  ForwardMsgRange(param[0].val.word, p, param[1].val.integer, param[1].val.integer);

  return COM_OK;
}

int com_clearmessages(int p, param_list param)
{
  if (player_num_messages(p) <= 0) {
    pprintf(p, "You have no messages.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    pprintf(p, "Messages cleared.\n");
    player_clear_messages(p);
  } else if (param[0].type == TYPE_WORD)
    ClearMsgsBySender(p, param);
  else if (param[0].type == TYPE_INT)
    ClrMsgRange (p, param[0].val.integer, param[0].val.integer);

  return COM_OK;
}

int com_znotify(int p, param_list param)
{
  int p1, count = 0;
  
  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if (player_notified(p, p1) && player_globals.parray[p1].status == PLAYER_PROMPT) {
      if (!count)
	pprintf(p, "Present company on your notify list:\n  ");
      pprintf(p, " %s", player_globals.parray[p1].name);
      count++;
    }
  }
  if (!count)
    pprintf(p, "No one from your notify list is logged on.");
  pprintf(p, "\n");
  
  count = 0;
  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if (player_notified(p1, p) && player_globals.parray[p1].status == PLAYER_PROMPT) {
      if (!count)
	pprintf(p,
		"The following players have you on their notify list:\n  ");
      pprintf(p, " %s", player_globals.parray[p1].name);
      count++;
    }
  }
  if (!count)
    pprintf(p, "No one logged in has you on their notify list.");
  pprintf(p, "\n");

  return COM_OK;
}

int com_qtell(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1;
  char tmp[MAX_STRING_LENGTH];
  char dummy[2];
  char buffer1[MAX_STRING_LENGTH];	/* no highlight and no bell */
  char buffer2[MAX_STRING_LENGTH];	/* no highlight and bell */
  char buffer3[MAX_STRING_LENGTH];	/* highlight and no bell */
  char buffer4[MAX_STRING_LENGTH];	/* highlight and and bell */
  int i,count;
  
  if (!in_list(p, L_TD, pp->name)) {
    pprintf(p, "Only TD programs are allowed to use this command.\n");
    return COM_OK;
  }
  strcpy(buffer1, ":\0");
  strcpy(buffer2, ":\0");
  strcpy(buffer3, ":\0");
  strcpy(buffer4, ":\0");
  
  sprintf(tmp, "%s", param[1].val.string);
  for (i = 0, count = 0; ((tmp[i] != '\0') && (count < 1029));) {
    if ((tmp[i] == '\\') && (tmp[i + 1] == 'n')) {
      strcat(buffer1, "\n:");
      strcat(buffer2, "\n:");
      strcat(buffer3, "\n:");
      strcat(buffer4, "\n:");
      count += 2;
      i += 2;
    } else if ((tmp[i] == '\\') && (tmp[i + 1] == 'b')) {
      strcat(buffer2, "\007");
      strcat(buffer4, "\007");
      count++;
      i += 2;
    } else if ((tmp[i] == '\\') && (tmp[i + 1] == 'H')) {
      strcat(buffer3, "\033[7m");
      strcat(buffer4, "\033[7m");
      count += 4;
      i += 2;
    } else if ((tmp[i] == '\\') && (tmp[i + 1] == 'h')) {
      strcat(buffer3, "\033[0m");
      strcat(buffer4, "\033[0m");
      count += 4;
      i += 2;
    } else {
      dummy[0] = tmp[i];
      dummy[1] = '\0';
      strcat(buffer1, dummy);
      strcat(buffer2, dummy);
      strcat(buffer3, dummy);
      strcat(buffer4, dummy);
      count++;
      i++;
    }
  }

  if (param[0].type == TYPE_WORD) {
    if ((p1 = player_find_bylogin(param[0].val.word)) < 0) {
      pprintf(p, "*qtell %s 1*\n", param[0].val.word);
      return COM_OK;
    }
    pprintf_prompt(p1, "\n%s\n", (player_globals.parray[p1].highlight && CheckPFlag(p1, PFLAG_BELL)) ? buffer4 :
		   (player_globals.parray[p1].highlight && !CheckPFlag(p1, PFLAG_BELL)) ? buffer3 :
		   (!player_globals.parray[p1].highlight && CheckPFlag(p1, PFLAG_BELL)) ? buffer2 :
		   buffer1);
    pprintf(p, "*qtell %s 0*\n", player_globals.parray[p1].name);

  } else {
    int p1;
    int ch = param[0].val.integer;

    if (ch == 0) {
      pprintf(p, "*qtell %d 1*\n", param[0].val.integer);
      return COM_OK;
    }
    if (ch < 0) {
      pprintf(p, "*qtell %d 1*\n", param[0].val.integer);
      return COM_OK;
    }
    if (ch >= MAX_CHANNELS) {
      pprintf(p, "*qtell %d 1*\n", param[0].val.integer);
      return COM_OK;
    }
    sprintf (tmp,"%d",param[0].val.integer);
    for (p1 = 0; p1 < player_globals.p_num ; p1++) {
      if ((p1 == p) || (player_censored(p1, p)) || (player_globals.parray[p1].status != PLAYER_PROMPT))
	continue;
      if (in_list(p1,L_CHANNEL,tmp))
        pprintf_prompt(p1, "\n%s\n", (player_globals.parray[p1].highlight && CheckPFlag(p1, PFLAG_BELL)) ? buffer4 :
		     (player_globals.parray[p1].highlight && !CheckPFlag(p1, PFLAG_BELL)) ? buffer3 :
		     (!player_globals.parray[p1].highlight && CheckPFlag(p1, PFLAG_BELL)) ? buffer2 :
		     buffer1);
    }
    pprintf(p, "*qtell %d 0*\n", param[0].val.integer);
  }
  return COM_OK;
}

static int on_channel(int ch, int p)
{
	char tmp[10];  /* 9 digits ought to be enough :) */

	sprintf (tmp,"%d",ch);
	return in_list(p, L_CHANNEL,tmp);  /* since needs ch converted to a string
					      kept hidden from view */
}
