//--------------------------------------------------------------------------
// TourneyParameters.hh - Class header for the TourneyParameters class
//
// Matthew E. Moses & Michael A. Long
//
// $Revision: 1.3 $
// $Date: 1998/09/10 19:58:41 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: TourneyParameters.hh,v $
// Revision 1.3  1998/09/10 19:58:41  mlong
// lots of little bug fixes and a few new features.
//
// Revision 1.2  1998/02/12 18:44:25  mlong
// *** empty log message ***
//
// Revision 1.2  1997/10/23 19:56:12  chess
// *** empty log message ***
//
// Revision 1.1  1997/05/02 23:52:06  chess
// Initial revision
//
//
//--------------------------------------------------------------------------

#ifndef _TOURNEYPARAMETERS_
#define _TOURNEYPARAMETERS_

class TourneyParameters {
 public:
    TourneyParameters();
    ~TourneyParameters();
    
 private:

 public:

  int time;
  int inc;
  char variant;
  int wild;
  char mode;
  char style;
  int ratingHigh;
  int ratingLow;
  int rounds;
  int currentRound;
  int maxPlayers;

 private:

};

#endif

