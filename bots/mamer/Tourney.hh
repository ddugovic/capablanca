//--------------------------------------------------------------------------
// Tourney.hh - Class header for the Tourney class
//
// Matthew E. Moses & Michael A. Long
//
// $Revision: 1.11 $
// $Date: 1998/09/10 19:58:41 $
//
// $Author: mlong $
// $Locker:  $
//
//--------------------------------------------------------------------------

#ifndef _TOURNEY_
#define _TOURNEY_

#define log22(x) (log(x)/log(2))

#include <fstream>
#include <iostream>

extern "C" {
#include <sys/param.h>
#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include <time.h>
#include <unistd.h>
#include <math.h>
}

#include "config.h"
#include "link.hh"
#include "linklist.hh"
#include "User.hh"
#include "TourneyParameters.hh"
#include "TourneyPlayers.hh"
#include "Game.hh"
#include "Storage.hh"

class Tourney : public Link {
 public:
  Tourney();
  Tourney(int, User *, TourneyParameters *);
  Tourney(int, User *, TourneyParameters *, int, int, char, char, int);
  Tourney(int, User *, TourneyParameters *, int, int, char, char, int, char);
  ~Tourney();

  void InitTourney(int, User *, int, int, char, char, int, char, int, int);

  int intcmp(int, int);

  void TellThemWhoTheyPlay(void);

  int IsTourney(int);
  short IsNotNew(void);
  short IsNotClosed(void);
  int IsPaused(void);
  int Open(void);
  void CloseAndStart(void);
  int GetStatus(void);
  void EndTourney();
  void SetPause(int);

  void Announce(void);

  int AddPlayer(const char *, int, float);
  int RemovePlayer(const char *);
  void SetVariable(int, int);
  void SetVariable(int, const char *);
  const char *GetWild(int);
  void CalculateAverage();

  void SortPlayers();
  int GetSortValueCount(double);
  Player *GetSortPlayer(const char *);
  Player *GetSortPlayer(int);

  TourneyPlayers *GetPlayer(const char *);
  int GetPlayerCount();
  int GetVariant();
  int GetRound();
  int GetRoundsRemaining();
  float GetAverageRating();

  int MakeAssignments();
  void SetOffsets();
  void SetPairingScores(TourneyPlayers *);
  TourneyPlayers *FindBestOpponent(TourneyPlayers *);
  int PairPlayers(TourneyPlayers *, TourneyPlayers *);
  void UnPairPlayer(TourneyPlayers *);
  void AssignColors(TourneyPlayers *, TourneyPlayers *);
  int SetGameResult(const char *, const char *, int);

  long GetStartDate();
  long GetEndDate();
  void SetEndDate();
  void SetPersist(int);
  int GetPersist();
  
  int PopLastPairedPlayer();
  void ClearPairedPlayers();

  void PrintPotentialLists();

 public:
  int number;
  long lastCshouted;

  LinkList<TourneyPlayers> playerList;
  LinkList<Game> gameList;
  LinkList<Player> sortList;
  LinkList<Storage> pairedPlayers;

  TourneyParameters params;

  char manager[NAMELEN];
  int  managerLevel;

 private:
  int persist;
  time_t startDate;
  time_t endDate;

  int status;
  int paused;

  float averageRating;

  int currentRound;
};

#endif

//---------------------------------------------------------
// $Log: Tourney.hh,v $
// Revision 1.11  1998/09/10 19:58:41  mlong
// lots of little bug fixes and a few new features.
//
// Revision 1.10  1998/04/29 15:24:07  mlong
// prepairing for the move to daimi
// new sorting routine.
//
// Revision 1.9  1998/04/18 18:46:31  mlong
// fixed delete bug and
// added delete tourney function
//
// Revision 1.8  1998/02/12 18:44:25  mlong
// *** empty log message ***
//
// Revision 1.7  1997/10/28 21:03:48  mlong
// *** empty log message ***
//
// Revision 1.6  1997/10/23 19:56:44  mlong
// *** empty log message ***
//
// Revision 1.5  1997/10/23 19:37:22  chess
// lots of new stuff
//
// Revision 1.4  1997/05/15 18:29:12  chess
//  added pending and TourneyPlayers support
// added HandleGetPlayerInfo & HandleGetGameInfo
//
// Revision 1.3  1997/05/02 23:55:18  chess
// added TourneyParameters class include
//
// Revision 1.2  1996/10/01 20:14:43  moses
// added methods
//
// Revision 1.1  1996/09/30  20:52:48  moses
// Initial revision
//
//--------------------------------------------------------------------------
