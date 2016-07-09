//--------------------------------------------------------------------------
// command_list.h - List of all the commands for mamer and their respective
//                  parameter lists.
//
// Matthew E. Moses & Michael A. Long
//
// $Revision: 1.2 $
// $Date: 1997/10/08 21:03:08 $
//
// $Author: chess $
// $Locker:  $
//
// $Log: command_list.h,v $
// Revision 1.2  1997/10/08 21:03:08  chess
// no log message
//
// Revision 1.1  1997/04/08 23:12:33  chess
// Initial revision
//
//
//--------------------------------------------------------------------------

#ifndef _COMLIST_
#define _COMLIST_

/*
  Parameter string format
  w - a word 
  o - an optional word 
  d - integer
  p - optional integer
  i - word or integer
  n - optional word or integer
  s - string to end
  t - optional string to end
  
  If the parameter option is given in lower case then the parameter is 
  converted to lower case before being passsed to the function. If it is
  in upper case, then the parameter is passed as typed.
  */


/***********************
 *   Command List      *
 ***********************/
t_cmds commands[] = {
  {"abort",       "d",      AbortTourney,          MANAGER },
  {"addabuse",    "w",      AddAbuse,              MANAGER },
  {"announce",    "d",      AnnounceTourney,       MANAGER },
  {"best",        "",       ListBest,              USER },
  {"close",       "d",      CloseTourney,          MANAGER },
  {"finger",      "o",      FingerUser,            USER },
  {"forfeit",     "wd",     ForfeitUser,           MANAGER },
  {"games",       "d",      ListGamesInTourney,    USER },
  {"grid",        "d",      ListGridForTourney,    USER },
  {"help",        "o",      Help,                  USER },
  {"info",        "p",      ListTourneyInfo,       USER },
  {"join",        "d",      JoinTourney,           USER },
  {"last",        "",       LastFinishedTourneys,  USER },
  {"load",        "o",      LoadSavedTourney,      MANAGER },
  {"loadedusers", "",       LoadedUsers,           MANAGER },
  {"manage",      "d",      ManageTourney,         MANAGER },
  {"managers",    "",       ListManagers,          USER },
  {"messman",     "",       MessageManagers,       SUPERMANAGER },
  {"newmgr",      "w",      AddManagerToList,      SUPERMANAGER },
  {"open",        "d",      OpenTourney,           MANAGER },
  {"pairings",    "d",      PairingsInTourney,     USER },
  {"players",     "d",      ListPlayersInTourney,  USER },
  {"pose",        "wT",     PoseAsMamer,           PRESIDENT },
  {"quit",        "",       Shutdown,              SUPERMANAGER },
  {"rank",        "o",      RankUser,              USER }, 
  {"reopen",      "d",      ReopenTourney,         MANAGER },
  {"resetabuse",  "w",      ResetAbuse,            SUPERMANAGER },
  {"results",     "d",      ResultsFromTourney,    USER },
  {"rmmgr",       "w",      DeleteManagerFromList, SUPERMANAGER },
  {"set",         "wn",     SetVariable,           MANAGER },
  {"setlevel",    "wd",     SetManagerLevel,       SUPERMANAGER },
  {"setres",      "dwwd",   SetResult,             MANAGER },
  {"setscore",    "dwd",    SetScore,              MANAGER },
  {"standings",   "d",      ListStandingsTourney,  USER },
  {"start",       "d",      StartTourney,          MANAGER },
  {"subabuse",    "d",      SubtractAbuse,         SUPERMANAGER },
  {"tourneys",    "",       ListTourneys,          USER },
  {"unmanage",    "d",      UnmanageTourney,       MANAGER },
  {"uncensor",    "w",      UncensorUser,          SUPERMANAGER },
  {"version",     "",       ShowVersion,           USER },
  {"who",         "d",      ListPlayerInTourney,   USER },
  {"withdraw",    "d",      WithdrawFromTourney,   USER },
  {"worst",       "",       ListWorst,             USER },
  { NULL,         NULL }
};


/************************
 *  Command Aliases     *
 ************************/
t_cmds aliases[] = {
  {"abor",   do_abort },
  {"adda",   do_addabuse },
  {"ann",    do_announce },
  {"cl",     do_close },
  {"fi",     do_finger },
  {"for",    do_forfeit },
  {"ga",     do_games },
  {"gr",     do_grid },
  {"he",     do_help },
  {"inf",    do_set },
  {"mm",     do_messman },
  {"op",     do_open },
  {"pair",   do_games },
  {"pl",     do_players },
  {"po",     do_pose },
  {"qui",    do_quit },
  {"reopen", do_reopen },
  {"reseta", do_resetabuse },
  {"resu",   do_results },
  {"rm",     do_rmmgr },
  {"setl",   do_setlevel },
  {"setr",   do_setres },
  {"sets",   do_setscore },
  {"stand",  do_standings },
  {"suba",   do_subabuse },
  {"unm",    do_unmanage },
  {"unc",    do_uncensor },
  {"ver",    do_version },
  {"wo",     do_worst },
  {NULL,       NULL }
};


#endif
