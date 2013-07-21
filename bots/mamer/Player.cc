//--------------------------------------------------------------------------
// Player.cc - Source file for the Player class
//
// Matthew E. Moses && Michael A. Long
//
// $Revision: 1.4 $
// $Date: 1998/09/10 19:57:17 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: Player.cc,v $
// Revision 1.4  1998/09/10 19:57:17  mlong
// lots of little bug fixes and a few new features
//
// Revision 1.3  1998/02/12 18:44:04  mlong
// ..
//
// Revision 1.2  1997/10/08 21:03:35  chess
// preparing for move to oracle machine at eworks.
//
// Revision 1.1  1997/05/16 03:22:36  chess
// Initial revision
//
// Revision 1.1  1997/05/14 17:00:30  chess
// Initial revision
//
//
//--------------------------------------------------------------------------

// static char RCSid[] = "$Id: Player.cc,v 1.4 1998/09/10 19:57:17 mlong Exp $";

#include "Player.hh"

//- Constructor -----------------------------------------------------------
Player::Player(char *n, int wt) {
  strcpy(name, n);
  value = wt;
}

//- Constructor -----------------------------------------------------------
Player::Player(char *n, float wt) {
  strcpy(name, n);
  floatValue = wt;
}

//- Constructor -----------------------------------------------------------
Player::Player(char *n, double wt) {
  strcpy(name, n);
  floatValue = wt;
}

//- Constructor -----------------------------------------------------------
Player::Player(char *n, float f, int i) {
  strcpy(name, n);
  floatValue = f;
  value = i;
}

//- Constructor -----------------------------------------------------------
Player::Player(char *n, float f, int i, int r) {
  strcpy(name, n);
  floatValue = f;
  value = i;
  rating = r;
}

//- DeConstructor ---------------------------------------------------------
Player::~Player() {
}

//- IsPlayer -----------------------------------------------------------------
int Player::IsPlayer(char *user) {
    if(0 == strcasecmp(user, name))
        return(1);
    else
        return(0);
} //- End of IsPlayer
