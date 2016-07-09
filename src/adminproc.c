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

/**
 *
 *     adminproc.c - All administrative commands and related functions
 *
 */

#include "includes.h"

#define PASSLEN 4


/*
  check that a player has sufficient rights for an operation
*/
int check_admin(int p, unsigned level)
{
	struct player *pp = &player_globals.parray[p];
	/* gods and head admins always get what they want */
	if (pp->adminLevel >= ADMIN_GOD || 
	    player_ishead(p)) {
		return 1;
	}
	return pp->adminLevel >= level;
}

/*
  check that player p1 can do an admin operation on player p2
*/
static int check_admin2(int p1, int p2)
{
	return check_admin(p1, player_globals.parray[p2].adminLevel+1);
}

/*
 * adjudicate
 *
 * Usage: adjudicate white_player black_player result
 *
 *   Adjudicates a saved (stored) game between white_player and black_player.
 *   The result is one of: abort, draw, white, black.  "Abort" cancels the game
 *   (no win, loss or draw), "white" gives white_player the win, "black" gives
 *   black_player the win, and "draw" gives a draw.
 */
int com_adjudicate(int p, param_list param)
{
  int wp, wconnected, bp, bconnected, g, inprogress, confused = 0;

  if (!FindPlayer(p, param[0].val.word, &wp, &wconnected))
    return COM_OK;
  if (!FindPlayer(p, param[1].val.word, &bp, &bconnected)) {
    if (!wconnected) 
     player_remove(wp);
    return COM_OK;
  }

  inprogress = ((player_globals.parray[wp].game >=0) &&(player_globals.parray[wp].opponent == bp));

  if (inprogress) {
    g = player_globals.parray[wp].game;
  } else {
    g = game_new();
    if (game_read(g, wp, bp) < 0) {
      confused = 1;
      pprintf(p, "There is no stored game %s vs. %s\n", player_globals.parray[wp].name, player_globals.parray[bp].name);
    } else {
      game_globals.garray[g].white = wp;
      game_globals.garray[g].black = bp;
    }
  }
  if (!confused) {
    if (strstr("abort", param[2].val.word) != NULL) {
      game_ended(g, WHITE, END_ADJABORT);

      pcommand(p, "message %s Your game \"%s vs. %s\" has been aborted.",
	       player_globals.parray[wp].name, player_globals.parray[wp].name, player_globals.parray[bp].name);

      pcommand(p, "message %s Your game \"%s vs. %s\" has been aborted.",
	       player_globals.parray[bp].name, player_globals.parray[wp].name, player_globals.parray[bp].name);
    } else if (strstr("draw", param[2].val.word) != NULL) {
      game_ended(g, WHITE, END_ADJDRAW);

      pcommand(p, "message %s Your game \"%s vs. %s\" has been adjudicated "
	       "as a draw", player_globals.parray[wp].name, player_globals.parray[wp].name, player_globals.parray[bp].name);

      pcommand(p, "message %s Your game \"%s vs. %s\" has been adjudicated "
	       "as a draw", player_globals.parray[bp].name, player_globals.parray[wp].name, player_globals.parray[bp].name);
    } else if (strstr("white", param[2].val.word) != NULL) {
      game_ended(g, WHITE, END_ADJWIN);

      pcommand(p, "message %s Your game \"%s vs. %s\" has been adjudicated "
	       "as a win", player_globals.parray[wp].name, player_globals.parray[wp].name, player_globals.parray[bp].name);

      pcommand(p, "message %s Your game \"%s vs. %s\" has been adjudicated "
	       "as a loss", player_globals.parray[bp].name, player_globals.parray[wp].name, player_globals.parray[bp].name);
    } else if (strstr("black", param[2].val.word) != NULL) {
      game_ended(g, BLACK, END_ADJWIN);
      pcommand(p, "message %s Your game \"%s vs. %s\" has been adjudicated "
	       "as a loss", player_globals.parray[wp].name, player_globals.parray[wp].name, player_globals.parray[bp].name);

      pcommand(p, "message %s Your game \"%s vs. %s\" has been adjudicated "
	       "as a win", player_globals.parray[bp].name, player_globals.parray[wp].name, player_globals.parray[bp].name);
    } else {
      confused = 1;
      pprintf(p, "Result must be one of: abort draw white black\n");
    }
  }
  if (!confused) {
    pprintf(p, "Game adjudicated.\n");
    if (!inprogress) {
      game_delete(wp, bp);
    } else {
      return (COM_OK);
    }
  }
  game_remove(g);
  if (!wconnected)
    player_remove(wp);
  if (!bconnected)
    player_remove(bp);
  return COM_OK;
}


/*
 * remplayer
 *
 * Usage:  remplayer name
 *
 *   Removes an account.  A copy of its files are saved under .rem.* which can
 *   be found in the appropriate directory (useful in case of an accident).
 *
 *   The account's details, messages, games and logons are all saved as
 *   'zombie' files.  These zombie accounts are not listed in handles or
 *   totals.
 */
int com_remplayer(int p, param_list param)
{
	char *player = param[0].val.word;
	char playerlower[MAX_LOGIN_NAME];
	int p1, lookup;

	strcpy(playerlower, player);
	stolower(playerlower);
	p1 = player_new();
	lookup = player_read(p1, playerlower);
	if (!lookup) {
		if (!check_admin2(p, p1)) {
			pprintf(p, "You can't remove an admin with a level higher than or equal to yourself.\n");
			player_remove(p1);
			return COM_OK;
		}
	}
	player_remove(p1);
	if (lookup) {
		pprintf(p, "No player by the name %s is registered.\n", player);
		return COM_OK;
	}
	if (player_find_bylogin(playerlower) >= 0) {
		pprintf(p, "A player by that name is logged in.\n");
		return COM_OK;
	}
	if (!player_kill(playerlower)) {
		pprintf(p, "Player %s removed.\n", player);
		UpdateRank(TYPE_BLITZ, NULL, NULL, player);
		UpdateRank(TYPE_STAND, NULL, NULL, player);
		UpdateRank(TYPE_WILD, NULL, NULL, player);
	} else {
		pprintf(p, "Remplayer failed.\n");
	}
	return COM_OK;
}

/*
 * raisedead
 *
 * Usage:  raisedead oldname [newname]
 *
 *   Restores an account that has been previously removed using "remplayer".
 *   The zombie files from which it came are removed.  Under most
 *   circumstances, you restore the account to the same handle it had
 *   before (oldname).  However, in some circumstances you may need to
 *   restore the account to a different handle, in which case you include
 *   "newname" as the new handle.  After "raisedead", you may need to use the
 *   "asetpasswd" command to get the player started again as a registered
 *   user, especially if the account had been locked
 *   by setting the password to *.
 */
int com_raisedead(int p, param_list param)
{
  char *player = param[0].val.word;
  char *newplayer = (param[1].type == TYPE_NULL  ?  player  :  param[1].val.word);
  char playerlower[MAX_LOGIN_NAME], newplayerlower[MAX_LOGIN_NAME];
  char plrFile[MAX_FILENAME_SIZE];

  int p2, lookup;

  strcpy(playerlower, player);
  stolower(playerlower);
  strcpy(newplayerlower, newplayer);
  stolower(newplayerlower);

  /* First make sure we have a player to raise. */
  sprintf (plrFile, PLAYER_DIR "/%c/.rem.%s", playerlower[0], playerlower);
  if (!file_exists (plrFile)) {
    pprintf(p, "No deleted player %s.\n", player);
    return COM_OK;
  }

  /* Now check for registered player. */
  sprintf (plrFile, PLAYER_DIR "/%c/%s", newplayerlower[0], newplayerlower);
  if (file_exists (plrFile)) {
    pprintf(p, "A player named %s is already registered.\n", newplayerlower);
    pprintf(p, "Obtain a new handle for the dead person.\n");
    pprintf(p, "Then use raisedead [oldname] [newname].\n");
    return COM_OK;
  }

  /* Don't raise over a logged in user. */
  if (player_find_bylogin(newplayerlower) >= 0) {
    pprintf(p, "A player named %s is logged in.\n", newplayerlower);
    pprintf(p, "Can't raise until that person leaves.\n");
    return COM_OK;
  }

  /* OK, ready to go. */
  if (!player_reincarn(playerlower, newplayerlower)) {
    if (param[1].type == TYPE_WORD)
      pprintf(p, "Player %s reincarnated to %s.\n", player, newplayer);
    else
      pprintf(p, "Player %s raised.\n", player);
    p2 = player_new();
    if (!(lookup = player_read(p2, newplayerlower))) {
      if (param[1].type == TYPE_WORD) {
        free(player_globals.parray[p2].name);
        player_globals.parray[p2].name = strdup(newplayer);
        player_save(p2);
      }
      if (player_globals.parray[p2].s_stats.rating > 0)
        UpdateRank(TYPE_STAND, newplayer, &player_globals.parray[p2].s_stats, newplayer);
      if (player_globals.parray[p2].b_stats.rating > 0)
        UpdateRank(TYPE_BLITZ, newplayer, &player_globals.parray[p2].b_stats, newplayer);
      if (player_globals.parray[p2].w_stats.rating > 0)
        UpdateRank(TYPE_WILD, newplayer, &player_globals.parray[p2].w_stats, newplayer);
    }
    player_remove(p2);
  } else {
    pprintf(p, "Raisedead failed.\n");
  }
  return COM_OK;
#if 0
 if (param[1].type == TYPE_NULL) {
    if (!player_raise(playerlower)) {
      pprintf(p, "Player %s raised from dead.\n", player);

      p1 = player_new();
      if (!(lookup = player_read(p1, playerlower))) {
	if (player_globals.parray[p1].s_stats.rating > 0)
	  UpdateRank(TYPE_STAND, player, &player_globals.parray[p1].s_stats, player);
	if (player_globals.parray[p1].b_stats.rating > 0)
	  UpdateRank(TYPE_BLITZ, player, &player_globals.parray[p1].b_stats, player);
	if (player_globals.parray[p1].w_stats.rating > 0)
	  UpdateRank(TYPE_WILD, player, &player_globals.parray[p1].w_stats, player);
      }
      player_remove(p1);
    } else {
      pprintf(p, "Raisedead failed.\n");
    }
    return COM_OK;
  } else {
    if (player_find_bylogin(newplayerlower) >= 0) {
      pprintf(p, "A player by the requested name is logged in.\n");
      pprintf(p, "Can't reincarnate until they leave.\n");
      return COM_OK;
    }
    p2 = player_new();
    lookup = player_read(p2, newplayerlower);
    player_remove(p2);
    if (!lookup) {
      pprintf(p, "A player by the name %s is already registered.\n", player);
      pprintf(p, "Obtain another new handle for the dead person.\n");
      return COM_OK;
    }
    if (!player_reincarn(playerlower, newplayerlower)) {
      pprintf(p, "Player %s reincarnated to %s.\n", player, newplayer);
      p2 = player_new();
      if (!(lookup = player_read(p2, newplayerlower))) {
	free(player_globals.parray[p2].name);
	player_globals.parray[p2].name = strdup(newplayer);
	player_save(p2);
	if (player_globals.parray[p2].s_stats.rating > 0)
	  UpdateRank(TYPE_STAND, newplayer, &player_globals.parray[p2].s_stats, newplayer);
	if (player_globals.parray[p2].b_stats.rating > 0)
	  UpdateRank(TYPE_BLITZ, newplayer, &player_globals.parray[p2].b_stats, newplayer);
	if (player_globals.parray[p2].w_stats.rating > 0)
	  UpdateRank(TYPE_WILD, newplayer, &player_globals.parray[p2].w_stats, newplayer);
      }
      player_remove(p2);
    } else {
      pprintf(p, "Raisedead failed.\n");
    }
  }
  return COM_OK;
#endif
}

/*
 * addplayer
 *
 * Usage: addplayer playername emailaddress realname
 *
 *   Adds a local player to the server with the handle of "playername".  For
 *   example:
 *
 *      addplayer Hawk u940456@daimi.aau.dk Henrik Gram
 */
int com_addplayer(int p, param_list param)
{
  char text[2048];
  char *newplayer = param[0].val.word;
  char *newname = param[2].val.string;
  char *newemail = param[1].val.word;
  char password[PASSLEN + 1];
  char newplayerlower[MAX_LOGIN_NAME];
  struct player *pp1;
  int p1, lookup;
  int i;

  if (strlen(newplayer) >= MAX_LOGIN_NAME) {
    pprintf(p, "Player name is too long\n");
    return COM_OK;
  }
  if (strlen(newplayer) < 3) {
    pprintf(p, "Player name is too short\n");
    return COM_OK;
  }
  if (!alphastring(newplayer)) {
    pprintf(p, "Illegal characters in player name. Only A-Za-z allowed.\n");
    return COM_OK;
  }
  strcpy(newplayerlower, newplayer);
  stolower(newplayerlower);
  p1 = player_new();
  lookup = player_read(p1, newplayerlower);
  if (!lookup) {
   pprintf(p, "A player by the name %s is already registered.\n", newplayerlower);
   player_remove(p1); 
   return COM_OK;
  }
  pp1 = &player_globals.parray[p1];
  pp1->name = strdup(newplayer);
  pp1->login = strdup(newplayerlower);
  pp1->fullName = strdup(newname);
  pp1->emailAddress = strdup(newemail);
  if (strcmp(newemail, "none")) {
    for (i = 0; i < PASSLEN; i++) {
      password[i] = 'a' + random() % 26;
    }
    password[i] = '\0';
    pp1->passwd = strdup(chessd_crypt(password, NULL));
  } else {
    password[0] = '\0';
    pp1->passwd = strdup(password);
  }
  PFlagON(p1, PFLAG_REG);
/*  pp1->network_player = 0; */
  PFlagON(p1, PFLAG_RATED);
  player_add_comment(p, p1, "Player added by addplayer.");
  player_save(p1);
  player_remove(p1);
  pprintf(p, "Added: >%s< >%s< >%s< >%s<\n", newplayer, newname, newemail, password);
  if (strcmp(newemail, "none")) {
/*
    sprintf(text, "\nYou have been added as a local player.\n\nLogin Name: "
	    "%s\nFull Name: %s\nEmail Address: %s\nInitial Password: "
	    "%s\n\nIf any of this information is incorrect, please "
	    "contact the administrator\nto get it corrected.\n\n"
	"Please write down your password.\n\nRegards,\n\nThe FICS admins\n",
	    newplayer, newname, newemail, password);
*/

   sprintf(text, "\nYour player account has been created.\n\n"
   "Login Name: %s\nFull Name: %s\nEmail Address: %s\nInitial Password: %s\n\n"
   "If any of this information is incorrect, please contact the administrator\n"
   "to get it corrected.\n\n"
   "You may change your password with the password command on the the server.\n"
   "\nPlease be advised that if this is an unauthorized duplicate account for\n"
   "you, by using it you take the risk of being banned from accessing this\n"
   "chess server.\n\n"        
   "Regards,\n\nThe FICS admins\n",
        newplayer, newname, newemail, password);

    mail_string_to_address(newemail, "FICS Account Created", text);
    if ((p1 = player_find_part_login(newplayer)) >= 0) {
      pprintf_prompt(p1, "\n\nYou are now registered! Confirmation together with\npassword is sent to your email address.\n\n");
      player_read(p1, newplayer);
      return COM_OK;
    }
    return COM_OK;
  } else {
    if ((p1 = player_find_part_login(newplayer)) >= 0) {
      pprintf_prompt(p1, "\n\nYou are now registered! Your have NO password!\n\n");
      player_read(p1, newplayer);
      return COM_OK;
    }
  }
  return COM_OK;
}

int com_pose(int p, param_list param)
{
	int p1;

	if ((p1 = player_find_part_login(param[0].val.word)) < 0) {
		pprintf(p, "%s is not logged in.\n", param[0].val.word);
		return COM_OK;
	}
	if (!check_admin2(p, p1)) {
		pprintf(p, "You can only pose as players below your adminlevel.\n");
		return COM_OK;
	}
	pprintf(p, "Command issued as %s\n", player_globals.parray[p1].name);
	pcommand(p1, "%s\n", param[1].val.string);
	return COM_OK;
}

/*
 * asetv
 *
 * Usage: asetv user instructions
 *
 *   This command executes "set" instructions as if they had been made by the
 *   user indicated.  For example, "asetv DAV shout 0" would set DAV's shout
 *   variable to 0.
 */
int com_asetv(int p, param_list param)
{
	int p1;
	
	if ((p1 = player_find_part_login(param[0].val.word)) < 0) {
		pprintf(p, "%s is not logged in.\n", param[0].val.word);
		return COM_OK;
	}
	if (!check_admin2(p, p1)) {
		pprintf(p, "You can only aset players below your adminlevel.\n");
		return COM_OK;
	}
	pprintf(p, "Command issued as %s\n", player_globals.parray[p1].name);
	pcommand(p1, "set %s\n", param[1].val.string);
	return COM_OK;
}

/*
 * announce
 *
 * Usage: announce message
 *
 *   Broadcasts your message to all logged on users.  Announcements reach all
 *   users and cannot be censored in any way (such as by "set shout 0").
 */
int com_announce(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	int p1;
	int count = 0;
	
	if (!printablestring(param[0].val.string)) {
		pprintf(p, "Your message contains some unprintable character(s).\n");
		return COM_OK;
	}
	for (p1 = 0; p1 < player_globals.p_num; p1++) {
		if (p1 == p)
			continue;
		if (player_globals.parray[p1].status != PLAYER_PROMPT)
			continue;
		count++;
		pprintf_prompt(p1, "\n\n    **ANNOUNCEMENT** from %s: %s\n\n", pp->name, param[0].val.string);
	}
	pprintf(p, "\n(%d) **ANNOUNCEMENT** from %s: %s\n\n", count, pp->name, param[0].val.string);
	return COM_OK;
}

/*
 * annunreg
 *
 * Usage:  annunreg message
 *
 *   Broadcasts your message to all logged on unregistered users, and admins,
 *   too.  Announcements reach all unregistered users and admins and cannot be
 *   censored in any way (such as by "set shout 0").
 */
int com_annunreg(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	int p1;
	int count = 0;

	if (!printablestring(param[0].val.string)) {
		pprintf(p, "Your message contains some unprintable character(s).\n");
		return COM_OK;
	}
	for (p1 = 0; p1 < player_globals.p_num; p1++) {
		if (p1 == p) continue;
		if (player_globals.parray[p1].status != PLAYER_PROMPT) continue;
		if (CheckPFlag(p1, PFLAG_REG)
		    && !check_admin(p1, ADMIN_ADMIN))
			continue;
		count++;
		pprintf_prompt(p1, "\n\n    **UNREG ANNOUNCEMENT** from %s: %s\n\n", 
			       pp->name, param[0].val.string);
	}
	pprintf(p, "\n(%d) **UNREG ANNOUNCEMENT** from %s: %s\n\n",
		count, pp->name, param[0].val.string);
	return COM_OK;
}

/*
 * asetpasswd
 *
 * Usage: asetpasswd player {password,*}
 *
 *   This command sets the password of the player to the password given.
 *   If '*' is specified then the player's account is locked, and no password
 *   will work until a new one is set by asetpasswd.
 *
 *   If the player is connected, he is told of the new password and the name
 *   of the admin who changed it, or likewise of his account status.  An
 *   email message is mailed to the player's email address as well.
 */
int com_asetpasswd(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  struct player *pp1;
  int p1, connected;
  char subject[400], text[10100];

  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  if (!check_admin2(p, p1)) {
    pprintf(p, "You can only set password for players below your adminlevel.\n");
    if (!connected)
      player_remove(p1);
    return COM_OK;
  }
  if (!CheckPFlag(p1, PFLAG_REG)) {
    pprintf(p, "You cannot set the password of an unregistered player!\n");
    return COM_OK;
  }
  pp1 = &player_globals.parray[p1];
  if (pp1->passwd)
    free(pp1->passwd);
  if (param[1].val.word[0] == '*') {
    pp1->passwd = strdup(param[1].val.word);
    pprintf(p, "Account %s locked!\n", pp1->name);
    sprintf(text, "Password of %s is now useless.  Your account at our"
                  " FICS has been locked.\n", pp1->name);
      pprintf(p, "Please leave a comment to explain why %s's account"
                 " was locked.\n", pp1->name);
      pcommand(p, "addcomment %s Account locked.\n", pp1->name);
  } else {
    pp1->passwd = strdup(chessd_crypt(param[1].val.word, NULL));
    sprintf(text, "Password of %s changed to \"%s\".\n", pp1->name, param[1].val.word);
    pprintf(p, "%s", text);
  }
  if (param[1].val.word[0] == '*') {
    sprintf(subject, "CHESSD: %s has locked your account.", pp->name);
    if (connected)
      pprintf_prompt(p1, "\n%s\n", subject);
  } else {
    sprintf(subject, "CHESSD: %s has changed your password.", pp->name);
    if (connected)
      pprintf_prompt(p1, "\n%s\n", subject);
  }
  mail_string_to_address(pp1->emailAddress, subject, text);
  player_save(p1);
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/*
 * asetemail
 *
 * Usage: asetemail player [address]
 *
 *   Sets the email address of the player to the address given.  If the
 *   address is omited, then the player's email address is cleared.  The
 *   person's email address is revealed to them when they use the "finger"
 *   command, but no other users -- except admins -- will have another
 *   player's email address displayed.
 */
int com_asetemail(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1, connected;
  char *oldemail;

  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  if (!check_admin2(p, p1)) {
    pprintf(p, "You can only set email addr for players below your adminlevel.\n");
    if (!connected)
      player_remove(p1);
    return COM_OK;
  }
  if (player_globals.parray[p1].emailAddress) {
    oldemail = strdup(player_globals.parray[p1].emailAddress);
    free(player_globals.parray[p1].emailAddress);
  } else {
    oldemail = strdup("");
  }
    
  if (param[1].type == TYPE_NULL) {
    player_globals.parray[p1].emailAddress = NULL;
    pprintf(p, "Email address for %s removed\n", player_globals.parray[p1].name);
    pcommand(p, "addcomment %s Email address removed.\n", player_globals.parray[p1].name);
    
  } else {
    player_globals.parray[p1].emailAddress = strdup(param[1].val.word);
    pprintf(p, "Email address of %s changed to \"%s\".\n", player_globals.parray[p1].name, param[1].val.word);
    pcommand(p, "addcomment %s Email address changed from %s to %s.\n", player_globals.parray[p1].name, oldemail, player_globals.parray[p1].emailAddress);
  }
  free(oldemail);
  player_save(p1);
  if (connected) {
    if (param[1].type == TYPE_NULL) {
      pprintf_prompt(p1, "\n\n%s has removed your email address.\n\n", pp->name);
    } else {
      pprintf_prompt(p1, "\n\n%s has changed your email address.\n\n", pp->name);
    }
  } else {
    player_remove(p1);
  }
  return COM_OK;
}

/*
 * asetrealname
 *
 * Usage:  asetrealname user newname
 *
 *   This command sets the user's real name (as displayed to admins on finger
 *   notes) to "newname".
 */
int com_asetrealname(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1, connected;

  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  if (!check_admin2(p, p1)) {
    pprintf(p, "You can only set real names for players below your adminlevel.\n");
    if (!connected)
      player_remove(p1);
    return COM_OK;
  }
  if (player_globals.parray[p1].fullName)
    free(player_globals.parray[p1].fullName);
  if (param[1].type == TYPE_NULL) {
    player_globals.parray[p1].fullName = NULL;
    pprintf(p, "Real name for %s removed\n", player_globals.parray[p1].name);
  } else {
    player_globals.parray[p1].fullName = strdup(param[1].val.word);
    pprintf(p, "Real name of %s changed to \"%s\".\n", player_globals.parray[p1].name, param[1].val.word);
  }
  player_save(p1);
  if (connected) {
    if (param[1].type == TYPE_NULL) {
      pprintf_prompt(p1, "\n\n%s has removed your real name.\n\n", pp->name);
    } else {
      pprintf_prompt(p1, "\n\n%s has changed your real name.\n\n", pp->name);
    }
  } else {
    player_remove(p1);
  }
  return COM_OK;
}

/*
 * asethandle
 *
 * Usage: asethandle oldname newname
 *
 *   This command changes the handle of the player from oldname to
 *   newname.  The various player information, messages, logins, comments
 *   and games should be automatically transferred to the new account.
 */
int com_asethandle(int p, param_list param)
{
  char *player = param[0].val.word;
  char *newplayer = param[1].val.word;
  char playerlower[MAX_LOGIN_NAME], newplayerlower[MAX_LOGIN_NAME];
  int p1;

  strcpy(playerlower, player);
  stolower(playerlower);
  strcpy(newplayerlower, newplayer);
  stolower(newplayerlower);
  if (player_find_bylogin(playerlower) >= 0) {
    pprintf(p, "A player by that name is logged in.\n");
    return COM_OK;
  }
  if (player_find_bylogin(newplayerlower) >= 0) {
    pprintf(p, "A player by that new name is logged in.\n");
    return COM_OK;
  }
  p1 = player_new();
  if (player_read(p1, playerlower)) {
    pprintf(p, "No player by the name %s is registered.\n", player);
    player_remove(p1);
    return COM_OK;
  } else {
	  if (!check_admin2(p, p1)) {
		  pprintf(p, "You can't set handles for an admin with a level higher than or equal to yourself.\n");
		  player_remove(p1);
		  return COM_OK;
	  }
  }
  player_remove(p1);

  p1 = player_new();
  if ((!player_read(p1, newplayerlower)) && (strcmp(playerlower, newplayerlower))) {
    pprintf(p, "Sorry that handle is already taken.\n");
    player_remove(p1);
    return COM_OK;
  }
  player_remove(p1);

  if ((!player_rename(playerlower, newplayerlower)) && (!player_read(p1, newplayerlower))) {
    pprintf(p, "Player %s renamed to %s.\n", player, newplayer);
    free(player_globals.parray[p1].name);
    player_globals.parray[p1].name = strdup(newplayer);
    player_save(p1);
    if (player_globals.parray[p1].s_stats.rating > 0)
      UpdateRank(TYPE_STAND, newplayer, &player_globals.parray[p1].s_stats, player);
    if (player_globals.parray[p1].b_stats.rating > 0)
      UpdateRank(TYPE_BLITZ, newplayer, &player_globals.parray[p1].b_stats, player);
    if (player_globals.parray[p1].w_stats.rating > 0)
      UpdateRank(TYPE_WILD, newplayer, &player_globals.parray[p1].w_stats, player);
  } else {
    pprintf(p, "Asethandle failed.\n");
  }
  player_remove(p1);
  return COM_OK;
}

/*
 * asetadmin
 *
 * Usage: asetadmin player AdminLevel
 *
 *   Sets the admin level of the player with the following restrictions.
 *   1. You can only set the admin level of players lower than yourself.
 *   2. You can only set the admin level to a level that is lower than
 *      yourself.
 */
int com_asetadmin(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1, connected, oldlevel;

  if (!FindPlayer(p, param[0].val.word,&p1, &connected))
    return COM_OK;

  if (!check_admin2(p, p1)) {
    pprintf(p, "You can only set adminlevel for players below your adminlevel.\n");
    if (!connected)
      player_remove(p1);
    return COM_OK;
  }
  if (!strcmp(player_globals.parray[p1].login, pp->login)) {
    pprintf(p, "You can't change your own adminlevel.\n");
    return COM_OK;
  }
  if (!check_admin(p, param[1].val.integer+1)) {
    pprintf(p, "You can't promote someone to or above your adminlevel.\n");
    if (!connected)
      player_remove(p1);
    return COM_OK;
  }
  oldlevel = player_globals.parray[p1].adminLevel;
  player_globals.parray[p1].adminLevel = param[1].val.integer;
  pprintf(p, "Admin level of %s set to %d.\n", player_globals.parray[p1].name, player_globals.parray[p1].adminLevel);
  player_save(p1);
  if (connected) {
    pprintf_prompt(p1, "\n\n%s has set your admin level to %d.\n\n", pp->name, player_globals.parray[p1].adminLevel);
  } else {
    player_remove(p1);
  }
  return COM_OK;
}

static void SetRating(int p1, param_list param, struct statistics *s)
{
  s->rating = param[1].val.integer;
  if (s->ltime == 0L)
    s->sterr = 70.0;

  if (param[2].type == TYPE_INT) {
    s->win = param[2].val.integer;
    if (param[3].type == TYPE_INT) {
      s->los = param[3].val.integer;
      if (param[4].type == TYPE_INT) {
	s->dra = param[4].val.integer;
	if (param[5].type == TYPE_INT) {
	  s->sterr = (double) param[5].val.integer;
	}
      }
    }
  }
  s->num = s->win + s->los + s->dra;
  if (s->num == 0) {
    s->ltime = 0L;
#if 0
    s->dra = 1;
    s->num = 1;
#endif
  } else {
    s->ltime = time(0);
  }
}

/*
 * asetblitz
 *
 * Usage: asetblitz handle rating won lost drew RD
 *
 *   This command allows admins to set a user's statistics for Blitz games.
 *   The parameters are self-explanatory: rating, # of wins, # of losses,
 *   # of draws, and ratings deviation.
 */
int com_asetblitz(int p, param_list param)
{
  int p1, connected;

  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  if (CheckPFlag(p1, PFLAG_REG)) {
      SetRating(p1, param, &player_globals.parray[p1].b_stats);
      player_save(p1);
      UpdateRank(TYPE_BLITZ, player_globals.parray[p1].name, &player_globals.parray[p1].b_stats,
	     player_globals.parray[p1].name);
  } else 
    pprintf(p, "%s is unregistered. Can't modify rating.\n", player_globals.parray[p1].name);

  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/*
 * asetwild
 *
 * Usage: asetwild handle rating won lost drew RD
 *
 *   This command allows admins to set a user's statistics for Wild games.
 *   The parameters are self-explanatory: rating, # of wins, # of losses,
 *   # of draws, and ratings deviation.
 */
int com_asetwild(int p, param_list param)
{
  int p1, connected;

  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  if (CheckPFlag(p1, PFLAG_REG)) {
    SetRating(p1, param, &player_globals.parray[p1].w_stats);
    player_save(p1);
    UpdateRank(TYPE_WILD, player_globals.parray[p1].name, &player_globals.parray[p1].w_stats,
	     player_globals.parray[p1].name);
    pprintf(p, "Wild rating for %s modified.\n", player_globals.parray[p1].name);
  } else 
    pprintf(p, "%s is unregistered. Can't modify rating.\n", player_globals.parray[p1].name);

  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/*
 * asetstd
 *
 * Usage: asetstd handle rating won lost drew RD
 *
 *   This command allows admins to set a user's statistics for Standard games.
 *   The parameters are self-explanatory: rating, # of wins, # of losses, # of
 *   draws, and ratings deviation.
 */
int com_asetstd(int p, param_list param)
{
  int p1, connected;

  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  if (CheckPFlag(p1, PFLAG_REG)) {
    SetRating(p1, param, &player_globals.parray[p1].s_stats);
    player_save(p1);
    UpdateRank(TYPE_STAND, player_globals.parray[p1].name, &player_globals.parray[p1].s_stats,
	     player_globals.parray[p1].name);
    pprintf(p, "Standard rating for %s modified.\n", player_globals.parray[p1].name);
  } else
    pprintf(p, "%s is unregistered. Can't modify rating.\n", player_globals.parray[p1].name);

  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/*
 * asetlight
 *
 * Usage: asetlight handle rating won lost drew RD
 *
 *   This command allows admins to set a user's statistics for Lightning games.
 *   The parameters are self-explanatory: rating, # of wins, # of losses, # of
 *   draws, and ratings deviation.
 */
int com_asetlight(int p, param_list param)
{
  int p1, connected;

  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  if (CheckPFlag(p1, PFLAG_REG)) {
    SetRating(p1, param, &player_globals.parray[p1].l_stats);
    player_save(p1);
    pprintf(p, "Lightning rating for %s modified.\n", player_globals.parray[p1].name);
  } else
    pprintf(p, "%s is unregistered. Can't modify rating.\n", player_globals.parray[p1].name);

  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/*
 * asetbug
 *
 * Usage: asetbug handle rating won lost drew RD
 *
 *   This command allows admins to set a user's statistics for Bughouse
 *   games.  The parameters are self-explanatory: rating, # of wins,
 *   # of losses, # of draws, and ratings deviation.
 */
int com_asetbug(int p, param_list param)
{
  int p1, connected;

  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  if (CheckPFlag(p1, PFLAG_REG)) {
    SetRating(p1, param, &player_globals.parray[p1].bug_stats);
    player_save(p1);
    pprintf(p, "Bughouse rating for %s modified.\n", player_globals.parray[p1].name);
  } else
    pprintf(p, "%s is unregistered. Can't modify rating.\n", player_globals.parray[p1].name);

  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/* ftell
 *
 * Usage: ftell user
 *
 *   This command forwards all further conversation between an admin and a
 *   user to all those listening in channel 0.
 *   It is unset as soon as the user logs off or ftell is typed without a
 *   parameter.
 */

int com_ftell(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1;
  char command[1024];

  if (param[0].type == TYPE_WORD) {

    if ((p1 = player_find_part_login(param[0].val.word)) < 0) {
      pprintf(p, "%s isn't logged in.\n", param[0].val.word);
      return COM_OK;
    }

    if (p1 == p) {
       pprintf (p, "Nobody wants to listen to you talking to yourself! :-)\n");
       return COM_OK;
    }

    if (pp->ftell != -1) {
      sprintf (command, "tell 0 I will no longer be forwarding the conversation between *%s* and myself.", player_globals.parray[pp->ftell].name);
      pcommand (p,command);
    } 

    sprintf (command, "tell 0 I will be forwarding the conversation between *%s* and myself to channel 0.", player_globals.parray[p1].name);
    pcommand (p,command);

    pp->ftell = p1;
    return COM_OK_NOPROMPT;

  } else {

      if (pp->ftell != -1) {

        pprintf (p,"Stopping the forwarding of the conservation with %s.\n",
                player_globals.parray[pp->ftell].name);
        pcommand (p,"tell 0 I will no longer be forwarding the conversation between *%s* and myself.",
                 player_globals.parray[pp->ftell].name);

        pp->ftell = -1;
        return COM_OK_NOPROMPT;
      } else
        pprintf (p,"You were not forwarding a conversation.\n");
  }

  return COM_OK;
}

/*
 * nuke
 *
 * Usage: nuke user
 *
 *   This command disconnects the user from the server.  The user is informed
 *   that she/he has been nuked by the admin named and a comment is
 *   automatically placed in the user's files (if she/he is a registered
 *   user, of course).
 */
int com_nuke(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	int p1, fd;
	
	if ((p1 = player_find_part_login(param[0].val.word)) < 0) {
		pprintf(p, "%s isn't logged in.\n", param[0].val.word);
		return COM_OK;
	}

	if (!check_admin2(p, p1)) {
		pprintf(p, "You need a higher adminlevel to nuke %s!\n", param[0].val.word);
		return COM_OK;
	}

	pprintf(p, "Nuking: %s\n", param[0].val.word);
	pprintf(p, "Please leave a comment explaining why %s was nuked.\n", player_globals.parray[p1].name);
	pprintf(p1, "\n\n**** You have been kicked out by %s! ****\n\n", pp->name);
	pcommand(p, "addcomment %s Nuked\n", player_globals.parray[p1].name);
	fd = player_globals.parray[p1].socket;
	process_disconnection(fd);
	net_close_connection(fd);
	return COM_OK;
}

/*
 * summon
 *
 * Usage: summon player
 *
 *   This command gives a beep and a message to the player indicating that you
 *   want to talk with him/her.  The command is useful for waking someone up,
 *   for example a sleepy admin or an ignorant player.
 */
int com_summon(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	int p1;
	
	if ((p1 = player_find_part_login(param[0].val.word)) < 0) {
		pprintf(p, "%s isn't logged in.\n", param[0].val.word);
		return COM_OK;
	}

	pprintf(p1, "\a\n");
	pprintf_highlight(p1, "%s", pp->name);
	pprintf_prompt(p1, " needs to talk with you.  Use tell %s <message>  to reply.\a\n", pp->name);
	pprintf(p, "Summoning sent to %s.\n", player_globals.parray[p1].name);
	return COM_OK;
}

/*
 * addcomment
 *
 * Usage: addcomment user comment
 *
 *   Places "comment" in the user's comments.  If a user has comments, the
 *   number of comments is indicated to admins using the "finger" command.
 *   The comments themselves are displayed by the "showcomments" command.
 */
int com_addcomment(int p, param_list param)
{
  int p1, connected;

  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  if (player_add_comment(p, p1, param[1].val.string)) {
    pprintf(p, "Error adding comment!\n");
  } else {
    pprintf(p, "Comment added for %s.\n", player_globals.parray[p1].name);
    player_save(p1);
  }
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/*
 * showcomment
 *
 * Usage: showcomment user
 *
 *   This command will display all of the comments added to the user's account.
 */
int com_showcomment(int p, param_list param)
{
  int p1, connected;

  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;
  player_show_comments(p, p1);
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/*
 * admin
 *
 * Usage: admin
 *
 *   This command toggles your admin symbol (*) on/off.  This symbol appears
 *   in who listings.
 */
int com_admin(int p, param_list param)
{
  TogglePFlag(p, PFLAG_ADMINLIGHT);
  if (CheckPFlag(p, PFLAG_ADMINLIGHT)) {
    pprintf(p, "Admin mode (*) is now shown.\n");
  } else {
    pprintf(p, "Admin mode (*) is now not shown.\n");
  }
  return COM_OK;
}

int com_hideinfo(int p, param_list param)
{
	TogglePFlag(p, PFLAG_HIDEINFO);
 
	if (CheckPFlag(p, PFLAG_HIDEINFO))
		pprintf(p, "Private user information now not shown.\n");
	else
		pprintf(p, "Private user information now shown.\n");
	
	return COM_OK;
}

/*
 * quota
 *
 * Usage: quota [n]
 *
 *   The command sets the number of seconds (n) for the shout quota, which
 *   affects only those persons on the shout quota list.  If no parameter
 *   (n) is given, the current setting is displayed.
 */
int com_quota(int p, param_list param)
{
  if (param[0].type == TYPE_NULL) {
    pprintf(p, "The current shout quota is 2 shouts per %d seconds.\n", seek_globals.quota_time);
    return COM_OK;
  }
  seek_globals.quota_time = param[0].val.integer;
  pprintf(p, "The shout quota is now 2 shouts per %d seconds.\n", seek_globals.quota_time);
  return COM_OK;
}


/* 
   force a server reload
*/
int com_areload(int p, param_list param)
{
	extern unsigned chessd_reload_flag;

	chessd_reload_flag = 1;
	
	pprintf(p, "Server reload started\n");

	return COM_OK;
}
