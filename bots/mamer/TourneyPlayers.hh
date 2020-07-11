//--------------------------------------------------------------------------
// TourneyPlayers.hh - Class header for the TourneyPlayers class
//
// Matthew E. Moses & Michael A. Long
//
// $Revision: 1.6 $
// $Date: 1998/09/10 19:58:41 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: TourneyPlayers.hh,v $
// Revision 1.6  1998/09/10 19:58:41  mlong
// lots of little bug fixes and a few new features.
//
// Revision 1.5  1998/04/29 15:24:07  mlong
// prepairing for the move to daimi
// new sorting routine.
//
// Revision 1.4  1998/02/12 18:44:25  mlong
// *** empty log message ***
//
// Revision 1.3  1997/10/28 21:03:48  mlong
// *** empty log message ***
//
// Revision 1.2  1997/10/23 19:56:12  chess
// *** empty log message ***
//
// Revision 1.1  1997/05/14 16:59:13  chess
// Initial revision
//
//
//--------------------------------------------------------------------------

#ifndef _TOURNEYPLAYERS_
#define _TOURNEYPLAYERS_

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
}

#include "config.h"
#include "User.hh"
#include "link.hh"
#include "linklist.hh"
#include "Player.hh"
#include "types.h"

class TourneyPlayers : public Link {
public:
  TourneyPlayers(const char *, int, float);
  TourneyPlayers();
  ~TourneyPlayers();
  
  void AddWhite();
  void AddBlack();
  void ClearWhites();
  void ClearBlacks();
  void ClearTotalWhites();
  void ClearTotalBlacks();

  int GetConsecutiveWhites();
  int GetConsecutiveBlacks();
  int GetTotalWhites();
  int GetTotalBlacks();
  int ColorDue();

  int IsPaired();
  void NowPaired(int);

  int AlreadyPlayed(const char *);
  void CalculateScore();

  int RemovePotentialOppList();
  Player *GetOpponentPlayer(const char *);
  void RemoveFromOppList(const char *);
  void RemoveLastOpponent();

private:
  void ChangeColorDue(int);

  void CalculatePerform();

public:

  char name[NAMELEN];
  int rating;
  float score;
  float extra; // [HGM] McMahon points
  int perform;
  int upset;
  int activeFlag;
  double sortValue;
  int oppChoice;
  int location;

  char oppName[NAMELEN];

  LinkList<Player> opponentList;
  LinkList<Player> potentialOpponentList;

private:

  int totalWhites;
  int totalBlacks;
  int consecutiveWhites;
  int consecutiveBlacks;
  int dueColor;
  int paired;

};


#endif
