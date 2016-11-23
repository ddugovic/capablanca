/*
   Copyright (c) 1993 Richard V. Nash.
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


#ifndef _COMMAND_LIST_H
#define _COMMAND_LIST_H

/*
  Parameter string format
  w - a word 
  o - an optional word 
  d - integer
  p - optional integer
  i - word or integer
  j - word or signed integer
  n - optional word or integer
  s - string to end
  t - optional string to end
  
  If the parameter option is given in lower case then the parameter is 
  converted to lower case before being passsed to the function. If it is
  in upper case, then the parameter is passed as typed.
 */

/* Try to keep this list in alpha order */

 /* Name	Options	Functions	Security */
static struct command_type command_list[] = {

  {"abort",             "",     com_abort,      ADMIN_USER },
  {"accept",		"n",	com_accept,	ADMIN_USER },
  {"addlist",           "ww",   com_addlist,    ADMIN_USER },
  {"adjourn",           "",     com_adjourn,    ADMIN_USER },
  {"alias",		"oT",	com_alias,	ADMIN_USER },
  {"allobservers",	"n",	com_allobservers,	ADMIN_USER },
  {"assess",		"noo",	com_assess,	ADMIN_USER },
  {"backward",          "p",    com_backward,   ADMIN_USER },
  {"bell",		"",	com_bell,	ADMIN_USER },
  {"best",		"o",	com_best,	ADMIN_USER },
  {"bname",	        "O",    com_bname,      ADMIN_USER },
  {"boards",		"o",	com_boards,	ADMIN_USER },
  {"bsetup",             "oT",  com_setup,      ADMIN_USER},
/*  {"channel",		"p",	com_channel,	ADMIN_USER }, */
  {"clearmessages",	"n",	com_clearmessages,	ADMIN_USER },
  {"clrsquare",         "w",    com_clrsquare,  ADMIN_USER },
  {"cshout",            "S",    com_cshout,     ADMIN_USER },
  {"date",		"",	com_date,	ADMIN_USER },
  {"decline",		"n",	com_decline,	ADMIN_USER },
  {"draw",		"",	com_draw,	ADMIN_USER },
  {"eco",               "n",    com_eco,        ADMIN_USER }, 
  {"examine",           "ono",  com_examine,    ADMIN_USER },
  {"finger",		"oo",	com_stats,	ADMIN_USER },
  {"flag",		"",	com_flag,	ADMIN_USER },
  {"flip",		"",	com_flip,	ADMIN_USER },
  {"fmessage",          "wd",   com_forwardmess, ADMIN_USER },
  {"forward",           "p",    com_forward,    ADMIN_USER },
  {"games",		"o",	com_games,	ADMIN_USER },
  {"getgi",             "w",    com_getgi,      ADMIN_USER },
  {"getpi",             "w",    com_getpi,      ADMIN_USER },
  {"goboard",           "i",    com_goboard,    ADMIN_USER },
  {"handles",           "w",    com_handles,    ADMIN_USER },
  {"hbest",             "o",    com_hbest,      ADMIN_USER },
  {"help",		"t",	com_help,	ADMIN_USER },
  {"history",		"o",	com_history,	ADMIN_USER },
  {"hrank",		"oo",	com_hrank,	ADMIN_USER },
  {"inchannel",		"n",	com_inchannel,	ADMIN_USER },
  {"iset",		"wS",	com_iset,	ADMIN_USER },
  {"it", 		"T",	com_it,		ADMIN_USER },
  {"jkill",             "w",    com_jkill,      ADMIN_USER },
  {"journal",		"o",	com_journal,	ADMIN_USER },
  {"jsave",		"wwi",	com_jsave,	ADMIN_USER },
  {"kibitz",		"S",	com_kibitz,	ADMIN_USER },
  {"limits",            "",     com_limits,     ADMIN_USER },  
  {"llogons",           "",     com_llogons,    ADMIN_USER },
  {"logons",		"o",	com_logons,	ADMIN_USER },
  {"mailmoves",		"n",	com_mailmoves,	ADMIN_USER },
  {"mailoldmoves",	"o",	com_mailoldmoves, ADMIN_USER },
  {"mailstored",	"wi",	com_mailstored,	ADMIN_USER },
  {"match",		"wt",	com_match,	ADMIN_USER },
  {"rmatch",		"wwt",	com_rmatch,	ADMIN_USER },
  {"messages",		"nT",	com_messages,	ADMIN_USER },
  {"mexamine",          "w",    com_mexamine,   ADMIN_USER },
  {"moretime",          "d",    com_moretime,   ADMIN_USER },
  {"moves",		"n",	com_moves,	ADMIN_USER },
  {"news",		"o",	com_news,	ADMIN_USER },
  {"next", 		"",	com_more, 	ADMIN_USER },
  {"observe",		"n",	com_observe,	ADMIN_USER },
  {"oldmoves",		"o",	com_oldmoves,	ADMIN_USER },
  {"open",		"",	com_open,	ADMIN_USER },
  {"partner",		"o",	com_partner,	ADMIN_USER },
  {"password",		"WW",	com_password,	ADMIN_USER },
  {"pause",		"",	com_pause,	ADMIN_USER },
  {"play",		"d",	com_play,	ADMIN_USER },
  {"pending",		"",	com_pending,	ADMIN_USER },
  {"prefresh",		"",	com_prefresh,	ADMIN_USER },
  {"promote",		"w",	com_promote,	ADMIN_USER },
  {"ptell",		"S",	com_ptell,	ADMIN_USER },
  {"ptime",             "",     com_ptime,      ADMIN_USER },
  {"qtell",             "iS",   com_qtell,      ADMIN_USER },
  {"quit",		"",	com_quit,	ADMIN_USER },
  {"rank",		"oo",	com_rank,	ADMIN_USER },
  {"refresh",		"n",	com_refresh,	ADMIN_USER },
  {"revert",            "",     com_revert,     ADMIN_USER },
  {"resign",		"o",	com_resign,	ADMIN_USER },
  {"say",		"S",	com_say,	ADMIN_USER },
//  {"seek",		"pppp",	com_seek,	ADMIN_USER },
  {"seek",		"t",	com_seek,	ADMIN_USER }, // [HGM]
  {"unseek",		"",	com_unseek,	ADMIN_USER },
  {"set",		"wT",	com_set,	ADMIN_USER },
  {"shout",		"T",	com_shout,	ADMIN_USER },
  {"showlist",          "o",    com_showlist,   ADMIN_USER },
  {"simabort",		"",	com_simabort,	ADMIN_USER },
  {"simallabort",	"",	com_simallabort,ADMIN_USER },
  {"simadjourn",	"",	com_simadjourn,	ADMIN_USER },
  {"simalladjourn",	"",	com_simalladjourn,ADMIN_USER },
  {"simgames",		"o",	com_simgames,	ADMIN_USER },
  {"simmatch",		"woo",	com_simmatch,	ADMIN_USER },
  {"simnext",		"",	com_simnext,	ADMIN_USER },
  {"simopen",		"",	com_simopen,	ADMIN_USER },
  {"simpass",		"",	com_simpass,	ADMIN_USER },
  {"simprev",           "",     com_simprev,    ADMIN_USER },
  {"sought",		"p",	com_sought,	ADMIN_USER },
  {"smoves",		"wj",	com_smoves,	ADMIN_USER },
  {"sposition",		"ww",	com_sposition,	ADMIN_USER },
  {"statistics",	"",	com_statistics,	ADMIN_USER },
  {"stored",		"o",	com_stored,	ADMIN_USER },
  {"style",		"d",	com_style,	ADMIN_USER },
  {"sublist",           "ww",   com_sublist,    ADMIN_USER },
  {"switch",		"",	com_switch,	ADMIN_USER },
  {"takeback",		"p",	com_takeback,	ADMIN_USER },
  {"tell",		"nS",	com_tell,	ADMIN_USER },
  {"ltell",		"wS",	com_ltell,	ADMIN_USER },
  {"time",		"n",	com_time,	ADMIN_USER },
  {"tomove",            "w",    com_tomove,     ADMIN_USER },
  {"toggle",            "ww",   com_togglelist, ADMIN_USER },
  {"tournset",          "wd",   com_tournset,   ADMIN_USER },
  {"unalias",		"w",	com_unalias,	ADMIN_USER },
  {"unexamine",         "",     com_unexamine,  ADMIN_USER },
  {"unobserve",		"n",	com_unobserve,	ADMIN_USER },
  {"unpause",		"",	com_unpause,	ADMIN_USER },
  {"uptime",		"",	com_uptime,	ADMIN_USER },
  {"variables",		"o",	com_variables,	ADMIN_USER },
  {"whenshut",		"",	com_whenshut,	ADMIN_USER },
  {"whisper",		"S",	com_whisper,	ADMIN_USER },
  {"who",               "T",    com_who,        ADMIN_USER },
  {"withdraw",		"n",	com_withdraw,	ADMIN_USER },
  {"wname",             "O",    com_wname,      ADMIN_USER },
  {"xtell",             "wS",   com_xtell,      ADMIN_USER },
  {"znotify",		"",	com_znotify,	ADMIN_USER },

#if 0
  {"ustat",		"",	com_ustat,	ADMIN_USER },
#endif

#if 0
  {"reply",		"S",	com_reply,	ADMIN_USER },
#endif
  {"pstat",		"oo",	com_pstat,	ADMIN_USER },
  {"remote",		"wS",	com_remote,	ADMIN_USER }, 

  {"aconfig",	        "OS",	com_aconfig,       ADMIN_GOD },
  {"addcomment",	"WS",	com_addcomment,    ADMIN_ADMIN },
  {"addplayer",		"WWS",	com_addplayer,	   ADMIN_ADMIN },
  {"adjudicate",	"www",	com_adjudicate,	   ADMIN_ADMIN },
  {"adump",	        "",	com_adump,	   ADMIN_GOD },
  {"admin",             "",     com_admin,         ADMIN_ADMIN },
  {"ahelp",		"t",	com_ahelp,	   ADMIN_ADMIN },
  {"acheckhelp",	"",	com_acheckhelp,	   ADMIN_ADMIN },
  {"announce",		"S",	com_announce,      ADMIN_ADMIN },
  {"annunreg",          "S",    com_annunreg,      ADMIN_ADMIN },
  {"anews",		"o",	com_anews,	   ADMIN_ADMIN },
  {"areload",           "",     com_areload,       ADMIN_GOD },
  {"asetv",             "wS",   com_asetv,         ADMIN_ADMIN },
  {"asetadmin",         "wd",   com_asetadmin,     ADMIN_ADMIN },
  {"asetblitz",         "wdpppp",com_asetblitz,    ADMIN_ADMIN },
  {"asetbughouse",      "wdpppp",com_asetbug,      ADMIN_ADMIN },
  {"asetemail",		"wO",	com_asetemail,	   ADMIN_ADMIN },
  {"asethandle",        "WW",   com_asethandle,    ADMIN_ADMIN },
  {"asetlight",         "wdpppp",com_asetlight,    ADMIN_ADMIN },
  {"asetpasswd",        "wW",   com_asetpasswd,    ADMIN_ADMIN },
  {"asetrealname",      "wT",   com_asetrealname,  ADMIN_ADMIN },
  {"asetstd",           "wdpppp",com_asetstd,      ADMIN_ADMIN },
  {"asetwild",          "wdpppp",com_asetwild,     ADMIN_ADMIN },
  {"cnewsi",            "S",     com_cnewsi,       ADMIN_ADMIN },
  {"cnewsf",            "dT",   com_cnewsf,        ADMIN_ADMIN },
  {"cnewst",            "dS",   com_cnewst,        ADMIN_ADMIN },
  {"cnewsp",            "d",    com_cnewsp,        ADMIN_ADMIN },
  {"cnewse",            "dp",   com_cnewse,        ADMIN_ADMIN },
  {"cnewsd",            "d",    com_cnewsd,        ADMIN_ADMIN },
  {"canewsi",            "S",    com_canewsi,       ADMIN_ADMIN },
  {"canewsf",            "dT",   com_canewsf,        ADMIN_ADMIN },
  {"canewst",            "dS",   com_canewst,        ADMIN_ADMIN },
  {"canewsp",            "d",    com_canewsp,        ADMIN_ADMIN },
  {"canewse",            "dp",   com_canewse,        ADMIN_ADMIN },
  {"canewsd",            "d",    com_canewsd,        ADMIN_ADMIN },
  {"ftell",             "o",    com_ftell,         ADMIN_ADMIN },
  {"hideinfo",          "",     com_hideinfo,      ADMIN_ADMIN },
  {"nuke",              "w",    com_nuke,          ADMIN_ADMIN },
  {"pose",		"wS",	com_pose,	   ADMIN_GOD   }, 
  {"quota",             "p",    com_quota,         ADMIN_ADMIN },
  {"raisedead",         "WO",   com_raisedead,     ADMIN_ADMIN },
  {"remplayer",         "w",    com_remplayer,     ADMIN_ADMIN },
  {"rerank",		"w",	com_fixrank,	   ADMIN_ADMIN },
  {"showcomment",	"w",	com_showcomment,   ADMIN_ADMIN },
  {"shutdown",          "oT",    com_shutdown,      ADMIN_ADMIN },
  {"summon",            "w",	com_summon,        ADMIN_ADMIN },

  {NULL, NULL, NULL, ADMIN_USER}
};

static struct alias_type g_alias_list[] = {
  {"comment",   "addcomment"},
  {"setup",	"bsetup"},
  {"sc",        "showcomment"},
  {"b",		"backward"},
  {"w",		"who"},
  {"t",		"tell"},
  {"m",		"match"},
  {"go",        "goboard"},
  {"goto",      "goboard"},
  {"gonum",     "goboard"}, /* dumping an obsolete command */
  {"f",		"finger"},
  {"hi",        "history"},
  {"a",		"accept"},
  {"saa",       "simallabort"},
  {"saab",	"simallaabort"},
  {"sab",       "simabort"},
  {"sadj",      "simadjourn"},
  {"saadj",     "simalladjourn"},
  {"sh",	"shout"},
  {"sn",	"simnext"},
  {"sp",        "simprev"},
  {"vars",	"variables"},
  {"g",		"games"},
  {"players",	"who a"},
  {"player",   "who a"},
  {"p",		"who a"},
  {"pl",	"who a"},
  {"o",		"observe"},
  {"r",		"refresh"},
  {"re",        "refresh"}, /* So r/re doesn't resign! */
  {"ch",	"channel"},
  {"in",	"inchannel"},
  {".",		"tell ."},
  {",",		"tell ,"},
  {"`",         "tell ."},
  {"!",		"shout"},
  {"I",		"it"},
  {"i",		"it"},
  {":",		"it"},
  {"exit",	"quit"},
  {"logout",	"quit"},
  {"bye",       "quit"},
  {"*",      	"kibitz"},
  {"#",         "whisper"},
  {"ma",	"match"},
  {"more",      "next"},
  {"n",         "next"},
  {"znotl",     "znotify"},
  {"+",         "addlist"},
  {"-",         "sublist"},
  {"=",         "showlist"},
  {"mam",     "xtell mamer"},
  {"tom",    "tell tomato"},
  {"egg",     "tell eggoi"},
  {"follow",  "toggle follow"},
  {"channel", "toggle channel"},
  {NULL, NULL}
};

#endif /* _COMMAND_LIST_H */
