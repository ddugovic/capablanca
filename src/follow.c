/*
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

/*
  handle 'follow' lists. These are a little different to FICS in that they are persistent
  (ie. they are more similar to the personal gnotify list)
*/

#include "includes.h"

/* 
   a game has started between p1 and p2
   anyone who has either player in their personal follow list will now send an 
   effective 'observe' command if they are idle
*/
void follow_start(int p1,int p2)
{
	int p;
	for (p = 0; p < player_globals.p_num; p++) {
		struct player *pp = &player_globals.parray[p];
		int in1=0, in2=0;

		/* don't follow ourselves */
		if ((p == p1) || (p == p2)) {
			continue;
		}

		/* don't follow while playing, examining etc */
		if (pp->status != PLAYER_PROMPT) {
			continue;
		}

		/* see if either player is in our follow list */
		in1 = in_list(p, L_FOLLOW, player_globals.parray[p1].login);
		if (in1 == 0) {
			in2 = in_list(p, L_FOLLOW, player_globals.parray[p2].login);
		}

		if (in1 == 0 && in2 == 0) {
			continue;
		}

		/* fake up an observe command */
		pcommand(p, "observe %s\n", player_globals.parray[in1?p1:p2].login);
		send_prompt (p);
	}
}
