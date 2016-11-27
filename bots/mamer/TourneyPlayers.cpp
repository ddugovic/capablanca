//--------------------------------------------------------------------------
// TourneyPlayers.cpp - Source file for the TourneyPlayers class
//
// Matthew E. Moses && Michael A. Long
//
// $Revision: 1.7 $
// $Date: 1998/09/10 19:57:17 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: TourneyPlayers.cpp,v $
// Revision 1.7  1998/09/10 19:57:17  mlong
// lots of little bug fixes and a few new features
//
// Revision 1.6  1998/04/29 15:23:19  mlong
// prepairing for the move to daimi
// new sorting routine.
//
//
// Revision 1.2  1997/10/08 21:03:35  chess
// preparing for move to oracle machine at eworks.
//
// Revision 1.1  1997/05/14 16:59:13  chess
// Initial revision
//
//
//--------------------------------------------------------------------------

// static char RCSid[] = "$Id: TourneyPlayers.cpp,v 1.7 1998/09/10 19:57:17 mlong Exp $";

#include "TourneyPlayers.hh"

//- Constructor -----------------------------------------------------------
TourneyPlayers::TourneyPlayers(const char *n, int r, float s) {
  perform=0;
  upset=0;

  totalWhites = 0;
  totalBlacks = 0;
  consecutiveBlacks = 0;
  consecutiveWhites = 0;

  activeFlag = 1;
  sortValue = r/10000.0;

  dueColor = 0;
  paired = 0;

  memset(oppName, '\0', NAMELEN);

  memset(name, '\0', NAMELEN);
  strcpy(name, n);

  rating = r;
  score = s;

  oppChoice=0;
  location=ONLINE;
}

//- Constructor -----------------------------------------------------------
TourneyPlayers::TourneyPlayers() {
  perform=0;
  upset=0;

  totalWhites = 0;
  totalBlacks = 0;
  consecutiveBlacks = 0;
  consecutiveWhites = 0;

  activeFlag = 1;
  sortValue = 0.0;

  dueColor = 0;
  paired = 0;

  memset(oppName, '\0', NAMELEN);

  memset(name, '\0', NAMELEN);
  rating = 0;
  score = 0;

  oppChoice=0;
  location=ONLINE;
}

//- DeConstructor ---------------------------------------------------------
TourneyPlayers::~TourneyPlayers() {
}

//- AlreadyPlayed --------------------------------------------------------
int TourneyPlayers::AlreadyPlayed(const char *oppName) {
  Player *temp;

  LinkListIter<Player> opponentIter(opponentList);

  while((temp = opponentIter.Next())) {
    if(!strcmp(oppName, temp->name)) {
      return 1;
    }
  }

  return 0;
}

//- Start of RemovePotentialOppList
int TourneyPlayers::RemovePotentialOppList() {
  Player *temp;

  LinkListIter<Player> opponentIter(potentialOpponentList);
  
  while((temp = opponentIter.Next())) {
    potentialOpponentList.Delete(temp);
  }
  return 1;
}//- end of RemovePotentialOppList

//- Start of RemoveFromOppList -------------------------------------------------
void TourneyPlayers::RemoveFromOppList(const char *name) {
  Player *p=NULL;

  LinkListIter<Player> opponentIter(opponentList);

  while((p = opponentIter.Next())) {
    if(strcasecmp(p->name, name) == 0) {
      opponentList.Delete(p);
      break;
    }
  }
}//- end RemoveFromOppList -----------------------------------------------------

//- Start of RemoveLastOpponent -------------------------------------------------
void TourneyPlayers::RemoveLastOpponent() {
  Player *p=NULL, *last=NULL;
  LinkListIter<Player> opponentIter(opponentList);

  opponentIter.Reset();
  while((p = opponentIter.Next()))
    last = p;

  cout << name << " " << last->name << endl;  
  opponentList.Delete(last);
}//- end RemoveLastOpponent -----------------------------------------------------    
       
//- Start GetOpponentPlayer --------------------------------------------------------
Player *TourneyPlayers::GetOpponentPlayer(const char *oppName) {
  Player *temp=NULL;
  LinkListIter<Player> opponentIter(opponentList);

  while((temp = opponentIter.Next())) {
    if(!strcmp(oppName, temp->name)) {
      return temp;
    }
  }

  return NULL;
}//- end of GetOpponentPlayer

//- IsPaired ---------------------------------
int TourneyPlayers::IsPaired(void) {
  return paired;
}//- end IsPaired

//- ColorDue ---------------------------------
int TourneyPlayers::ColorDue(void) {
  return dueColor;
}//- end colorDue

//- NowPaired ---------------------------------
void TourneyPlayers::NowPaired(int value) {
  paired = value;
}//- end NowPaired

//-ChangeColorDue ---------------------------------
void TourneyPlayers::ChangeColorDue(int value) {
  dueColor = value;
}//- end ToggleColorDue

//- AddWhite ---------------------------------
void TourneyPlayers::AddWhite(void) {
  consecutiveWhites++;
  totalWhites++; // [HGM] this varable did not seem to get updater anywhere
  ChangeColorDue(0);
  ClearBlacks();
}//- end AddWhite  

//- AddBlack ---------------------------------
void TourneyPlayers::AddBlack(void) {
  consecutiveBlacks++;
  totalBlacks++; // [HGM] this varable did not seem to get updater anywhere
  ChangeColorDue(1);
  ClearWhites();
}//- end AddBlack

//- ClearWhites ---------------------------------
void TourneyPlayers::ClearWhites(void) {
  consecutiveWhites = 0;
}//- end ClearWhites

//- ClearBlacks ---------------------------------
void TourneyPlayers::ClearBlacks(void) {
  consecutiveBlacks = 0;
}//- end ClearBlacks

//- ClearTotalWhites ---------------------------------
void TourneyPlayers::ClearTotalWhites(void) {
  totalWhites = 0;
}//- end ClearTotalWhites

//- ClearTotalBlacks ---------------------------------
void TourneyPlayers::ClearTotalBlacks(void) {
  totalBlacks = 0;
}//- end ClearTotalBlacks

//- GetTotalWhites ---------------------------------
int TourneyPlayers::GetTotalWhites(void) {
  return totalWhites;
}//- end GetTotalWhites

//- GetTotalBlacks ---------------------------------
int TourneyPlayers::GetTotalBlacks(void) {
  return totalBlacks;
}//- end GetTotalBlacks

//- GetConsecutiveWhites ---------------------------------
int TourneyPlayers::GetConsecutiveWhites(void) {
  return consecutiveWhites;
}//- end GetConsecutiveWhites

//- GetConsecutiveBlacks ---------------------------------
int TourneyPlayers::GetConsecutiveBlacks(void) {
  return consecutiveBlacks;
}//- end GetConsecutiveBlacks

//- CalculateScore --------------------------------------
void TourneyPlayers::CalculateScore(void) {
  score = 0.0;
  Player *opp;
  LinkListIter<Player> opponentIter(opponentList);

  while((opp = opponentIter.Next())) {
    if(opp->floatValue > 0.0) {
      score += opp->floatValue;
    }
  }
  CalculatePerform();
}//- End of CalculateScore -------------------

//- CalculatePerform --------------------------------------
void TourneyPlayers::CalculatePerform(void) {
  int total=0, counter=0, upsetpoints=0, rtng=0;
  Player *opp;
  LinkListIter<Player> opponentIter(opponentList);

  while((opp = opponentIter.Next())) {
    if(!strcmp(opp->name, "_BYE_")) continue;
    rtng = opp->rating;
    if(opp->rating <= 0) rtng = 1675;
    if(opp->floatValue == 0.5) {
      total += rtng;
      if(rtng > rating)
	upsetpoints += (rtng - rating)/2;
    } else if(opp->floatValue == 1.0) {
      total += (rtng + 400);
      if(rtng > rating)
	upsetpoints += (rtng - rating);
    } else if(opp->floatValue == 0.0) {
      total += (rtng - 400);
    }
    counter++;
  }

  if(counter > 0)
    perform = total/counter;
  else
    perform = 0;
  upset = upsetpoints;
}//- End of CalculatePerform -------------------



