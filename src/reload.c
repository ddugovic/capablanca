/*
  load/unload local variables

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

static void variable_reload(void)
{
	if (config_open() != 0) {
		d_printf("CHESSD: config database open failed\n");
		exit(1);
	}

	news_open();
	commands_init();
	ratings_init();
	wild_init();
	book_open();
	
	init_userstat();
}

/* initialise variables that can be re-initialised on code reload */
void initial_load(void)
{
	command_globals.startuptime = time(0);

	seek_globals.quota_time = 60;
	command_globals.player_high = 0;
	command_globals.game_high = 0;
	srandom(command_globals.startuptime);
	variable_reload();
}

/* initialise variables that can be re-initialised on code reload */
void reload_open(void)
{
	load_all_globals("globals.dat");
	variable_reload();
}


/* initialise variables that can be re-initialised on code reload */
void reload_close(void)
{
	news_close();
	config_close();
	book_close();
	lists_close();
	save_all_globals("globals.dat");
#if 0
	m_free_all();
#endif
}
