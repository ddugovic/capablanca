//--------------------------------------------------------------------------
// Game.hh - Class header for the Game class
//
// Matthew E. Moses & Michael A. Long
//
// $Revision: 1.3 $
// $Date: 1998/09/10 19:58:41 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: Game.hh,v $
// Revision 1.3  1998/09/10 19:58:41  mlong
// lots of little bug fixes and a few new features.
//
// Revision 1.2  1998/04/29 15:24:07  mlong
// prepairing for the move to daimi
// new sorting routine.
//
// Revision 1.1  1997/07/18 15:42:58  chess
// Initial revision
//
//
//--------------------------------------------------------------------------

#ifndef _Game_
#define _Game_

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

class Game : public Link {
public:

  Game(const char *, const char *, int, int, int, char);
  ~Game();
  
int IsGame(const char *, const char *, int, int, int, char);

private:

public:

  char whiteName[NAMELEN];
  char blackName[NAMELEN];
  int rated;
  int time;
  int inc;
  char variation;
  int gameNumber;

private:

};


#endif






