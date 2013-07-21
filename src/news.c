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
  support for the 'news' command
 */

#include "includes.h"

static TDB_CONTEXT *news_db;

/* close the news database */
void news_close(void)
{
	if (!news_db) return;
	tdb_close(news_db);
	news_db = NULL;
}

/* 
   open the news database
   returns 0 on success
 */
int news_open(void)
{
	news_db = tdb_open(NEWS_DB, 0, 0, O_RDWR|O_CREAT, 0600);
	if (!news_db) {
		d_printf("Unable to open news database!\n");
		return -1;
	}
	return 0;
}

/*
  free up a news item
*/
static void news_free(struct news *news)
{
	FREE(news->poster);
	FREE(news->title);
	FREE(news->message);
}

/*
  return the maximum news item number
*/
static int news_max(int admin)
{
	return tdb_get_int(news_db, admin?"ADMIN_MAXNUM":"MAXNUM", 0);
}

/*
  save a news item
*/
static int news_save(int num, struct news *news, int admin)
{
	const char *s;
	char *name;

	/* marshall it into a string */
	s = marshall_news(news);
	if (!s) {
		return -1;
	}

	asprintf(&name, "%s%u", admin?"ADMIN/":"", num);
	if (tdb_set_string(news_db, name, s) != 0) {
		free(name);
		free(s);
		return -1;
	}

	free(name);
	free(s);

	if (news_max(admin) < num) {
		tdb_set_int(news_db, admin?"ADMIN_MAXNUM":"MAXNUM", num);
	}

	return 0;
}


/*
  load a news item
*/
static int news_load(int num, struct news *news, int admin)
{
	const char *s;
	char *name;
	int ret;

	asprintf(&name, "%s%u", admin?"ADMIN/":"", num);
	s = tdb_get_string(news_db, name);
	if (!s) {
		free(name);
		return -1;
	}
	free(name);

	ret = unmarshall_news(news, s);
	free(s);
	return ret;
}

/*
  delete a news item
*/
static int news_delete(int p, int num, int admin)
{
	char *name;

	asprintf(&name, "%s%u", admin?"ADMIN/":"", num);
	if (tdb_delete_string(news_db, name) == 0) {
		pprintf(p,"Deleted news item %u\n", num);
	} else {
		pprintf(p,"Unable to delete news item %u !\n", num);
	}
	free(name);
	return 0;
}


/*
  summarise the news
 */
static int news_summary(int p, int first, int how_many, int admin)
{
	int i;
	if (first < 1) {
		first = 1;
	}
	for (i=first; i< first+how_many; i++) {
		struct news news;
		if (news_load(i, &news, admin) == 0) {
			pprintf(p, "%4u (%s) %s\n", 
				i, 
				strday(&news.post_date), 
				news.title);
			news_free(&news);
		}
	}

	return COM_OK;
}


/*
  lookup a news item
 */
static int do_news(int p, param_list param, int admin)
{
	int num;
	struct news news;

	if (param[0].type == TYPE_NULL) {
		return news_summary(p, news_max(admin)-9, 10, admin);
	}
	if (strcasecmp(param[0].val.word, "all") == 0) {
		return news_summary(p, 1, news_max(admin), admin);
	}

	num = atoi(param[0].val.word);
	if (num <= 0) {
		pprintf(p, "Bad news item number '%s'\n", param[0].val.word);
		return COM_OK;
	}

	if (news_load(num, &news, admin) != 0) {
		pprintf(p, "News item %u not found\n", num);
		return COM_OK;
	}

	pprintf_more(p,"%4u (%s) %s\n\n%s\n\nPosted by %s.\n", 
		     num, 
		     strday(&news.post_date), 
		     news.title,
		     news.message,
		     news.poster);

	news_free(&news);

	return COM_OK;
}

/*
  lookup a news item
 */
int com_news(int p, param_list param)
{
	return do_news(p, param, 0);
}


/*
  lookup a admin news item
 */
int com_anews(int p, param_list param)
{
	return do_news(p, param, 1);
}


/*
  create a new news item
*/
static int cnewsi(int p, const char *title, int admin)
{
	struct player *pp = &player_globals.parray[p];
	struct news news;
	int ret, num;

	ZERO_STRUCT(news);
	news.poster = strdup(pp->name);
	news.post_date = time(NULL);
	news.message = strdup("");
	news.title = strndup(title, 45);

	num = news_max(admin) + 1;
	ret = news_save(num, &news, admin);
	
	if (ret == 0) {
		pprintf(p,"Created news item %u\n", num);
	} else {
		pprintf(p,"Failed to create news item %u !\n", num);
	}

	news_free(&news);
	return ret;
}

/*
  add to a news item
*/
static int cnewsf(int p, int num, const char *message, int admin)
{
	struct news news;
	char *msg;

	if (news_load(num, &news, admin) != 0) {
		pprintf(p, "News item %u not found\n", num);
		return -1;
	}

	asprintf(&msg, "%s%s\n", news.message, message);
	FREE(news.message);
	news.message = msg;

	if (news_save(num, &news, admin) != 0) {
		pprintf(p, "News item %u could not be updated !\n", num);
		news_free(&news);
		return -1;
	}

	pprintf(p, "News item %u updated\n", num);

	news_free(&news);
	return 0;
}

/*
  set title on a news item
*/
static int cnewst(int p, int num, const char *title, int admin)
{
	struct news news;

	if (news_load(num, &news, admin) != 0) {
		pprintf(p, "News item %u not found\n", num);
		return -1;
	}

	FREE(news.title);
	news.title = strndup(title, 45);

	if (news_save(num, &news, admin) != 0) {
		pprintf(p, "News item %u could not be updated !\n", num);
		news_free(&news);
		return -1;
	}

	pprintf(p, "News item %u updated\n", num);

	news_free(&news);
	return 0;
}

/*
  set poster on a news item
*/
static int cnewsp(int p, int num, int admin)
{
	struct player *pp = &player_globals.parray[p];
	struct news news;

	if (news_load(num, &news, admin) != 0) {
		pprintf(p, "News item %u not found\n", num);
		return -1;
	}

	FREE(news.poster);
	news.poster = strdup(pp->name);

	if (news_save(num, &news, admin) != 0) {
		pprintf(p, "News item %u could not be updated !\n", num);
		news_free(&news);
		return -1;
	}

	pprintf(p, "News item %u updated\n", num);

	news_free(&news);
	return 0;
}

/*
  delete last line of an item
*/
static int cnewsd(int p, int num, int admin)
{
	struct news news;
	char *s;

	if (news_load(num, &news, admin) != 0) {
		pprintf(p, "News item %u not found\n", num);
		return -1;
	}

	news.message[strlen(news.message)-1] = 0;
	s = strrchr(news.message, '\n');
	if (s) {
		s[1] = 0;
	} else {
		news.message[0] = 0;
	}

	if (news_save(num, &news, admin) != 0) {
		pprintf(p, "News item %u could not be updated !\n", num);
		news_free(&news);
		return -1;
	}

	pprintf(p, "News item %u updated\n", num);

	news_free(&news);
	return 0;
}


/*
  set expiry on a news item
*/
static int cnewse(int p, int num, int expiry, int admin)
{
	struct news news;

	if (news_load(num, &news, admin) != 0) {
		pprintf(p, "News item %u not found\n", num);
		return -1;
	}

	if (expiry == -1) {
		news_free(&news);
		return news_delete(p, num, admin);
	} else if (expiry == 0) {
		news.expires = 0;
	} else {
		news.expires = time(NULL) + (expiry * 24*60*60);
	}

	if (news_save(num, &news, admin) != 0) {
		pprintf(p, "News item %u could not be updated !\n", num);
		news_free(&news);
		return -1;
	}

	pprintf(p, "News item %u updated\n", num);

	news_free(&news);
	return 0;
}

/*
  create a normal news item
*/
int com_cnewsi(int p, param_list param)
{
	cnewsi(p, param[0].val.string, 0);
	return COM_OK;
}

/*
  add to a normal news item
*/
int com_cnewsf(int p, param_list param)
{
	cnewsf(p, param[0].val.integer, 
	       param[1].type != TYPE_NULL?param[1].val.string:"", 
	       0);
	return COM_OK;
}

/*
  set title on a normal news item
*/
int com_cnewst(int p, param_list param)
{
	cnewst(p, param[0].val.integer, param[1].val.string, 0);
	return COM_OK;
}

/*
  set poster on a normal news item
*/
int com_cnewsp(int p, param_list param)
{
	cnewsp(p, param[0].val.integer, 0);
	return COM_OK;
}

/*
  set expiry on a normal news item
*/
int com_cnewse(int p, param_list param)
{
	cnewse(p, 
	       param[0].val.integer, 
	       param[1].type != TYPE_NULL?param[1].val.integer:-1, 
	       0);
	return COM_OK;
}

/*
  delete last line on a normal news item
*/
int com_cnewsd(int p, param_list param)
{
	cnewsd(p, param[0].val.integer, 0);
	return COM_OK;
}

/*
  create a admin news item
*/
int com_canewsi(int p, param_list param)
{
	cnewsi(p, param[0].val.string, 1);
	return COM_OK;
}

/*
  add to a admin news item
*/
int com_canewsf(int p, param_list param)
{
	cnewsf(p, param[0].val.integer, 
	       param[1].type != TYPE_NULL?param[1].val.string:"", 
	       1);
	return COM_OK;
}

/*
  set title on a admin news item
*/
int com_canewst(int p, param_list param)
{
	cnewst(p, param[0].val.integer, param[1].val.string, 1);
	return COM_OK;
}

/*
  set poster on a admin news item
*/
int com_canewsp(int p, param_list param)
{
	cnewsp(p, param[0].val.integer, 1);
	return COM_OK;
}

/*
  set expiry on a admin news item
*/
int com_canewse(int p, param_list param)
{
	cnewse(p, param[0].val.integer, 
	       param[1].type != TYPE_NULL?param[1].val.integer:-1, 
	       1);
	return COM_OK;
}

/*
  delete last line on a admin news item
*/
int com_canewsd(int p, param_list param)
{
	cnewsd(p, param[0].val.integer, 1);
	return COM_OK;
}


/*
  tell user about new news
*/
void news_login(int p)
{
	struct player *pp = &player_globals.parray[p];
	int max;

	max = news_max(0);
	if (max > pp->latest_news) {
		pprintf(p,"\nThere are %u new news items since your last login\n", 
			max - pp->latest_news);
		pp->latest_news = max;
	}

	if (!check_admin(p, ADMIN_ADMIN)) return;

	max = news_max(1);
	if (max > pp->admin_latest_news) {
		pprintf(p,"\nThere are %u new admin news items since your last login\n", 
			max - pp->admin_latest_news);
		pp->admin_latest_news = max;
	}
}
