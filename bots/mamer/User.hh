//--------------------------------------------------------------------------
// User.hh - Class header for the User class
//
// Matthew E. Moses
//
// $Revision: 1.8 $
// $Date: 1998/09/10 19:58:41 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: User.hh,v $
// Revision 1.8  1998/09/10 19:58:41  mlong
// lots of little bug fixes and a few new features.
//
// Revision 1.7  1998/02/12 18:44:25  mlong
// *** empty log message ***
//
// Revision 1.6  1997/04/13 03:21:32  chess
// AddAbuse and SetStatistic added
//
// Revision 1.5  1997/04/07 22:22:12  chess
// *** empty log message ***
//
// Revision 1.4  1997/03/27 13:45:28  chess
// added functions to return user's statistics
//
// Revision 1.3  1997/03/21 15:29:53  moses
// *** empty log message ***
//
// Revision 1.2  1996/10/01 20:14:43  moses
// added methods
//
// Revision 1.1  1996/09/30  20:52:48  moses
// Initial revision
//
//
//--------------------------------------------------------------------------

#ifndef _USER_
#define _USER_

#include <fstream>
#include <iostream>

using namespace std;

extern "C" {
#include <sys/param.h>
#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include <time.h>
#include <unistd.h>
}

#include "config.h"
#include "link.hh"

#ifndef NAMELEN
#define NAMELEN 19
#endif

class User : public Link {
public:
  
  User();
  User(const char *, const char *);
  ~User();
  
  int IsUser(const char *);
  
  void LoadPlayer(const char *);
  short LoadPlayer(const char *, const char *);
  
  void SavePlayer(const char *);
  
  float GetRating(void);
  int GetManagerLevel(void);
  
  int GetAbuse(void);
  long GetWins(void);
  long GetLosses(void);
  long GetDraws(void);
  long GetFirsts(void);
  long GetSeconds(void);
  long GetThirds(void);
  float GetPlacePoints(void);
  long GetPlayedTourneys(void);
  long GetManagedTourneys(void);
  long GetLast(void);
  void SetLast(long);
  
  void AddAbuse(int);
  void ResetAbuse(void);
  
  void AddManagedTourney();
  void ChangeManagerLevel(int);

  void AddPlayedTourney();

  int SetStatistic(int, int);
  
  void AddStat(double);
  void AddStat(int);
  void CalculateRating(float, float);

 private:
  void CalculateRating(void);

  void AddWin(void);
  void AddLoss(void);
  void AddDraw(void);
  
  void AddFirst(void);
  void AddSecond(void);
  void AddThird(void);
  
  void CreateDirectory(const char *, const char *);

 public:

    int inTourney;
    char name[NAMELEN];

 private:

    int  abuse;

    long playedTourneys;

    long wins;
    long losses;
    long draws;

    long firsts;
    long seconds;
    long thirds;

    float rating;
    float placePoints;

    int managerLevel;

    long managedTourneys;

    long last;
    int  tourneyLocation;    
};

#endif





