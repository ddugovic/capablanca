/*
   Copyright (c) 1993 Richard V. Nash.
   Copyright (c) 2000 Dan Papasian
   Copyright (C) 2002 Andrew Tridgell
   Copyright (C) 2016 Daniel Dugovic
   
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

/* Configure file locations in this include file. */


#ifndef _CONFIG_H
#define _CONFIG_H
#include "autoconfig.h"

/* CONFIGURE THIS: The port on which the server binds */

#define DEFAULT_PORT 5000

/* Which is the default language for help files, see variable.h for the
    current available settings */

#define LANG_DEFAULT      LANG_ENGLISH


/* CONFIGURE THESE: Locations of the data, players, and games directories */
/* These must be absolute paths because some mail daemons may be called */
/* from outside the pwd */

#define MAX_ADVERTS -1
/* These small files are printed when users log in
   They can be used for advertisements or to promote server features
   MAX_ADVERTS = the number of adverts you have
   They should be put in the DEFAULT_ADVERTS directory
   and be labelled with the numbers 0 - MAX_ADVERTS-1 inclusive
   Eg if MAX_ADVERTS is 3 then the files are 0,1,2

   If you do not want to use the adverts feature set it to -1
*/

/* define the directory that will be the root directory of FICS */
#define FICSROOT CHESSDDIR "/"

#define CONFIG_DB         FICSROOT "config.tdb"
#define NEWS_DB           FICSROOT "news.tdb"
#define LIB_DIR           FICSROOT "lib"
#define ADVERT_DIR        FICSROOT "data/adverts"
#define MESS_DIR          FICSROOT "data/messages"
#define INDEX_DIR         FICSROOT "data/index"
#define HELP_DIR          FICSROOT "data/help"
#define HELP_SPANISH      FICSROOT "data/Spanish"
#define HELP_FRENCH       FICSROOT "data/French"
#define HELP_DANISH       FICSROOT "data/Danish"
#define INFO_DIR          FICSROOT "data/info"
#define ADHELP_DIR        FICSROOT "data/admin"
#define USCF_DIR          FICSROOT "data/uscf"
#define STATS_DIR         FICSROOT "data/stats"
#define SPOOL_DIR         FICSROOT "spool/mail.XXXXXX"
#define PLAYER_DIR        FICSROOT "players"
#define ADJOURNED_DIR     FICSROOT "games/adjourned"
#define HISTORY_DIR       FICSROOT "games/history"
#define JOURNAL_DIR       FICSROOT "games/journal"
#define BOARD_DIR         FICSROOT "data/boards"
#define LISTS_DIR         FICSROOT "data/lists"
#define BOOK_DIR          FICSROOT "data/book"
#define USAGE_DIR         FICSROOT "data/usage"
#define USAGE_SPANISH     FICSROOT "data/usage_spanish"
#define USAGE_FRENCH      FICSROOT "data/usage_french"
#define USAGE_DANISH      FICSROOT "data/usage_danish"

/* all of these can be overridden using the 'aconfig' command */
#define DEFAULT_MAX_PLAYER 5000
#define DEFAULT_MAX_STORED 50
#define DEFAULT_MAX_USER_LIST_SIZE 100
#define DEFAULT_MAX_ALIASES 20
#define DEFAULT_RATING 0
#define DEFAULT_RD 350
#define DEFAULT_TIME 2
#define DEFAULT_INCREMENT 12
#define DEFAULT_MAX_SOUGHT 1000
#define DEFAULT_IDLE_TIMEOUT 36000
#define DEFAULT_LOGIN_TIMEOUT 300
#define DEFAULT_GUEST_PREFIX_ONLY 0

#endif /* _CONFIG_H */
