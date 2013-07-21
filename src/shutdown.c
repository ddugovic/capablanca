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

/* shutdown.c
  Contains stuff related to shutdowns
  
*/

#include "includes.h"

static int shutdownTime = 0;
static int lastTimeLeft;
static int shutdownStartTime;
static char downer[1024];
static char reason[1024];

void output_shut_mess(void)
{
	time_t shuttime = time(0);
	d_printf( "CHESSD: Shutting down at %s\n", strltime(&shuttime));
}

static void ShutDown(void)
{
	int p1;
	time_t shuttime = time(0);

	for (p1 = 0; p1 < player_globals.p_num; p1++) {
		if (player_globals.parray[p1].status != PLAYER_PROMPT)
			continue;
		pprintf(p1, "\n\n    **** Server shutdown ordered by %s. ****\n", 
			downer);
		if (reason[0] != '\0')
			pprintf(p1, "\n    **** We are going down because: %s. ****\n", 
				reason);
	}
	TerminateCleanup();
	d_printf( "CHESSD: Shut down ordered at %s by %s.\n", 
		strltime(&shuttime), downer);

	output_shut_mess();
	net_close();
	exit(0);
}

void ShutHeartBeat(void)
{
  int t = time(0);
  int p1;
  int timeLeft;
  int crossing = 0;

  if (!shutdownTime)
    return;
  if (!lastTimeLeft)
    lastTimeLeft = shutdownTime;
  timeLeft = shutdownTime - (t - shutdownStartTime);
  if ((lastTimeLeft > 3600) && (timeLeft <= 3600))
    crossing = 1;
  if ((lastTimeLeft > 2400) && (timeLeft <= 2400))
    crossing = 1;
  if ((lastTimeLeft > 1200) && (timeLeft <= 1200))
    crossing = 1;
  if ((lastTimeLeft > 600) && (timeLeft <= 600))
    crossing = 1;
  if ((lastTimeLeft > 300) && (timeLeft <= 300))
    crossing = 1;
  if ((lastTimeLeft > 120) && (timeLeft <= 120))
    crossing = 1;
  if ((lastTimeLeft > 60) && (timeLeft <= 60))
    crossing = 1;
  if ((lastTimeLeft > 10) && (timeLeft <= 10))
    crossing = 1;
  if (crossing) {
    d_printf(
    "CHESSD:   **** Server going down in %d minutes and %d seconds. ****\n\n",
            timeLeft / 60,
            timeLeft - ((timeLeft / 60) * 60));
    if (reason[0] != '\0')
      d_printf("CHESSD: We are going down because: %s.\n",reason);
    for (p1 = 0; p1 < player_globals.p_num; p1++) {
      if (player_globals.parray[p1].status != PLAYER_PROMPT)
        continue;
      pprintf(p1,
                     "\n\n    **** Server going down in %d minutes and %d seconds. ****\n",
                     timeLeft / 60,
                     timeLeft - ((timeLeft / 60) * 60));
      if (reason[0] != '\0')
        pprintf_prompt(p1,"\n    **** We are going down because: %s. ****\n",reason);
      else
        pprintf_prompt(p1,"\n");
    }
  }
  lastTimeLeft = timeLeft;
  if (timeLeft <= 0) {
    ShutDown();
  }
}

int check_and_print_shutdown(int p)

 /* Tells a user if they is to be a shutdown */
 /* returns 0 if there is not to be one, 1 if there is to be one */
 /* for whenshut command */

{


  int timeLeft;
  int t = time(0);

  if (!shutdownTime)
    return 0; /* no shut down */

  timeLeft = shutdownTime - (t - shutdownStartTime);

  pprintf(p,
                     "\n    **** Server going down in %d minutes and %d seconds. ****\n",
                     timeLeft / 60,
                     timeLeft - ((timeLeft / 60) * 60));
  if (reason[0] != '\0')
    pprintf(p, "\n    **** We are going down because: %s. ****\n", reason);
  return 1;
}


/*
 * shutdown
 *
 * Usage: shutdown [now,cancel,time]
 *
 *   This command shutsdown the server.  If the parameter is omitted or
 *   is 'now' then the server is immediately halted cleanly.  If a time is
 *   given then a countdown commences and the server is halted when time is
 *   up.  If 'cancel' is given then the countdown is stopped.
 */
int com_shutdown(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  char *ptr;
  int p1, secs;

  strcpy(downer, pp->name);
  shutdownStartTime = time(0);
  if (shutdownTime) {           /* Cancel any pending shutdowns */
    for (p1 = 0; p1 < player_globals.p_num; p1++) {
      if (player_globals.parray[p1].status != PLAYER_PROMPT)
        continue;
      pprintf(p1, "\n\n    **** Server shutdown canceled by %s. ****\n", downer);
    }
    shutdownTime = 0;
    if (param[0].type == TYPE_NULL)
      return COM_OK;
  }
  /* Work out how soon to shut down */
  if (param[0].type == TYPE_NULL)
    shutdownTime = 300;
  else {
    if (!strcmp(param[0].val.word, "now"))
      shutdownTime = 0;
    else if (!strcmp(param[0].val.word, "cancel"))
      return COM_OK;
    else {
      ptr = param[0].val.word;
      shutdownTime = secs = 0;
      p1 = 2;
      while (*ptr) {
        switch (*ptr) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          secs = secs * 10 + *ptr - '0';
          break;
        case ':':
          if (p1--) {
            shutdownTime = shutdownTime * 60 + secs;
            secs = 0;
            break;
          }
        default:
          shutdownTime = 0;
          pprintf(p, "I don't know what you mean by %s\n", param[0].val.word);
          return COM_OK;
        }
        ptr++;
      }
      shutdownTime = shutdownTime * 60 + secs;
    }
  }
  if (shutdownTime <= 0)
    ShutDown();
  if (param[1].type == TYPE_STRING)
    strcpy (reason,param[1].val.string);
  else
    reason[0] = '\0'; /* No reason - perhaps admin is in a bad mood? :) */
  for (p1 = 0; p1 < player_globals.p_num; p1++) {
    if (player_globals.parray[p1].status != PLAYER_PROMPT)
      continue;
    pprintf(p1, "\n\n    **** Server shutdown ordered by %s. ****\n", downer);
    if (reason[0] != '\0')
       pprintf(p1, "    **** We are going down because: %s. ****\n",reason);
    pprintf(p1,
        "    **** Server going down in %d minutes and %d seconds. ****\n",
                   shutdownTime / 60, shutdownTime % 60);
    if (p != p1)  /* fix double prompt - DAV */
      pprintf_prompt (p1,"\n");
    else
      pprintf (p1,"\n");
  }
  lastTimeLeft = 0;
  return COM_OK;
}

int com_whenshut(int p, param_list param)

{
 if (check_and_print_shutdown(p) == 0) {
   pprintf (p,"No shutdown currently in progress\n");
 }
 return COM_OK;
}
