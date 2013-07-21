//--------------------------------------------------------------------------
// Player.hh - Class header for the Player class
//
// Matthew E. Moses & Michael A. Long
//
// $Revision: 1.3 $
// $Date: 1998/02/12 18:44:25 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: Player.hh,v $
// Revision 1.3  1998/02/12 18:44:25  mlong
// *** empty log message ***
//
// Revision 1.2  1997/11/11 16:48:06  chess
// *** empty log message ***
//
// Revision 1.2  1997/10/23 19:56:12  chess
// *** empty log message ***
//
// Revision 1.1  1997/05/16 03:22:36  chess
// Initial revision
//
//
//
//--------------------------------------------------------------------------

#ifndef _Player_
#define _Player_

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

class Player : public Link {
public:

  Player(char *, int);
  Player(char *, float);
  Player(char *, double);
  Player(char *, float, int);
  Player(char *, float, int, int);
  ~Player();
  
int IsPlayer(char *);

private:

public:

  char name[NAMELEN];
  int  value;
  double floatValue;
  int rating;

private:

};


#endif






