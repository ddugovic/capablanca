/*
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

enum { SEEKOPEN = 0, SEEKCLOSED };

/* The values of the follwing enums cannot be changed as they will end up */
/* Being seen by the client- we can add values later but the ones */
/* Here, after being documented, have to be here forever */
enum { UNTIMED = 0, BLITZ, STANDARD, NONSTANDARD, WILD, LIGHTNING };

#undef BLACK
#undef WHITE
enum { BLACK = 0, WHITE};


static int get_empty_seekslot(void);
static char *form_ad(struct pending * ad, int i);

int com_seek(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	int             p1, count = 0;
	int             num;	/* sought ID */
	char           *msgtxt;
	char		board[100], category[100];
	int		wt, bt, winc, binc, rated, white;

	
	srandom(time(0));
#if 0
	if (!CheckPFlag(p, PFLAG_REG)) {
		pprintf(p, "Only registered players can use the seek command.\n");
		return COM_OK;
	}
#endif

	if ((pp->game >= 0) && ((game_globals.garray[pp->game].status == GAME_EXAMINE) || (game_globals.garray[pp->game].status == GAME_SETUP))) {
		pprintf(p, "ERROR: You can't seek a game while you are examining one.\n");
		return COM_OK;
	}
	if (pp->game >= 0) {
		pprintf(p, "ERROR: You can't seek a game while you are playing one.\n");
		return COM_OK;
	}
	num = get_empty_seekslot();
	if (num == -1) {
		pprintf(p, "ERROR: Sorry, all available slots for seek are closed.\n");
		return COM_OK;
	}
	seek_globals.ads[num].whofrom = p;

#if 1
	// [HGM] attempt to make seek as powerful as match
	wt = bt = winc = binc = -1;
	board[0] = category[0] = '\0';
	white = rated = -1;
	if (param[0].type != TYPE_NULL) {
	      if (!parse_match_string(p, &wt,&bt,&winc,&binc,&white,&rated,category,
	                                                  board,param[0].val.string))
	      return COM_OK; /* couldn't parse */
	}

    if (category[0]) {
      if (!board[0] && strcmp(category,"bughouse")) {
        pprintf(p, "You must specify a board and a category.\n");
        return COM_OK;

      } else if (board[0]) { /* not bughouse */
        char fname[MAX_FILENAME_SIZE];

        sprintf(fname, "%s/%s/%s", BOARD_DIR, category, board);
        if (!file_exists(fname)) {
          pprintf(p, "No such category/board: %s/%s\n", category, board);
          return COM_OK;
        }
      }
    }

	seek_globals.ads[num].status = SEEKCLOSED; // params are valid; create ad

	if(wt < 0) wt = pp->d_time; if(bt < 0) bt = wt;
	if(winc < 0) winc = pp->d_inc; if(binc < 0) binc = bt;
	seek_globals.ads[num].wtime = wt;
	seek_globals.ads[num].btime = bt;
	seek_globals.ads[num].winc = winc;
	seek_globals.ads[num].binc = binc;

	if (rated != -1) {
		if (!rated || !CheckPFlag(p, PFLAG_REG) || seek_globals.ads[num].wtime == 0)
			seek_globals.ads[num].rated = UNRATED;	/* unrated */
		else if (rated)
			seek_globals.ads[num].rated = RATED;	/* rated */
	} else
		seek_globals.ads[num].rated = BoolCheckPFlag(p, PFLAG_RATED);


	if (white != -1) {
		seek_globals.ads[num].seek_color = white ? WHITE : BLACK;
	} else
		seek_globals.ads[num].seek_color = random() % 2;

#else
	seek_globals.ads[num].status = SEEKCLOSED;

	if (param[0].type == TYPE_INT)
		seek_globals.ads[num].wtime = param[0].val.integer;	/* White time */
	else
		seek_globals.ads[num].wtime = pp->d_time;

	if (param[1].type == TYPE_INT)
		seek_globals.ads[num].winc = param[1].val.integer;	/* White increment */
	else
		seek_globals.ads[num].winc = pp->d_inc;

	seek_globals.ads[num].btime = seek_globals.ads[num].wtime;	/* Black time */
	seek_globals.ads[num].binc = seek_globals.ads[num].winc;	/* Black increment */

	if (param[2].type == TYPE_INT) {
		if (param[2].val.integer == UNRATED || !CheckPFlag(p, PFLAG_REG) || seek_globals.ads[num].wtime == 0)
			seek_globals.ads[num].rated = UNRATED;	/* unrated */
		else if (param[2].val.integer == RATED)
			seek_globals.ads[num].rated = RATED;	/* rated */
	} else
		seek_globals.ads[num].rated = BoolCheckPFlag(p, PFLAG_RATED);


	if (param[3].type == TYPE_INT) {
		if (param[3].val.integer == WHITE)
			seek_globals.ads[num].seek_color = WHITE;
		else if (param[3].val.word[0] == BLACK)
			seek_globals.ads[num].seek_color = BLACK;
	} else
		seek_globals.ads[num].seek_color = random() % 2;

	category[0]='\0';
	board[0]='\0';
	/* FIXME- use proper values */
#endif
	seek_globals.ads[num].category = strdup(category);
	seek_globals.ads[num].board_type = strdup(board);

	seek_globals.ads[num].next = NULL;

	msgtxt = form_ad(&seek_globals.ads[num], num);

	for (p1 = 0; p1 < player_globals.p_num; p1++) {
		if ((p1 == p) || !CheckPFlag(p1, PFLAG_ADS)
		  || (player_globals.parray[p1].status != PLAYER_PROMPT)
		  || (player_censored(p1, p)))
			continue;
		count++;
		pprintf(p1, "%s", msgtxt);
	}
	pprintf(p, "%s", msgtxt);
	free(msgtxt);

	return COM_OK;
}

int com_play(int p, param_list param)
{
	int             game;

	game = param[0].val.integer;

	if (seek_globals.ads[game].status != SEEKCLOSED) {
		pprintf(p, "ERROR: Either that game has already started or "
			"it is no longer available.\n");
		return COM_OK;
	}
	if (!CheckPFlag(p, PFLAG_REG) && seek_globals.ads[game].rated == 1) {
		pprintf(p, "ERROR: Unregistred users cannot play rated games.\n");
		return COM_OK;
	}
	if (seek_globals.ads[game].whofrom == p) {
		pprintf(p, "ERROR: You cannot respond to your own seek ads\n");
		return COM_OK;
	}
	seek_globals.ads[game].whoto = p;

	decline_withdraw_offers(p, -1, PEND_ALL, DO_DECLINE | DO_WITHDRAW); 
	decline_withdraw_offers(seek_globals.ads[game].whofrom, -1, PEND_ALL, DO_DECLINE | DO_WITHDRAW);

	accept_match(&seek_globals.ads[game], p, seek_globals.ads[game].whofrom);
#if 0
	free(seek_globals.ads[game].category);
	free(seek_globals.ads[game].board_type);
#endif
return 0;
}

extern FILE *comlog;

int com_sought(int p, param_list param)
{
	int             i;

	for (i = 0; i < seek_globals.max_ads; i++) {
		if (seek_globals.ads[i].status == SEEKCLOSED) {
			char *msgtxt = form_ad(&seek_globals.ads[i], i);
if(comlog) fprintf(comlog, "msgtext = %s\n", msgtxt), fflush(comlog);
			pprintf(p, "%s", msgtxt);
			free(msgtxt);
		}
	}
	return COM_OK;
}


int com_unseek(int p, param_list param)
{
	/* TODO: need to add parameters */
	withdraw_seeks(p);
	return COM_OK;
}

void withdraw_seeks(int p)
{
	int             i, p1;

	for (i = 0; i < seek_globals.max_ads; i++) {
		if (seek_globals.ads[i].whofrom == p && 
		    seek_globals.ads[i].status == SEEKCLOSED) {
			seek_globals.ads[i].status = SEEKOPEN;

			for (p1 = 0; p1 < player_globals.p_num; p1++) {
				if ((p1 == p) || (player_globals.parray[p1].status != PLAYER_PROMPT) ||
				    (player_censored(p1, p)) || !CheckPFlag(p1,PFLAG_ADS))
					continue;
				pprintf_prompt(p1, "AD_DESTROY: %d\n", i);
			}
			pprintf(p, "Ads removed: %d\n", i);
			FREE(seek_globals.ads[i].category);
			FREE(seek_globals.ads[i].board_type);
		}
	}
}

static int get_empty_seekslot(void)
{
	int i;

	for (i = 0; i < seek_globals.max_ads; i++) {
		if (seek_globals.ads[i].status != SEEKCLOSED) 
			return i;
	}

	if (seek_globals.max_ads >= config_get_int("MAX_SOUGHT", DEFAULT_MAX_SOUGHT)) {
		d_printf("Too many seeks! please raise MAX_SOUGHT\n");
		return -1;
	}

	seek_globals.max_ads += 100;
	seek_globals.ads = (struct pending *)realloc(seek_globals.ads,
						     seek_globals.max_ads * sizeof(struct pending));
	
	ZERO_STRUCT(seek_globals.ads[i]);
	return i;

}

static char *form_ad(struct pending * ad, int i)
{

	char           *final, buf[100];
	int             rating, total, type;

	total = ad->wtime * 60 + ad->winc * 40;

	if(total == 0) {
		type = UNTIMED;
		rating = player_globals.parray[ad->whofrom].l_stats.rating;
        } else if (total < 180) {
                type = LIGHTNING;
                rating = player_globals.parray[ad->whofrom].l_stats.rating;
        } else if (total >= 900) {
                type = STANDARD; 
                rating = player_globals.parray[ad->whofrom].s_stats.rating;
        } else {
                type = BLITZ;
                rating = player_globals.parray[ad->whofrom].b_stats.rating;
	}

if(comlog) fprintf(comlog, "seek %d type = %d\n", i, type), fflush(comlog);
	if(ad->category[0]) { // [HGM] print category with seek ad
		sprintf(buf, " %s", ad->category);
		if(ad->board_type[0] && strcmp(ad->board_type, "0"))
			sprintf(buf + strlen(buf), " %s", ad->board_type);
	} else strcpy(buf, TypeStrings[type]); // [HGM] replaced color by type here...

	asprintf(&final, "%3u %4u %-17s %3u %3u %-7s %-10s\n",
		 i, 
		 rating,
		 player_globals.parray[ad->whofrom].name, 
		 ad->wtime, 
		 ad->winc, 
		 ad->rated?"rated":"unrated",
		 buf);

	return final;
}
