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
  automatic marshalling/unmarshalling system for C structures
*/

#define EXTERN
#include "includes.h"
#include "parsers/parse_info.h"

/* load all the globals */
void load_all_globals(const char *fname)
{
	struct all_globals a;
	char *s;

	s = file_load(fname, NULL);
	if (!s) {
		d_printf("Unable to load globals!\n");
		return;
	}
	memset(&a, 0, sizeof(a));
	gen_parse(pinfo_all_globals, (char *)&a, s);
	free(s);

	net_globals = *a.net_globals;
	game_globals = *a.game_globals;
	player_globals = *a.player_globals;
	command_globals = *a.command_globals;
	gics_globals = *a.gics_globals;
	timeseal_globals = *a.timeseal_globals;

	FREE(a.net_globals);
	FREE(a.game_globals);
	FREE(a.player_globals);
	FREE(a.command_globals);
	FREE(a.gics_globals);
	FREE(a.timeseal_globals);
}

/* save all the globals */
void save_all_globals(const char *fname)
{
	struct all_globals a, a2;
	char *s, *s2;

	memset(&a, 0, sizeof(a));
	a.net_globals = &net_globals;
	a.game_globals = &game_globals;
	a.player_globals = &player_globals;
	a.command_globals = &command_globals;
	a.gics_globals = &gics_globals;
	a.timeseal_globals = &timeseal_globals;

	s = gen_dump(pinfo_all_globals, (char *)&a, 0);
	if (!s) {
		d_printf("Unable to dump globals!\n");
		return;
	}
	file_save(fname, s, strlen(s));
	memset(&a2, 0, sizeof(a2));
	gen_parse(pinfo_all_globals, (char *)&a2, s);
	s2 = gen_dump(pinfo_all_globals, (char *)&a2, 0);
	if (strcmp(s, s2)) {
		d_printf("ERROR: globals parse mismatch!\n");
		file_save("cmp.dat", s2, strlen(s2));
	}
	free(s);
	free(s2);
}

/*
 * adump - dump complete server state to globals.dat
 */
int com_adump(int p, param_list param)
{
	save_all_globals("globals.dat");
	return COM_OK;
}


/*
  marshall a player structure
*/
const char *marshall_player(const struct player *pp)
{
	return gen_dump(pinfo_player, (const char *)pp, 0);
}

/*
  unmarshall a player structure
*/
int unmarshall_player(struct player *pp, const char *s)
{
	return gen_parse(pinfo_player, (char *)pp, s);
}


/*
  marshall a game structure
*/
const char *marshall_game(const struct game *gg)
{
	return gen_dump(pinfo_game, (const char *)gg, 0);
}

/*
  unmarshall a game structure
*/
int unmarshall_game(struct game *gg, const char *s)
{
	return gen_parse(pinfo_game, (char *)gg, s);
}

/*
  marshall a news structure
*/
const char *marshall_news(const struct news *nn)
{
	return gen_dump(pinfo_news, (const char *)nn, 0);
}

/*
  unmarshall a news structure
*/
int unmarshall_news(struct news *nn, const char *s)
{
	return gen_parse(pinfo_news, (char *)nn, s);
}



/*
  custom dump/load functions
*/
int gen_dump_struct_in_addr(struct parse_string *p, const char *ptr, unsigned indent)
{
	return gen_addgen(p, "%s", inet_ntoa(*(struct in_addr *)ptr));
}

int gen_parse_struct_in_addr(char *ptr, const char *str)
{
	struct in_addr *a = (struct in_addr *)ptr;
	if (inet_aton(str, a) == 0) {
		/* probably an old style */
		a->s_addr = atoi(str);
	}
	return 0;
}
