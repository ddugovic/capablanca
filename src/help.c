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
  support for the 'help' and 'ahelp' commands
 */

#include "includes.h"

const char *help_dir[NUM_LANGS] = {HELP_DIR, HELP_SPANISH, 
					  HELP_FRENCH, HELP_DANISH};



/*
  help from a directory
*/	
static int help_page(int p, const char *dir, const char *cmd)
{
	int count;
	char *filenames[1000];

	/* try for help dir exact match */
	if (psend_file(p, dir, cmd) == 0) {
		return COM_OK;
	}
	
	/* search help dir for a partial match */
	count = search_directory(dir, cmd, filenames, 1000);
	if (count == 1) {
		psend_file(p, dir, filenames[0]);
		FREE(filenames[0]);
		return COM_OK;
	}
	
	if (count > 0) {
		pprintf(p,"-- Matches: %u help topics --", count);
		display_directory(p, filenames, count);
		return COM_OK;
	}

	/* allow admins to use 'help' for admin pages */
	if (dir != ADHELP_DIR && check_admin(p, ADMIN_ADMIN)) {
		return help_page(p, ADHELP_DIR, cmd);
	}

	pprintf(p,"No help found for topic '%s'\n", cmd);

	return COM_OK;
}

/*
  public help
 */
int com_help(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	char *cmd;
	int i, n;
	const struct alias_type *list;

	if (param[0].type == TYPE_STRING) {
		cmd = param[0].val.string;
	} else {
		cmd = "help";
	}

	/* see if its a global alias */
	list = alias_list_global();
	for (i=0;list && list[i].comm_name;i++) {
		if (strcasecmp(cmd, list[i].comm_name) == 0) {
			pprintf(p,"'%s' is a global alias for '%s'\n",
				cmd, list[i].alias);
			return COM_OK;
		}
	}

	/* or a personal alias */
	list = alias_list_personal(p, &n);
	for (i=0;i<n;i++) {
		if (strcasecmp(cmd, list[i].comm_name) == 0) {
			pprintf(p,"'%s' is a personal alias for '%s'\n",
				cmd, list[i].alias);
			return COM_OK;
		}
	}

	return help_page(p, help_dir[pp->language], cmd);
}

/*
  admin only help
 */
int com_ahelp(int p, param_list param)
{
	char *cmd;

	if (param[0].type == TYPE_STRING) {
		cmd = param[0].val.string;
	} else {
		cmd = "commands";
	}

	return help_page(p, ADHELP_DIR, cmd);
}

