/*
  global configuration option handler for lasker chess server

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

/* database for holding config info */
static TDB_CONTEXT *config_db;

static int config_set(const char *name, char *value);

/*
  tdb logging function, just in case something goes wrong we log database errors to stderr
 */
static void tdb_log_fn(TDB_CONTEXT *t, int level, const char *format, ...)
{
	va_list ap;
	
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}


/* 
   initialise the config database with some default values. This is only called once
   on initial installation
   returns 0 on success
 */
static int config_init(void)
{
	config_db = tdb_open_ex(CONFIG_DB, 0, 0, O_RDWR|O_CREAT, 0600, tdb_log_fn);
	if (!config_db) {
		d_printf("Failed to create the config database!\n");
		return -1;
	}

	/* a few important defaults */
	config_set("DEFAULT_PROMPT", "fics% ");
	config_set("GUEST_LOGIN", "guest");

	/* this allows an initial admin connection */
	return config_set("HEAD_ADMIN", "admin");
}

/* close the config database */
void config_close(void)
{
	if (!config_db) return;
	tdb_close(config_db);
	config_db = NULL;
}

/* 
   open the config database
   returns 0 on success
 */
int config_open(void)
{
	config_db = tdb_open_ex(CONFIG_DB, 0, 0, O_RDWR, 0600, tdb_log_fn);
	if (!config_db && errno == ENOENT) {
		return config_init();
	}
	if (!config_db) {
		return -1;
	}
	return 0;
}

/* 
   fetch a value from the config database - caller must free 
*/
const char *config_get(const char *name)
{
	TDB_DATA data;
	TDB_DATA key;

	key.dptr = name;
	key.dsize = strlen(name)+1;

	data = tdb_fetch(config_db, key);

	/* this trick allows config variables to show up in 'aconfig' as
	   soon as they are used */
	if (!data.dptr && config_set(name, "NOT_CONFIGURED") == 0) {
		data = tdb_fetch(config_db, key);
	}

	return data.dptr;
}


/* 
   fetch an integer value from the config database, with a default value
*/
int config_get_int(const char *name, int default_v)
{
	TDB_DATA data;
	TDB_DATA key;
	int v;

	key.dptr = name;
	key.dsize = strlen(name)+1;

	data = tdb_fetch(config_db, key);

	/* this trick allows config variables to show up in 'aconfig' as
	   soon as they are used */
	if (!data.dptr) {
#if 1
		/* somehow this prevents a segmentation fault */
		printf("%s=%d\n", name, default_v);
#endif
		char *s = NULL;
		data.dsize = asprintf(&s, "%d", default_v) + 1;
		data.dptr = s;
		tdb_store(config_db, key, data, TDB_REPLACE);
	}

	v = atoi(data.dptr);
	free(data.dptr);

	return v;
}


/* 
   set a configration variable. 
   Returns 0 on success 
*/
static int config_set(const char *name, char *value)
{
	TDB_DATA data;
	TDB_DATA key;
	int ret;

	key.dptr = name;
	key.dsize = strlen(name)+1;
	
	if (strcmp(value, "-") == 0) {
		return tdb_delete(config_db, key);
	}

	data.dptr = value;
	data.dsize = strlen(value)+1;
  
	ret = tdb_store(config_db, key, data, TDB_REPLACE);
	return ret;
}

/* 
   fetch a value from the config database - will be auto-freed on a future call
   to config_get_tmp()
*/
const char *config_get_tmp(const char *name)
{
	static const char *ret[10];
	static unsigned idx;
	const char **p = &ret[idx];
	idx = (idx+1) % 10;
	if (*p) {
		free(*p);
	}
	*p = config_get(name);
	return *p;
}


/*
  dump the current config
 */
static void config_dump(int p)
{
	TDB_DATA data1, data2;
  
	/* looping using firstkey/nextkey is a pain ... */
	for (data1 = tdb_firstkey(config_db); 
	     data1.dptr; 
	     data2 = data1, 
		     data1 = tdb_nextkey(config_db, data2), 
		     free(data2.dptr)) {
		pprintf(p, "%s = %s\n", data1.dptr, config_get_tmp(data1.dptr));
	}
}

/*
 * aconfig
 *
 * Usage: aconfig variable value
 *
 *   sets the config variable 'variable' to 'value'
 */
int com_aconfig(int p, param_list param)
{
	if (param[0].type == TYPE_NULL) {
		/* if no parameters are given then dump the current config */
		config_dump(p);
		return COM_OK;
	}

	if (config_set(param[0].val.word, param[1].val.string) != 0) {
		return COM_FAILED;
	}
  
	return COM_OK;
}
