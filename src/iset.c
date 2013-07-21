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


#include "includes.h"

/*
  set a boolean ivar
*/
static int iset_bool(int p, unsigned *v, const char *var, const char *value)
{
	if (strcmp(value, "1") == 0) {
		*v = 1;
		pprintf(p, "%s set\n", var);
	} else if (strcmp(value, "0") == 0) {
		*v = 0;
		pprintf(p, "%s unset\n", var);
	} else {
		pprintf(p, "Bad value '%s' given for ivariable '%s'.\n", value, var);
	}
	return COM_OK;
}

/*
  support the iset command
*/
int com_iset(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	char *var = param[0].val.word;
	char *value = param[1].val.string;
	struct ivariables *iv;

	iv = &pp->ivariables;

	if (iv->lock) {
		pprintf(p,"Cannot alter: Interface setting locked.\n");
		return COM_OK;
	}

	if (strcasecmp(var,"ms") == 0) {
		return iset_bool(p, &iv->ms, var, value);
	} else if (strcasecmp(var,"lock") == 0) {
		return iset_bool(p, &iv->lock, var, value);
	} else if (strcasecmp(var,"startpos") == 0) {
		return iset_bool(p, &iv->startpos, var, value);
	} else {
		pprintf(p,"No such ivariable \"%s\".\n", var);
	}

	return COM_OK;
}
