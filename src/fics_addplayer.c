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

static void usage(char *progname)
{
  d_printf( "Usage: %s [-a level] [-l] [-n] UserName \"Full Name\" EmailAddress\n", progname);
  exit(1);
}

/* Parameters */
static int local = 1;
static char *funame = NULL, *fname = NULL, *email = NULL;

unsigned chessd_reload_flag;

#define PASSLEN 4
int main(int argc, char *argv[])
{
  int admin, i, ch;
  int p;
  char password[PASSLEN + 1];
  char *text;
  struct player *pp;

  admin = 0;

  while((ch = getopt(argc, argv, "lna:")) != -1) {
	switch(ch) {
      	case 'l':
		local = 1;
		break;
    	case 'n':
		local = 0;
		break;
	case 'a':
		admin = atoi(optarg);
      }
  }

  if (argc > 3) {
    funame = strdup(argv[argc-3]);
    fname = strdup(argv[argc-2]);
    email = strdup(argv[argc-1]);
  }
  if (!funame || !fname || !email)
    usage(argv[0]);

  /* Add the player here */
  if (strlen(funame) >= MAX_LOGIN_NAME) {
    d_printf( "Player name is too long\n");
    exit(0);
  }
  if (strlen(funame) < 3) {
    d_printf( "Player name is too short\n");
    exit(0);
  }
  if (!alphastring(funame)) {
    d_printf( "Illegal characters in player name. Only A-Za-z allowed.\n");
    exit(0);
  }

  srandom(time(0));
  p = player_new();
  if (!player_read(p, funame)) {
    d_printf( "%s already exists.\n", funame);
    exit(0);
  }

  pp = &player_globals.parray[p];

  pp->name = strdup(funame);
  pp->login = strdup(funame);
  pp->adminLevel = admin;

  stolower(pp->login);
  pp->fullName = strdup(fname);
  pp->emailAddress = strdup(email);
  for (i = 0; i < PASSLEN; i++) {
    password[i] = 'a' + random() % 26;
  }
  password[i] = '\0';
  pp->passwd = strdup(chessd_crypt(password, NULL));
  PFlagON(p, PFLAG_REG);
/*  pp->network_player = !local; */
  PFlagON(p, PFLAG_RATED);
  player_save(p);
/*  changed by Sparky  12/15/95  Dropped reference to 'Network' players
    and spiffed it up  */
/*
  printf("Added %s player >%s< >%s< >%s< >%s<\n", local ? "local" : "network",
	 funame, fname, email, password);
  rasprintf(&text, "\nYou have been added as a %s player.\nIf this is a network account it may take a while to show up on all of the\nclients.\n\nLogin Name: %s\nFull Name: %s\nEmail Address: %s\nInitial Password: %s\n\nIf any of this information is incorrect, please contact the administrator\nto get it corrected.\nPlease write down your password, as it will be your initial passoword\non all of the servers.\n", local ? "local" : "network", funame, fname, email, password);
*/

  printf("Added %s account: >%s< >%s< >%s< >%s<\n", 
	 admin?"ADMIN":"player",
         funame, fname, email, password);

  asprintf(&text, "\nYour player account has been created.\n\n"
   "Login Name: %s\nFull Name: %s\nEmail Address: %s\nInitial Password: %s\n\n"
   "If any of this information is incorrect, please contact the administrator\n"
   "to get it corrected.\n\n"
   "You may change your password with the password command on the the server.\n"
   "\nPlease be advised that if this is an unauthorized duplicate account for\n"
   "you, by using it you take the risk of being banned from accessing this\n"
   "chess server.\n\n"
   "Regards,\n\nThe FICS admins\n", 
        funame, fname, email, password);

  free(text);

  mail_string_to_address(email, "FICS Account Created", text);
  return 0;
}
