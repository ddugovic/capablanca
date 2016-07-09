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
#include "command_list.h"

const char *usage_dir[NUM_LANGS] = {USAGE_DIR, USAGE_SPANISH, 
					   USAGE_FRENCH, USAGE_DANISH};

static int lastCommandFound = -1;

static char *guest_name(void);
static int check_user(char *user);

/* Copies command into comm, and returns pointer to parameters in
 * parameters
 */
static int parse_command(char *com_string,
			   char **comm,
			   char **parameters)
{
	*comm = com_string;
	*parameters = eatword(com_string);
	if (**parameters != '\0') {
		**parameters = '\0';
		(*parameters)++;
		*parameters = eatwhite(*parameters);
	}
	if (strlen(*comm) >= MAX_COM_LENGTH) {
		return COM_BADCOMMAND;
	}
	return COM_OK;
}

/* numalias is the maximum number to search through */
int alias_lookup(char *tmp, struct alias_type *alias_list, int numalias)
{
	int i;
	
	for (i = 0; i < numalias && alias_list[i].comm_name; i++) {
		if (!strcasecmp(tmp, alias_list[i].comm_name))
			return i;
	}
	return -1;			/* not found */
}

/* Puts alias substitution into alias_string */
static void alias_substitute(struct alias_type *alias_list, int num_alias,
			     char *com_str, char outalias[])
{
	char *name, *atpos;
	int i;
	int at_offset=0;

	/* handle punctuation commands like '+' */
	if (ispunct(*com_str)) {
		int n;
		for (n=0;ispunct(com_str[n]);n++) ;
		name = strndup(com_str, n);
	} else {
		name = strndup(com_str, strcspn(com_str, " \t"));
	}
	com_str += strlen(name);

	while (isspace(*com_str)) com_str++;

	i = alias_lookup(name, alias_list, num_alias);

	if (i >= 0) {
		free(name);
		name = strdup(alias_list[i].alias);
	}

	/* now substitute '@' values in name */
	while ((atpos = strchr(name+at_offset, '@'))) {
		char *name2 = NULL;
		asprintf(&name2, "%*.*s%s%s", 
			 atpos-name, atpos-name, name,
			 com_str,
			 atpos+1);
		if (!name2) break;

		/* try to prevent loops */
		at_offset = (atpos - name) + strlen(com_str);

		free(name);
		name = name2;
	}

	/* there is an implicit @ after each alias */
	if (at_offset == 0 && *com_str) {
	  sprintf(outalias, "%s %s", name, com_str);
	} else {
	  strcpy(outalias, name);
	}

	free(name);
}

/* Returns pointer to command that matches */
static int match_command(char *comm, int p)
{
	int i = 0;
	int gotIt = -1;
	int len = strlen(comm);

	while (command_list[i].comm_name) {
		if (strncmp(command_list[i].comm_name, comm, len) == 0 &&
		    check_admin(p, command_list[i].adminLevel)) {
			if (gotIt >= 0)
				return -COM_AMBIGUOUS;
			gotIt = i;
		}
		i++;
	}

	if (gotIt == -1) {
		return -COM_FAILED;
	}

	if (in_list(p, L_REMOVEDCOM, command_list[gotIt].comm_name)) {
		pprintf(p, "Due to a bug - this command has been temporarily removed.\n");
		return -COM_FAILED;
	}
	lastCommandFound = gotIt;
	return gotIt;
}

/* Gets the parameters for this command */
static int get_parameters(int command, char *parameters, param_list params)
{
  int i, parlen;
  int paramLower;
  char c;
  static char punc[2];

  punc[1] = '\0';		/* Holds punc parameters */
  for (i = 0; i < MAXNUMPARAMS; i++)
    (params)[i].type = TYPE_NULL;	/* Set all parameters to NULL */
  parlen = strlen(command_list[command].param_string);
  for (i = 0; i < parlen; i++) {
    c = command_list[command].param_string[i];
    if (isupper(c)) {
      paramLower = 0;
      c = tolower(c);
    } else {
      paramLower = 1;
    }
    switch (c) {
    case 'w':
    case 'o':			/* word or optional word */
      parameters = eatwhite(parameters);
      if (!*parameters)
	return (c == 'o' ? COM_OK : COM_BADPARAMETERS);
      (params)[i].val.word = parameters;
      (params)[i].type = TYPE_WORD;
      if (ispunct(*parameters)) {
	punc[0] = *parameters;
	(params)[i].val.word = punc;
	parameters++;
	if (*parameters && isspace(*parameters))
	  parameters++;
      } else {
	parameters = eatword(parameters);
	if (*parameters != '\0') {
	  *parameters = '\0';
	  parameters++;
	}
      }
      if (paramLower)
	stolower((params)[i].val.word);
      break;

    case 'd':
    case 'p':			/* optional or required integer */
      parameters = eatwhite(parameters);
      if (!*parameters)
	return (c == 'p' ? COM_OK : COM_BADPARAMETERS);
      if (sscanf(parameters, "%d", &(params)[i].val.integer) != 1)
	return COM_BADPARAMETERS;
      (params)[i].type = TYPE_INT;
      parameters = eatword(parameters);
      if (*parameters != '\0') {
	*parameters = '\0';
	parameters++;
      }
      break;

    case 'i':
    case 'j':
    case 'n':			/* optional or required word or integer */
      parameters = eatwhite(parameters);
      if (!*parameters)
	return (c == 'n' ? COM_OK : COM_BADPARAMETERS);
      if (sscanf(parameters, "%d", &(params)[i].val.integer) != 1) {
	(params)[i].val.word = parameters;
	(params)[i].type = TYPE_WORD;
      } else {
	(params)[i].type = TYPE_INT;
      }
      if (ispunct(*parameters) && (c != 'j' || (params)[i].type != TYPE_INT)) {
	punc[0] = *parameters;
	(params)[i].val.word = punc;
	(params)[i].type = TYPE_WORD;
	parameters++;
	if (*parameters && isspace(*parameters))
	  parameters++;
      } else {
	parameters = eatword(parameters);
	if (*parameters != '\0') {
	  *parameters = '\0';
	  parameters++;
	}
      }
      if ((params)[i].type == TYPE_WORD)
	if (paramLower)
	  stolower((params)[i].val.word);
      break;

    case 's':
    case 't':			/* optional or required string to end */
      if (!*parameters)
	return (c == 't' ? COM_OK : COM_BADPARAMETERS);
      (params)[i].val.string = parameters;
      (params)[i].type = TYPE_STRING;
      while (*parameters)
	parameters = nextword(parameters);
      if (paramLower)
	stolower((params)[i].val.string);
      break;
    }
  }
  if (*parameters)
    return COM_BADPARAMETERS;
  else
    return COM_OK;
}

static void printusage(int p, char *command_str)
{
  struct player *pp = &player_globals.parray[p];
  int i, parlen, UseLang = pp->language;
  int command;
  char c;

  char *filenames[1000];        /* enough for all usage names */

  if ((command = match_command(command_str, p)) < 0) {
    pprintf(p, "  UNKNOWN COMMAND\n");
    return;
  }

/*Usage added by DAV 11/19/95 */
  /* First lets check if we have a text usage file for it */

  i = search_directory(usage_dir[UseLang], command_str, filenames, 1000);
  if (i == 0) { /* nope none in current Lang */
    if (UseLang != LANG_DEFAULT) {
      i += search_directory(usage_dir[LANG_DEFAULT], command_str, filenames, 1000);
      if (i > 0) {
        pprintf(p, "No usage available in %s; using %s instead.\n",
                Language(UseLang), Language(LANG_DEFAULT));
        UseLang = LANG_DEFAULT;
      }
    }
  }

 if (i != 0) {
  if ((i == 1) || (!strcmp(*filenames, command_str))) { /* found it? then send */
    if (psend_file(p, usage_dir[UseLang], *filenames)) {
      /* we should never reach this unless the file was just deleted */
      pprintf(p, "Usage file %s could not be found! ", *filenames);
      pprintf(p, "Please inform an admin of this. Thank you.\n");
      /* no need to print 'system' usage - should never happen */
    }
    pprintf(p, "\nSee '%s %s' for a complete description.\n", 
	    ((command_list[lastCommandFound].adminLevel > ADMIN_USER) ? "ahelp" :
"help"),
	    command_list[lastCommandFound].comm_name);
    return;
  } 
 } 

  /* print the default 'system' usage files (which aren't much help!) */

  pprintf(p, "Usage: %s", command_list[lastCommandFound].comm_name);

  parlen = strlen(command_list[command].param_string);
  for (i = 0; i < parlen; i++) {
    c = command_list[command].param_string[i];
    if (isupper(c))
      c = tolower(c);
    switch (c) {
    case 'w':			/* word */
      pprintf(p, " word");
      break;
    case 'o':			/* optional word */
      pprintf(p, " [word]");
      break;
    case 'd':			/* integer */
      pprintf(p, " integer");
      break;
    case 'p':			/* optional integer */
      pprintf(p, " [integer]");
      break;
    case 'i':			/* word or integer */
      pprintf(p, " {word, integer}");
      break;
    case 'n':			/* optional word or integer */
      pprintf(p, " [{word, integer}]");
      break;
    case 's':			/* string to end */
      pprintf(p, " string");
      break;
    case 't':			/* optional string to end */
      pprintf(p, " [string]");
      break;
    }
  }
  pprintf(p, "\nSee '%s %s' for a complete description.\n", 
          ((command_list[lastCommandFound].adminLevel > ADMIN_USER) ? "ahelp" :
"help"),
          command_list[lastCommandFound].comm_name);
}

static int one_command(int p, char *command, char **cmd)
{
	struct player *pp = &player_globals.parray[p];
	int which_command, retval;
	char *comm=NULL, *parameters=NULL;
	param_list params;

	if ((retval = parse_command(command, &comm, &parameters)))
		return retval;
	if (pp->game >= 0) {
		if ((game_globals.garray[pp->game].status == GAME_SETUP) && (is_drop(comm)))
			return COM_ISMOVE;
	}
	if (is_move(comm)) {
		if (pp->game == -1) {
			pprintf(p, "You are not playing or examining a game.\n");
			return COM_OK;
		}

		if (game_globals.garray[pp->game].status == GAME_SETUP)
			return COM_ISMOVE_INSETUP;
		else 
			return COM_ISMOVE;
	}
	stolower(comm);		/* All commands are case-insensitive */
	*cmd = comm;
	if ((which_command = match_command(comm, p)) < 0)
		return -which_command;
	if (!check_admin(p, command_list[which_command].adminLevel)) {
		return COM_RIGHTS;
	}
	if ((retval = get_parameters(which_command, parameters, params)))
		return retval;

	if (command_list[which_command].adminLevel >= ADMIN_ADMIN) {
		admin_log(pp, command, params);
	}

	return command_list[which_command].comm_func(p, params);
}

static int process_command(int p, char *com_string, char **cmd)
{
	struct player *pp = &player_globals.parray[p];
	char *tok;
	char *ptr = NULL;
	char astring1[MAX_STRING_LENGTH * 4];
	char astring2[MAX_STRING_LENGTH * 4];

#ifdef DEBUG
	if (strcasecmp(pp->name, pp->login)) {
		d_printf( "CHESSD: PROBLEM Name=%s, Login=%s\n", 
			pp->name, pp->login);
	}
#endif
	if (!com_string)
		return COM_FAILED;
#ifdef DEBUG
	d_printf( "%s, %s, %d: >%s<\n", 
		pp->name, pp->login, pp->socket, com_string);
#endif

	/* don't expand the alias command */
	if (strncmp(com_string, "alias ", 6) == 0) {
		return one_command(p, com_string, cmd);
	}

	/* don't expand the alias command */
	if (com_string[0] == '$') {
		return one_command(p, eatwhite(com_string+1), cmd);
	}

	alias_substitute(pp->alias_list, pp->numAlias,
			 com_string, astring1);
	alias_substitute(g_alias_list, 999,
			 astring1, astring2);

#ifdef DEBUG
	if (strcmp(com_string, astring2) != 0) {
		d_printf( "%s -alias-: >%s< >%s<\n", 
			pp->name, com_string, astring2);
	}
#endif

	for (tok=strtok_r(astring2, ";", &ptr); tok; tok=strtok_r(NULL, ";", &ptr)) {
		char alias_string1[MAX_STRING_LENGTH * 4];
		char alias_string2[MAX_STRING_LENGTH * 4];
		int retval;
		while (isspace(*tok)) tok++;

		alias_substitute(pp->alias_list, pp->numAlias,
				 tok, alias_string1);
		alias_substitute(g_alias_list, 999,
				 alias_string1, alias_string2);

#ifdef DEBUG
		if (strcmp(tok, alias_string2) != 0) {
			d_printf( "%s -alias2-: >%s<\n", 
				pp->name, alias_string2);
		}
#endif

		retval = one_command(p, alias_string2, cmd);

		/* stop on first error */
		if (retval != COM_OK) return retval;
	}

	return COM_OK;
}

static int process_login(int p, char *loginname)
{
	struct player *pp = &player_globals.parray[p];
	char loginnameii[80];
	int is_guest = 0;

	loginname = eatwhite(loginname);

	if (!*loginname) {
		goto new_login;
	}

	/* if 'guest' was specified then pick a random guest name */
	if (strcasecmp(loginname, config_get_tmp("GUEST_LOGIN")) == 0) {
		loginname = guest_name();
		is_guest = 1;
		pprintf(p,"\nCreated temporary login '%s'\n", loginname);
	}

	strlcpy(loginnameii, loginname, sizeof(loginnameii));

	if (!alphastring(loginnameii)) {
		pprintf(p, "\nSorry, names can only consist of lower and upper case letters.  Try again.\n");
		goto new_login;
	} 
	if (strlen(loginnameii) < 3) {
		pprintf(p, "\nA name should be at least three characters long!  Try again.\n");
		goto new_login;
	}

	if (strlen(loginnameii) > (MAX_LOGIN_NAME - 1)) {
		pprintf(p, "\nSorry, names may be at most %d characters long.  Try again.\n",
			MAX_LOGIN_NAME - 1);
		goto new_login;
	} 

	if (in_list(p, L_BAN, loginnameii)) {
		pprintf(p, "\nPlayer \"%s\" is banned.\n", loginnameii);
		return COM_LOGOUT;
	}

	if (!in_list(p, L_ADMIN, loginnameii) &&
	    player_count(0) >= config_get_int("MAX_PLAYER", DEFAULT_MAX_PLAYER)) {
		psend_raw_file(p, MESS_DIR, MESS_FULL);
		return COM_LOGOUT;
	} 

	if (player_read(p, loginnameii) != 0) {
		if (!is_guest && 
		    config_get_int("GUEST_PREFIX_ONLY", DEFAULT_GUEST_PREFIX_ONLY)) {
			goto new_login;
		}
		/* they are not registered */
		strcpy(pp->name, loginnameii);
		if (in_list(p, L_FILTER, dotQuad(pp->thisHost))) {
			pprintf(p, "\nDue to abusive behavior, nobody from your site may login.\n");
			pprintf(p, "If you wish to use this server please email %s\n", 
				config_get_tmp("REGISTRATION_ADDRESS"));
			pprintf(p, "Include details of a nick-name to be called here, e-mail address and your real name.\n");
			pprintf(p, "We will send a password to you. Thanks.\n");
			return COM_LOGOUT;
		}

		if (player_count(0) >= config_get_int("MAX_PLAYER", DEFAULT_MAX_PLAYER) - 100) {
			psend_raw_file(p, MESS_DIR, MESS_FULL_UNREG);
			return COM_LOGOUT;
		}

		pprintf_noformat(p, "\n\"%s\" is not a registered name.  You may play unrated games as a guest.\n(After logging in, do \"help register\" for more info on how to register.)\n\nPress return to enter the FICS as \"%s\":", 
				 loginnameii, loginnameii);
		pp->status = PLAYER_PASSWORD;
		turn_echo_off(pp->socket);
		return COM_OK;
	}

	pprintf_noformat(p, "\n\"%s\" is a registered name.  If it is yours, type the password.\nIf not, just hit return to try another name.\n\npassword: ", 
			 pp->name);
	pp->status = PLAYER_PASSWORD;
	turn_echo_off(pp->socket);
	if (strcasecmp(loginnameii, pp->name)) {
		pprintf(p, "\nYou've got a bad name field in your playerfile -- please report this to an admin!\n");
		return COM_LOGOUT;
	}

	if (CheckPFlag(p, PFLAG_REG)
	    && (pp->fullName == NULL)) {
		pprintf(p, "\nYou've got a bad playerfile -- please report this to an admin!\n");
		pprintf(p, "Your FullName is missing!");
		pprintf(p, "Please log on as an unreg until an admin can correct this.\n");
		return COM_LOGOUT;
	}
	if (CheckPFlag(p, PFLAG_REG)
	    && (pp->emailAddress == NULL)) {
		pprintf(p, "\nYou've got a bad playerfile -- please report this to an admin!\n");
		pprintf(p, "Your Email address is missing\n");
		pprintf(p, "Please log on as an unreg until an admin can correct this.\n");
		return COM_LOGOUT;
	}
  
  
	return COM_OK;

new_login:
	/* give them a new prompt */
	psend_raw_file(p, MESS_DIR, MESS_LOGIN);
	pprintf(p, "login: ");
	return COM_OK;
}

static void boot_out(int p, int p1)
{
	struct player *pp = &player_globals.parray[p];
	int fd;
	pprintf(p, "\n **** %s is already logged in - kicking them out. ****\n", pp->name);
	pprintf(p1, "**** %s has arrived - you can't both be logged in. ****\n", pp->name);
	fd = player_globals.parray[p1].socket;
	process_disconnection(fd);
	net_close_connection(fd);
}

static int process_password(int p, char *password)
{
  struct player *pp = &player_globals.parray[p];
  static int Current_ad;
  int p1;
  char salt[3];
  int fd;
  struct in_addr fromHost;
  int messnum;
  char fname[10];
  int dummy; /* (to hold a return value) */ 

  turn_echo_on(pp->socket);

  if (pp->passwd && CheckPFlag(p, PFLAG_REG)) {
    salt[0] = pp->passwd[3];
    salt[1] = pp->passwd[4];
    salt[2] = '\0';
    if (strcmp(chessd_crypt(password,salt), pp->passwd)) {
      fd = pp->socket;
      fromHost = pp->thisHost;
      if (*password) {
	pprintf(p, "\n\n**** Invalid password! ****\n\n");
        d_printf("FICS (process_password): Bad password for %s [%s] [%s] [%s]\n",
		pp->login, 
		password, 
		salt,
		pp->passwd);
      }
      player_clear(p);
      pp->logon_time = pp->last_command_time = time(0);
      pp->status = PLAYER_LOGIN;
      pp->socket = fd;
      if (fd >= net_globals.no_file)
        d_printf("FICS (process_password): Out of range fd!\n");

      pp->thisHost = fromHost;

      psend_raw_file(p, MESS_DIR, MESS_LOGIN);
      pprintf(p, "login: ");
      return COM_OK;
    }
  }
  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if (player_globals.parray[p1].name != NULL) {
      if ((!strcasecmp(pp->name, player_globals.parray[p1].name)) && (p != p1)) {
	if (!CheckPFlag(p, PFLAG_REG)) {
	  pprintf(p, "\n*** Sorry %s is already logged in ***\n", pp->name);
	  return COM_LOGOUT;
	}
	boot_out(p, p1);
      }
    }
  }
  
  if (player_ishead(p)) {
	  pprintf(p,"\n  ** LOGGED IN AS HEAD ADMIN **\n");
	  pp->adminLevel = ADMIN_GOD;
  }

  news_login(p);

  if (pp->adminLevel > 0) {
    psend_raw_file(p, MESS_DIR, MESS_ADMOTD);
  } else {
    psend_raw_file(p, MESS_DIR, MESS_MOTD);
  }
  if (MAX_ADVERTS >=0) {
    pprintf (p, "\n");
    sprintf (fname,"%d",Current_ad);
    Current_ad = (Current_ad + 1) % MAX_ADVERTS;
    psend_raw_file(p, ADVERT_DIR, fname);
  }
  if (!pp->passwd && CheckPFlag(p, PFLAG_REG))
    pprintf(p, "\n*** You have no password. Please set one with the password command.");
  if (!CheckPFlag(p, PFLAG_REG))
    psend_raw_file(p, MESS_DIR, MESS_UNREGISTERED);
  pp->status = PLAYER_PROMPT;
  player_write_login(p);
  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if (p1 == p)
      continue;
    if (player_globals.parray[p1].status != PLAYER_PROMPT)
      continue;
    if (!CheckPFlag(p1, PFLAG_PIN))
      continue;
    if (player_globals.parray[p1].adminLevel > 0) {
      pprintf_prompt(p1, "\n[%s (%s: %s) has connected.]\n", pp->name,
		     (CheckPFlag(p, PFLAG_REG) ? "R" : "U"),
		     dotQuad(pp->thisHost));
    } else {
      pprintf_prompt(p1, "\n[%s has connected.]\n", pp->name);
    }
  }
  pp->num_comments = player_num_comments(p);
  messnum = player_num_messages(p);

  if (messnum) {
    if (messnum == 1)
      pprintf(p, "\nYou have 1 message.\nUse \"messages\" to display it, or \"clearmessages\" to remove it.\n");
    else
      pprintf(p, "\nYou have %d messages.\nUse \"messages\" to display them, or \"clearmessages\" to remove them.\n", messnum);
  }

  player_notify_present(p);
  player_notify(p, "arrived", "arrival");
  showstored(p);

  if (CheckPFlag(p, PFLAG_REG) && (pp->lastHost.s_addr != 0) &&
      (pp->lastHost.s_addr != pp->thisHost.s_addr)) {
    pprintf(p, "\nPlayer %s: Last login: %s ", pp->name,
	    dotQuad(pp->lastHost));
    pprintf(p, "This login: %s", dotQuad(pp->thisHost));
  }
  pp->lastHost = pp->thisHost;
  if (CheckPFlag(p, PFLAG_REG) && !pp->timeOfReg)
    pp->timeOfReg = time(0);
  pp->logon_time = pp->last_command_time = time(0);
  dummy = check_and_print_shutdown(p);
  pprintf_prompt(p, "\n");
  if (CheckPFlag(p, PFLAG_REG))
    announce_avail(p);
  return 0;
}

FILE *comlog = NULL;

static int process_prompt(int p, char *command)
{
  struct player *pp = &player_globals.parray[p];
  int retval;
  char *cmd = "";

	if(comlog == NULL) comlog = fopen("command.log", "a");
	if(comlog) fprintf(comlog, "p%d: '%s'\n", p, command), fflush(comlog);

  command = eattailwhite(eatwhite(command));
  if (!*command) {
	  send_prompt(p);
	  return COM_OK;
  }
  retval = process_command(p, command, &cmd);
  switch (retval) {
  case COM_OK:
    retval = COM_OK;
    send_prompt(p);
    break;
  case COM_OK_NOPROMPT:
    retval = COM_OK;
    break;
  case COM_ISMOVE:
    retval = COM_OK;

    if (pp->game >= 0 && game_globals.garray[pp->game].status == GAME_ACTIVE
        && pp->side == game_globals.garray[pp->game].game_state.onMove
        && game_globals.garray[pp->game].flag_pending != FLAG_NONE) {
	    ExecuteFlagCmd(p, net_globals.con[pp->socket]);
    }

    process_move(p, command);
    send_prompt(p);

    break;
  case COM_ISMOVE_INSETUP:
    pprintf(p, "You are still setting up the position.\n");
    pprintf_prompt(p, "Type: 'setup done' when you are finished editing.\n");
    retval = COM_OK;
    break;
  case COM_RIGHTS:
    pprintf_prompt(p, "%s: Insufficient rights.\n", cmd);
    retval = COM_OK;
    break;
  case COM_AMBIGUOUS:
/*    pprintf(p, "%s: Ambiguous command.\n", cmd); */
    {
      int len = strlen(cmd);
      int i = 0;
      pprintf(p, "Ambiguous command. Matches:");
      while (command_list[i].comm_name) {
	if (strncmp(command_list[i].comm_name, cmd, len) == 0 &&
	    check_admin(p, command_list[i].adminLevel)) {
		pprintf(p, " %s", command_list[i].comm_name);
	}
	i++;
      }
    }
    pprintf_prompt(p, "\n");
    retval = COM_OK;
    break;
  case COM_BADPARAMETERS:
    printusage(p, command_list[lastCommandFound].comm_name);
    send_prompt(p);
    retval = COM_OK;
    break;
  case COM_FAILED:
  case COM_BADCOMMAND:
    pprintf_prompt(p, "%s: Command not found.\n", cmd);
    d_printf("Command not found [%s]\n", cmd);
    retval = COM_OK;
    break;
  case COM_LOGOUT:
    retval = COM_LOGOUT;
    break;
  }
  return retval;
}

/* Return 1 to disconnect */
int process_input(int fd, char *com_string)
{
  int p = player_find(fd);
  int retval = 0;
  struct player *pp;

  if (p < 0) {
    d_printf( "CHESSD: Input from a player not in array!\n");
    return -1;
  }

  pp = &player_globals.parray[p];

  command_globals.commanding_player = p;
  pp->last_command_time = time(0);

  switch (pp->status) {
  case PLAYER_EMPTY:
    d_printf( "CHESSD: Command from an empty player!\n");
    break;
  case PLAYER_NEW:
    d_printf( "CHESSD: Command from a new player!\n");
    break;
  case PLAYER_INQUEUE:
    /* Ignore input from player in queue */
    break;
  case PLAYER_LOGIN:
    retval = process_login(p, com_string);
    if (retval == COM_LOGOUT && com_string != NULL)
      d_printf("%s tried to log in and failed.\n", com_string);
    break;
  case PLAYER_PASSWORD:
    retval = process_password(p, com_string);
    break;
  case PLAYER_PROMPT:
	  FREE(pp->busy);
	  retval = process_prompt(p, com_string);
	  break;
  }

  command_globals.commanding_player = -1;
  return retval;
}

int process_new_connection(int fd, struct in_addr fromHost)
{
	struct player *pp;
	int p = player_new();

	pp = &player_globals.parray[p];

	pp->status = PLAYER_LOGIN;
	if (fd >= net_globals.no_file)
		d_printf("FICS (process_new_connection): Out of range fd!\n");
	
	pp->socket = fd;
	pp->thisHost = fromHost;
	pp->logon_time = time(0);
	psend_raw_file(p, MESS_DIR, MESS_WELCOME);
	pprintf(p, "Head admin : %s", config_get_tmp("HEAD_ADMIN"));
	pprintf(p, "    Complaints to : %s\n", config_get_tmp("HEAD_ADMIN_EMAIL"));
	pprintf(p, "Server location: %s", config_get_tmp("SERVER_LOCATION"));
	pprintf(p, "    Server version : %s\n", VERS_NUM);
	pprintf(p, "Server name : %s\n", config_get_tmp("SERVER_HOSTNAME"));
	psend_raw_file(p, MESS_DIR, MESS_LOGIN);
	pprintf(p, "login: ");
	
	return 0;
}

int process_disconnection(int fd)
{
  int p = player_find(fd);
  int p1;
  char command[1024];
  struct player *pp;

  if (p < 0) {
    d_printf( "CHESSD: Disconnect from a player not in array!\n");
    return -1;
  }

  pp = &player_globals.parray[p];

  if (CheckPFlag(p, PFLAG_REG) && CheckPFlag(p, PFLAG_OPEN) &&
      pp->game < 0)
    announce_notavail(p);
  if ((pp->game >=0) && ((game_globals.garray[pp->game].status == GAME_EXAMINE) || (game_globals.garray[pp->game].status == GAME_SETUP))) {
    pcommand(p, "unexamine");
  }
  if ((pp->game >=0) && (in_list(p, L_ABUSER, pp->name)
        || (game_globals.garray[pp->game].link >= 0)))

     pcommand(p, "resign");

  if (pp->ftell != -1)
    pcommand (p,"tell 0 I am logging out now - conversation forwarding stopped.");

  withdraw_seeks(p);

  if (pp->status == PLAYER_PROMPT) {
    for (p1 = 0; p1 < player_globals.p_num; p1++) {
      if (p1 == p)
	continue;
      if (player_globals.parray[p1].status != PLAYER_PROMPT)
	continue;

      if (player_globals.parray[p1].ftell == p) {
        player_globals.parray[p1].ftell = -1;
        sprintf (command,"tell 0 *%s* has logged out - conversation forwarding stopped.",pp->name);
        pcommand (p1,command);
        pprintf_prompt (p1,"%s, whose tells you were forwarding has logged out.\n",
                 pp->name);
      }

      if (!CheckPFlag(p1, PFLAG_PIN))
	continue;
      pprintf_prompt(p1, "\n[%s has disconnected.]\n", pp->name);
    }
    player_notify(p, "departed", "departure");
    player_notify_departure(p);
    player_write_logout(p);
    if (CheckPFlag(p, PFLAG_REG)) {
      pp->totalTime += time(0) - pp->logon_time;
      player_save(p);
    } else {			/* delete unreg history file */
      char fname[MAX_FILENAME_SIZE];
      sprintf(fname, "%s/player_data/%c/%s.games", STATS_DIR, pp->login[0], pp->login);
      unlink(fname);
    }
  }
  player_remove(p);
  return 0;
}

/* Called every few seconds */
int process_heartbeat(int *fd)
{
	struct tm *nowtm;
	int p;
	time_t now = time(0); 
	unsigned idle_timeout = config_get_int("IDLE_TIMEOUT", DEFAULT_IDLE_TIMEOUT);
	unsigned login_timeout = config_get_int("LOGIN_TIMEOUT", DEFAULT_LOGIN_TIMEOUT);

	/* Check for timed out connections */
	for (p = 0; p < player_globals.p_num; p++) {
		struct player *pp = &player_globals.parray[p];

		if ((pp->status == PLAYER_LOGIN ||
		     pp->status == PLAYER_PASSWORD) &&
		    player_idle(p) > login_timeout) {
			pprintf(p, "\n**** LOGIN TIMEOUT ****\n");
			*fd = pp->socket;
			return COM_LOGOUT;
		}
		if (pp->status == PLAYER_PROMPT &&
		    player_idle(p) > idle_timeout &&
		    !check_admin(p, ADMIN_ADMIN) &&
		    !in_list(p, L_TD, pp->name)) {
			pprintf(p, "\n**** Auto-logout - you were idle more than %u minutes. ****\n", 
				idle_timeout/60);
			*fd = pp->socket;
			return COM_LOGOUT;
		}
	}
	nowtm=localtime((time_t *)&now);
	if (nowtm->tm_min==0) {
		gics_globals.userstat.users[nowtm->tm_hour*2]=player_count(1);
		save_userstat();
	}
	if (nowtm->tm_min==30) {
		gics_globals.userstat.users[nowtm->tm_hour*2+1]=player_count(1);
		save_userstat();
	}
	if (command_globals.player_high > gics_globals.userstat.usermax) {
		gics_globals.userstat.usermax=command_globals.player_high;
		gics_globals.userstat.usermaxtime=now;
		save_userstat();
	}
	if (command_globals.game_high > gics_globals.userstat.gamemax) {
		gics_globals.userstat.gamemax=command_globals.game_high;
		gics_globals.userstat.gamemaxtime=now;
		save_userstat();
	}
	
	ShutHeartBeat();
	return COM_OK;
}

/* helper function for sorting command list */
static int command_compare(struct command_type *c1, struct command_type *c2)
{
	return strcasecmp(c1->comm_name, c2->comm_name);
}

void commands_init(void)
{
	FILE *fp, *afp;
	int i = 0;
	int count=0, acount=0;

	/* sort the command list */
	qsort(command_list, 
	      (sizeof(command_list)/sizeof(command_list[0])) - 1,
	      sizeof(command_list[0]),
	      (COMPAR_FN_T)command_compare);

	command_globals.commanding_player = -1;
	
	fp = fopen_s(HELP_DIR "/commands", "w");
	if (!fp) {
		d_printf( "CHESSD: Could not write commands help file.\n");
		return;
	}
	afp = fopen_s(ADHELP_DIR "/commands", "w");
	if (!afp) {
		d_printf( "CHESSD: Could not write admin commands help file.\n");
		fclose(fp);
		return;
	}

	while (command_list[i].comm_name) {
		if (command_list[i].adminLevel >= ADMIN_ADMIN) {
			fprintf(afp, "%-19s", command_list[i].comm_name);
			acount++;
			if (acount % 4 == 0) {
				fprintf(afp,"\n");
			}
		} else {
			fprintf(fp, "%-19s", command_list[i].comm_name);
			count++;
			if (count % 4 == 0) {
				fprintf(fp,"\n");
			}
		}
		i++;
	}

	fprintf(afp,"\n");
	fprintf(fp,"\n");

	fclose(fp);
	fclose(afp);

	d_printf("CHESSD: Loaded %d commands (admin=%d normal=%d)\n", i, acount, count);
}

/* Need to save rated games */
void TerminateCleanup(void)
{
	int p1;
	int g;
  
	save_userstat();

	for (g = 0; g < game_globals.g_num; g++) {
		if (game_globals.garray[g].status != GAME_ACTIVE)
			continue;
		if (game_globals.garray[g].rated) {
			game_ended(g, WHITE, END_ADJOURN);
		}
	}
	for (p1 = 0; p1 < player_globals.p_num; p1++) {
		if (player_globals.parray[p1].status == PLAYER_EMPTY)
			continue;
		pprintf(p1, "\n    **** Server shutting down immediately. ****\n\n");
		if (player_globals.parray[p1].status != PLAYER_PROMPT) {
			close(player_globals.parray[p1].socket);
		} else {
			pprintf(p1, "Logging you out.\n");
			psend_raw_file(p1, MESS_DIR, MESS_LOGOUT);
			player_write_logout(p1);
			if (CheckPFlag(p1, PFLAG_REG))
				player_globals.parray[p1].totalTime += time(0) - player_globals.parray[p1].logon_time;
			player_save(p1);
		}
	}
	destruct_pending();
}

static char *guest_name(void)
{
       static char name[20];
       int found;

#define RANDLET ((char)('A' + (random() % 26)))
       srandom(time(0));

       found = 0;
       while(!found) {
               snprintf(name,sizeof(name), "Guest%c%c%c%c", RANDLET, RANDLET, RANDLET, RANDLET);
               found = check_user(name);
       }
#undef RANDLET
       return name;
}

static int check_user(char *user)
{
       int i;
       
       for (i = 0; i < player_globals.p_num; i++) {
               if (player_globals.parray[i].name != NULL) {
                       if (!strcasecmp(user, player_globals.parray[i].name))
                               return 0;
               }
       }
       return 1;
}


/* return the global alias list */
const struct alias_type *alias_list_global(void)
{
	return g_alias_list;
}

/* return a personal alias list */
const struct alias_type *alias_list_personal(int p, int *n)
{
	struct player *pp = &player_globals.parray[p];

	*n = pp->numAlias;
	return pp->alias_list;
}

/*
  report on any missing help pages
*/
int com_acheckhelp(int p, param_list param)
{
	int i;
	int count;

	for (count=i=0; command_list[i].comm_name; i++) {
		char *fname;
		asprintf(&fname, "%s/%s", 
			 command_list[i].adminLevel?ADHELP_DIR:HELP_DIR, 
			 command_list[i].comm_name);
		if (!file_exists(fname)) {
			pprintf(p, "Help for command '%s' is missing%s\n",
				command_list[i].comm_name,
				command_list[i].adminLevel?" (admin)":"");
			count++;
		}
		free(fname);
	}

	pprintf(p, "%d commands are missing help files\n", count);

	for (count=i=0; command_list[i].comm_name; i++) {
		char *fname;
		asprintf(&fname, "%s/%s", USAGE_DIR, command_list[i].comm_name);
		if (!file_exists(fname)) {
			pprintf(p, "Usage for command '%s' is missing%s\n",
				command_list[i].comm_name,
				command_list[i].adminLevel?" (admin)":"");
			count++;
		}
		free(fname);
	}

	pprintf(p, "%d commands are missing usage files\n", count);

	return COM_OK;
}
