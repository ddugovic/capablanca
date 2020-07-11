//--------------------------------------------------------------------------
// Tourney.cpp - Source file for Tourney class
//
// Matthew E. Moses
//
// $Revision: 1.11 $
// $Date: 1998/09/10 19:57:17 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: Tourney.cpp,v $
// Revision 1.11  1998/09/10 19:57:17  mlong
// lots of little bug fixes and a few new features
//
// Revision 1.10  1998/04/29 15:23:19  mlong
// prepairing for the move to daimi
// new sorting routine.
//
// Revision 1.9  1998/04/18 20:05:14  mlong
// fixed BYE bug
//
// Revision 1.8  1998/04/18 18:46:04  mlong
// fixed delete bug &
// added delete tourney function
//
// Revision 1.4  1997/10/08 21:03:35  chess
// preparing for move to oracle machine at eworks.
//
// Revision 1.3  1997/05/15 18:27:53  chess
// added pending and TourneyPlayers support
// added HandleGetPlayerInfo & HandleGetGameInfo
//
// Revision 1.2  1996/10/01 20:14:43  moses
// Added a new method IsTourney
//
// Revision 1.1  1996/09/30  20:52:48  moses
// Initial revision
//
//--------------------------------------------------------------------------

//static char RCSid[] = "$Id: Tourney.cpp,v 1.11 1998/09/10 19:57:17 mlong Exp $";

#include "Tourney.hh"
#include "Mamer.hh"

extern Mamer gMamer;

//- Constructor ------------------------------------------------------------
Tourney::Tourney() {
} //- End of Tourney

Tourney::Tourney(int n,User *u, TourneyParameters *tp) {
  InitTourney(n, u, tp->time, tp->inc, tp->mode, tp->style, tp->rounds, tp->variant, tp->ratingLow, tp->ratingHigh);
}

Tourney::Tourney(int n, User *u, TourneyParameters *tp, int t, int i, char m, char s, int r) {
  InitTourney(n, u, t, i, m, s, r, tp->variant, tp->ratingLow, tp->ratingHigh);
}

Tourney::Tourney(int n, User *u, TourneyParameters *tp, int t, int i, char m, char s, int r, char v) {
  InitTourney(n, u, t, i, m, s, r, v, tp->ratingLow, tp->ratingHigh);
}

void Tourney::InitTourney(int n, User *u, int t, int i, char m, char s, int r, char v, int rl, int rh) {
  number = n;
  strncpy(manager, u->name, NAMELEN - 1);
  managerLevel = u->GetManagerLevel();
  averageRating = 0;
  params.time = t;
  params.inc = i;
  params.mode = m;
  params.style = s;
  params.rounds = r;
  params.variant = v;
  params.wild = 10;
  params.ratingLow = rl;
  params.ratingHigh = rh;
  params.currentRound = 0;
  params.maxPlayers = DEFAULT_MAX_PLAYERS;

  startDate = 0;
  endDate = 0;
  persist = 0;
  lastCshouted = 0;

  status = NEW;
  paused = FALSE;
}

//- Deconstructor ---------------------------------------------------------
Tourney::~Tourney() {

} //- End of ~Tourney

//- IsTourney -------------------------------------------------------------
int Tourney::IsTourney(int tourn) {
    if(tourn == number)
	return(1);
    else
	return(0);
} //- End of IsTourney

//- IsNotNew-------------------------------------------------------------
short Tourney::IsNotNew(void) {
  if(NEW != status)
    return(TRUE);
  return(FALSE);
}

//- IsNotClosed-------------------------------------------------------------
short Tourney::IsNotClosed(void) {
  if(CLOSED != status)
    return(TRUE);
  return(FALSE);
}

//- AddPlayer ----------------------------------------------------------
int Tourney::AddPlayer(const char *name, int rating, float score) {
  TourneyPlayers *newPlayer = NULL, *tp = NULL;
  Player *newSortPlayer = NULL;

  if (status != OPEN   // If we are not open then we can't enter the tourney
      && !(score != 0. && status == CLOSED)) return 3; // [HGM] unless the manager adds us as late join!
  
  tp = GetPlayer(name);

  if(tp != NULL) return(2);   // if we are already in the tourney we can't enter it again/

  if(rating >= params.ratingLow && rating <= params.ratingHigh && status == OPEN) {

    if(GetPlayerCount() >= params.maxPlayers) return(0); // [HGM] never exceed max players (or would it be safe to do so?)

    newPlayer = new TourneyPlayers(name, rating, 0.); // [HGM] always set start score = 0.
    newSortPlayer = new Player(name, 0);

    playerList.Append(newPlayer);
    SortPlayers();
    gMamer.XServerCom("%s %i %s %s%i%s %s%i %i %s%s", "tell", gMamer.channelNumber, name, "(", rating, ")", 
    		      "has joined tourney #", number, GetPlayerCount(), "players now.", "\n");
    CalculateAverage();

    if(status == CLOSED) { // [HGM] late join; do some stuff already done in CloseAndStart for the others
	newPlayer->ClearWhites();
	newPlayer->ClearBlacks();
	newPlayer->ClearTotalWhites();
	newPlayer->ClearTotalBlacks();

	// give the player a BYE (which we might have to add)
	LinkListIter<TourneyPlayers> playerIter(playerList);
	playerIter.Reset();
	while((tp = playerIter.Next())) {
	    if(strcmp(tp->name, "_BYE_") == 0)  break;
	}
	if(!tp) {
	    tp = new TourneyPlayers("_BYE_", 0, 0);  
	    playerList.Append(tp);                  // add the bye to the tourney players list
	    SortPlayers();
	}
	newPlayer->opponentList.Append(new Player("_BYE_", 0., 0, 0)); // add a BYE for missed round, to prevent it can get a second
	
	return 1; // in any case never start automatically
    }

    if(GetPlayerCount() >= params.maxPlayers)
      CloseAndStart();
    return(1);   // we entered the tourney
  } else 
    return(0); // sucks to be us cause our rating doesn't fit the params
}

//- RemovePlayer ----------------------------------------------------------
int Tourney::RemovePlayer(const char *name) {
  TourneyPlayers *tp = NULL, *p=NULL;
  Player *opp=NULL;
  int roundsRemaining=0;

  tp = GetPlayer(name);
  printf("forfeiting %s\n", tp->name);
  if(tp == NULL) return -1; // Player not in THIS tourney

  tp->activeFlag = 0;

  roundsRemaining = GetRoundsRemaining();

  // This code will go through a forfeited players list and give him a loss for current opponent
  // with a quick modification it will give losses for all games played as well...
  LinkListIter<Player> opponentIter(tp->opponentList);  // List of opponents this player has had
  opponentIter.Reset();
  while((opp = opponentIter.Next())) {
    if(strcmp("_BYE_", opp->name) != 0) {  // If I am not _BYE_
      p = GetPlayer(opp->name);
      if(opp->floatValue == -1) {  // floatValue stores the game result for this player
	if(opp->value) {  // if player leaving was white
	  SetGameResult(tp->name, opp->name, 0);
	} else {
	  SetGameResult(opp->name, tp->name, 1);
	}
	roundsRemaining++;
      }
    }
  }
  
  LinkListIter<TourneyPlayers> playerIter(playerList);
  playerIter.Reset();
  while((p = playerIter.Next())) {
    if(strcmp(p->name, "_BYE_") == 0)  { 
      if(p->activeFlag != 0)
	p->activeFlag = 0;
      else
	p->activeFlag = 1;
      break;
    }
  }

  SortPlayers();

  return roundsRemaining;
}

//- GetPlayer ----------------------------------------------------------
TourneyPlayers *Tourney::GetPlayer(const char *name) {
  LinkListIter<TourneyPlayers> playerIter(playerList);
  TourneyPlayers *tp = NULL;
  
  playerIter.Reset();
  while((tp = playerIter.Next()))
    if(!strcasecmp(tp->name, name))  
      return tp;
  
  return NULL;
}

//- GetRound ----------------------------------------------------------
int Tourney::GetRound() {
  return params.currentRound;
}//- end of GetRound --------------------------------------------------

//- GetRoundsRemaining ------------------------------------------------
int Tourney::GetRoundsRemaining() {
  return (params.rounds - params.currentRound);
}//- end of GetRoundsRemaining -----------------------------------------

//- SortPlayers ----------------------------------
void Tourney::SortPlayers() {
  Player          *temp=NULL, *s=NULL;
  TourneyPlayers  *tp = NULL;
  int             i=0, added=0;

  LinkListIter<TourneyPlayers> playerIter(playerList);
  LinkListIter<Player> sortIter(sortList);

  sortIter.Reset();
  while((s = sortIter.Next())) sortList.Delete(s);

  i=0;
  playerIter.Reset(); // [HGM]
  while((tp = playerIter.Next())) {
    (tp->activeFlag) ? tp->sortValue = (tp->score + tp->rating/10000.0) : tp->sortValue = -1.0;
  //  tp->ClearWhites();
  //  tp->ClearBlacks();
    if((status == OPEN) && (i < (GetPlayerCount()/2)))
      (i % 2) ? tp->AddWhite() : tp->AddBlack();
    i++;
  }

  playerIter.Reset();
  while((tp = playerIter.Next())) {
    added=0;
    sortIter.Reset();
    temp = new Player(tp->name, tp->sortValue);
    while((s = sortIter.Next())) {
      if(tp->sortValue > s->floatValue) {
	sortList.Insert(s, temp);
	added = 1;
	break;
      }
    }
    if(!added)
      sortList.Append(temp);
  }

  i = 1;
  sortIter.Reset();
  while((s = sortIter.Next())) { 
    s->value = i;
    if(gMamer.debugLevel >= 10) printf("%4d %-18s\n", s->value, s->name); 
    i++;
  }
}//- end of Sort Players ----------------------

//- GetSortValueCount -------------------------------------------------------
int Tourney::GetSortValueCount(double value) {
  LinkListIter<TourneyPlayers> playerIter(playerList);
  int count=0;
  TourneyPlayers *tp=NULL;
  Player *s=NULL;

  while((tp = playerIter.Next())) {
    if(tp->sortValue == value) {
      s = GetSortPlayer(tp->name);
      if(s->value != 0)
	count++;
    }
  }
  
  return count;
}

//- GetSortPlayer ----------
Player *Tourney::GetSortPlayer(const char *name) {
  Player *p = NULL;
  LinkListIter<Player> sortIter(sortList);

  while((p = sortIter.Next())) {
    if(strcasecmp(p->name, name) == 0) {
      return p;
    }
  }

  return p;
}//- end of GetSortPlayer -----

//- GetSortPlayer ------------------------------
Player *Tourney::GetSortPlayer(int place) {
  Player *p = NULL;
  LinkListIter<Player> sortIter(sortList);

  while((p = sortIter.Next())) {
    if(p->value == place) {
      return p;
    }
  }
  
  return p;
}//- end of GetSortPlayer -----

//- CalculateAverage --------------------------------------------------
void Tourney::CalculateAverage(void) {
  int total=0, count=0;
  TourneyPlayers *p;
  LinkListIter<TourneyPlayers> playerIter(playerList);
  
  while((p = playerIter.Next())) {
    if(p->rating > 0) {
      total += p->rating;
      count++;
    }
  }
  if(count)
    averageRating = ((float)total/(float)count);
  else
    averageRating = 0;
}//- end CalculateAverage

//- GetAverageRating ----------------------------------------------------------
float Tourney::GetAverageRating(void) {
  return averageRating;
}//end GetAverageRating

//- GetVariant ----------------------------------------------------------
int Tourney::GetVariant(void) {
  float eTime;

  switch(params.variant) {
  case 'w':
    return(0);
  case 'r':
    eTime = (float)params.time + (0.6666667 * (float)params.inc);
    if(eTime < 3)
      return(1);
    else if(eTime < 15)
      return(2);
    else 
      return(3);
  case 'b':
    return(4);
  case 's':
    return(5);
  default:
    return(2);
  }
}

//- Open ----------------------------------------------------------
int Tourney::Open(void) {
  if(status == NEW) {
    status = OPEN;
    return 1;
  } else {
    return 0;
  }
}

//- GetPlayerCount ----------------------------------------------------------
int Tourney::GetPlayerCount() {
  int count=0;
  TourneyPlayers *p;
  LinkListIter<TourneyPlayers> playerIter(playerList);
  
  while((p = playerIter.Next())) {
    if(p->activeFlag != 0)
      count++;
  }

  return count;
}//- end GetPlayerCount ----------------------------------

//- SetPersist -------------------------------------------------------------
void Tourney::SetPersist(int i) {
  persist = i;
} //- end SetPersist

//- GetPersist -------------------------------------------------------------
int Tourney::GetPersist() {
  return persist;
} //- end GetPersist

//- SetEndDate -------------------------------------------------------------
void Tourney::SetEndDate() {
  endDate = time(0);
} //- end of SetEndDate ----------------------------------------------------

//- CloseAndStart ----------------------------------------------------------
void Tourney::CloseAndStart(void) {
  TourneyPlayers *tp = NULL;
  status = CLOSED;
  params.currentRound = 0;
  Player*s = NULL;

  LinkListIter<TourneyPlayers> playerIter(playerList);
  LinkListIter<Player> sortIter(sortList);

  startDate = time(0);

  cout << "tourney started at: " << ctime(&startDate) << endl;

  if(params.rounds == 0) { 
    switch(params.style) {
    case 'r':
      params.rounds = GetPlayerCount() - 1;
      break;
    case 's':
    case 'm': // [HGM] McMahon
      params.rounds = (int)ceil(log2(GetPlayerCount())); 
      break;
    default:
      params.rounds = DEFAULT_ROUNDS;
      break;
    }
  }

  // this is to stop a 4 player tourney from having 2 rounds
  params.rounds = (params.rounds < MINIMUM_ROUNDS) ? MINIMUM_ROUNDS : params.rounds;
  
  playerIter.Reset(); // [HGM] this code moved here from SortPlayers
  while((tp = playerIter.Next())) {
    tp->ClearWhites();
    tp->ClearBlacks();
    tp->ClearTotalWhites();
    tp->ClearTotalBlacks();
  }

  if(params.style == 'm') { // [HGM] McMahon
    int i = 1;
    sortIter.Reset();
    while((s = sortIter.Next())) { 
      TourneyPlayers *tp = GetPlayer(s->name);
      int nr = GetPlayerCount();
      int mx = params.rounds + 1;
      int p = (nr - i)*2*mx/nr;
      if(p > mx) p = mx;
      tp->extra = tp->score = p*0.5;
      i++;
    }
  }

  MakeAssignments();
  TellThemWhoTheyPlay();  // tell them who they play 
}

int Tourney::PopLastPairedPlayer() {
  Storage *p=NULL, *lastPlayer=NULL;
  LinkListIter<Storage> pairedIter(pairedPlayers);
  int last=0;

  while((p = pairedIter.Next())) {
    lastPlayer = p;
    last = p->value;
  }

  if(last) {
    cout << "Popping: " << lastPlayer->name << " from the paired list " << endl;
    pairedPlayers.Delete(lastPlayer);
  } else
    cout << "Popping: _NOBODY_" << " from the paired list " << endl;

  return last;
}

void Tourney::ClearPairedPlayers() {
  Storage *p=NULL;
  LinkListIter<Storage> pairedIter(pairedPlayers);

  while((p = pairedIter.Next())) pairedPlayers.Delete(p);
}

int Tourney::MakeAssignments(void) {
  TourneyPlayers *tp = NULL, *opponent = NULL, *bye = NULL;
  Storage *newPairedPlayer=NULL;
  Player *p=NULL, *opp=NULL;
  int everybodyPaired=0, playerCount=0, i=1;
  LinkListIter<TourneyPlayers> playerIter(playerList);
  
  params.currentRound++;
  if(params.currentRound > params.rounds) {
    cout << "Returning because current round is > rounds" << endl;
    cerr << "Returning because current round is > rounds" << endl;
    return 0;
  }  
  // Initialize a few things...make sure nobody is paired,
  playerIter.Reset();
  while((tp = playerIter.Next())) {
    UnPairPlayer(tp);
    if(strcmp(tp->name, "_BYE_") == 0)  { bye = tp; tp->activeFlag = 0; }  // unset the byeFlag [HGM] and remember bye and deactivate
  }
  playerCount = GetPlayerCount();
  if(playerCount % 2){   // we need to add a bye
   if(bye) bye->activeFlag = 1; else { // [HGM] if bye existed, re-activate it
    bye = new TourneyPlayers("_BYE_", 0, 0);  
    playerList.Append(bye);                  // add the bye to the tourney players list
    SortPlayers();
    playerCount++;
   }
  }
  
  // Set up the PairingScores
  playerIter.Reset();
  while((tp = playerIter.Next())) { if(!tp->IsPaired()) SetPairingScores(tp); }
  
  playerIter.Reset();
  while((tp = playerIter.Next())) { UnPairPlayer(tp); tp->oppChoice=0; }  // unpair all the players
  
  i = 1;
  ClearPairedPlayers();
  while(everybodyPaired == 0) {
    everybodyPaired = 0;
    p = GetSortPlayer(i);
    tp = GetPlayer(p->name);
    opponent = (TourneyPlayers *)NULL;
    // PrintPotentialLists();
    if((tp->IsPaired() == FALSE) && tp->activeFlag) { // If I am not paired and I am active pair me
      if((opponent = FindBestOpponent(tp))) {
	newPairedPlayer = new Storage(tp->name, i);
	pairedPlayers.Append(newPairedPlayer);
	cerr << "Adding: " << tp->name << " " << i << " " << "to the paired list " << opponent->name << endl;
	everybodyPaired = PairPlayers(tp, opponent);  // Actually Pair me
	i++;                                // go to the next player
      } else {                              // If there is no opponent for me go back and repair up the tree
	if(tp->oppChoice > playerCount) {  // If I have tried all my opponents
	  tp->oppChoice = 0;               // reset me so I can try again later
	  i = PopLastPairedPlayer();          // returns the last player paired & removes him from the paired list
	  cerr << "i=" << i << endl;
	  if(i <= 0)  {                         // this would be really bad means we can't even find 
	    cout << "Returning because we can't find pairings" << endl;
	    cerr << "Returning because we can't find pairings" << endl;
	    return 0;                       // an opponent for the first player.  Tourney has to be over now
	  }
	  p = GetSortPlayer(i);
	  tp = GetPlayer(p->name);
	  opponent = GetPlayer(tp->oppName);
	  cout << "UnPairing: " << tp->name << " " << opponent->name << " choice: " << tp->oppChoice << endl;
	  tp->RemoveLastOpponent();           // removes the person we were planning on playing
	  opponent->RemoveLastOpponent();
	  UnPairPlayer(tp);                   // unpair us so we can be re-paired
	  UnPairPlayer(opponent);
	  tp->oppChoice++;                           // try his next possible opponent
	} else {
	  tp->oppChoice++;   // Try my next opponent
	}
      }
    } else {  // if I am already paired go to the next player and pair him
      i++;
    }
  }

  if(everybodyPaired > 0) {
    playerIter.Reset();
    while((tp = playerIter.Next())) UnPairPlayer(tp);  // unpair all players so we can use that to tell
    playerIter.Reset();                                 // if they have been assiged a color
    while((tp = playerIter.Next())) {
      if((!tp->IsPaired()) && (tp->activeFlag != 0)) {
	opponent = GetPlayer(tp->oppName);
	AssignColors(tp, opponent);
	tp->NowPaired(TRUE);               // mark BOTH players as having a color
	opponent->NowPaired(TRUE);         // this is important for when we hit this player later in the Iter
      }
    }
  }

  playerIter.Reset();
  while((tp = playerIter.Next()))  {
    if(0 == strcmp(tp->name, "_BYE_")) {  // If I am the bye
      LinkListIter<Player> opponentIter(tp->opponentList);
      while((opp = opponentIter.Next())) { // Got through my opponents and find the one I am playing now
	if(0 == strcasecmp(opp->name, tp->oppName)) { // & Give him a win
	  if(opp->value)
	    SetGameResult(tp->name, tp->oppName, 0);
	  else
	    SetGameResult(tp->oppName, tp->name, 1);
	  gMamer.XServerCom("tell %s you get a BYE this round.%s", opp->name, "\n");
	}
      }
    }
  }
  return 1;
}

//- PrintPotentialLists
void Tourney::PrintPotentialLists() {
  TourneyPlayers *tp=NULL;
  Player *o=NULL;

  LinkListIter<TourneyPlayers> playerIter(playerList);

  while((tp = playerIter.Next())) {
    printf("%-10s %i\n", tp->name, tp->oppChoice);
    LinkListIter<Player> oppIter(tp->potentialOpponentList);    
    while((o = oppIter.Next())) {
      printf("%d %-10s ", o->value, o->name);
    }
    printf("\n\n");
  }
}

//- Start of FindBestOpponent
TourneyPlayers *Tourney::FindBestOpponent(TourneyPlayers *tp) {
  Player *tmp = NULL;
  
  LinkListIter<Player> opponentIter(tp->potentialOpponentList);
  while((tmp = opponentIter.Next())) {
    if((tmp->value == tp->oppChoice) && (0 == GetPlayer(tmp->name)->IsPaired())) {
      return GetPlayer(tmp->name);
    }
  }

  return NULL;
}

//- Start of SetPairingSores -------------------------------
void Tourney::SetPairingScores(TourneyPlayers *tp) {
  double score;
  TourneyPlayers *opponent = NULL;
  Player *temp=NULL, *newOpp=NULL, *t=NULL, *me=NULL;
  int offset=2, place=1, i=0, added=0;
  
  tp->RemovePotentialOppList();

  LinkListIter<TourneyPlayers> playerIter(playerList);
  
  SortPlayers();
  
  while((opponent = playerIter.Next())) {
    if((strcmp(tp->name, opponent->name) != 0) && 
       (tp->score == opponent->score)) {
      offset++;
      if(opponent->rating > tp->rating) {
	place++;
      }
    }
  }
  offset = offset / 2;
  if(place > offset) { offset *= -1; }

  me = GetSortPlayer(tp->name);
  playerIter.Reset();
  while((opponent = playerIter.Next())) {
    if(strcmp(tp->name, opponent->name) && (tp->activeFlag !=0)) { // If this isn't MY name & I am active
      if((!tp->AlreadyPlayed(opponent->name)) && (!opponent->IsPaired()) && (opponent->activeFlag != 0)
	  && (tp->ColorDue() != opponent->ColorDue() // they are due different color, never a problem
	     || tp->ColorDue() ? // both are due white. Check if one of them could accept black without breaking 'absolute' color rules
				 tp->GetConsecutiveBlacks() < 2 &&
				 tp->GetTotalBlacks() - tp->GetTotalWhites() < 2 ||
				 opponent->GetConsecutiveBlacks() < 2 &&
				 opponent->GetTotalBlacks() - opponent->GetTotalWhites() < 2
			       : // both are due black. Check if any of them can accept white
				 tp->GetConsecutiveWhites() < 2 &&
				 tp->GetTotalWhites() - tp->GetTotalBlacks() < 2 ||
				 opponent->GetConsecutiveWhites() < 2 &&
				 opponent->GetTotalWhites() - opponent->GetTotalBlacks() < 2
	     )
	) { 
	// and I haven't played this person and this person is active. (not forfeited)
	t = GetSortPlayer(opponent->name);
	score = ((abs(t->value - (me->value + offset))) * 1000);
	if(opponent->score >= tp->score) {
	  score = score + ((opponent->score - tp->score) * 10.0);
	} else {
	  score = score + ((tp->score - opponent->score) * 10.0);
	}
	score += abs(opponent->ColorDue() - tp->ColorDue());
	score += (abs(opponent->rating - tp->rating)) * 0.0001;

	if(!strcmp(opponent->name, "_BYE_")) { score = 999999; }

	added=0;
	newOpp = new Player(opponent->name, score);
	LinkListIter<Player> opponentIter(tp->potentialOpponentList);
	opponentIter.Reset();
	while((temp = opponentIter.Next())) {
	  if(score < temp->floatValue) {
	    tp->potentialOpponentList.Insert(temp, newOpp);
	    added = 1;
	    break;
	  }
	}
	if(!added)
	  tp->potentialOpponentList.Append(newOpp);
	opponentIter.Reset();
	i = 0;
	while((temp = opponentIter.Next())) {
	  temp->value = i;
	  i++;
	}
      }
    }
  }
}

//- Start of PairPlayers ----------------------------------
int Tourney::PairPlayers(TourneyPlayers *p1, TourneyPlayers *p2) {
  TourneyPlayers *tp;
  Player *temp = NULL;

  LinkListIter<TourneyPlayers> playerIter(playerList);

  temp = new Player(p2->name, -1.0, 0, p2->rating);
  p1->opponentList.Append(temp);
  p1->NowPaired(TRUE);
  strcpy(p1->oppName, p2->name);

  temp = new Player(p1->name, -1.0, 0, p1->rating);
  p2->opponentList.Append(temp);
  p2->NowPaired(TRUE);
  strcpy(p2->oppName, p1->name);

  playerIter.Reset();
  while((tp = playerIter.Next())) {
    if((!tp->IsPaired()) && (tp->activeFlag != 0))
      return 0;
  }
  
  return 1;
}

//- Start of UnPairPlayer ----------------------------------
void Tourney::UnPairPlayer(TourneyPlayers *p1) {
  if(p1 != NULL)
    p1->NowPaired(FALSE);
}//- end of UnPairPlayer

//- intcmp ----
int Tourney::intcmp(int a, int b) {
  if(a > b) {
    return 1;
  } else {
    if (a == b) {
      return 0;
    } else {
      return -1;
    }
  }
}
//- end intcmp ----

//- AssignColors ----------------------------------------------------------
void Tourney::AssignColors(TourneyPlayers *p1, TourneyPlayers *p2) {
  int p1Color=0, rated = 1;
  Game *g = NULL;
  Player *opp1 = NULL, *opp2 = NULL;

  cerr << "P1: " << p1->name << " due=" << p1->ColorDue() << " total=" << p1->GetTotalWhites() << "/" << p1->GetTotalBlacks()
	<< " consecutive=" << p1->GetConsecutiveWhites() << "/" << p1->GetConsecutiveBlacks() << endl;
  cerr << "P2: " << p2->name << " due=" << p2->ColorDue() << " total=" << p2->GetTotalWhites() << "/" << p2->GetTotalBlacks()
	<< " consecutive=" << p2->GetConsecutiveWhites() << "/" << p2->GetConsecutiveBlacks() << endl;
  if(params.mode != 'r') { rated = 0; }
  if(intcmp(p1->ColorDue(), p2->ColorDue()) != 0) {
    if(p1->ColorDue()) { p1Color = 1; }
  } else {
    if(p1->ColorDue()) {   // Both are due white; need to find out how due.
      switch (intcmp(p1->GetConsecutiveBlacks(), p2->GetConsecutiveBlacks())) {
      case 1:
	p1Color = 1;
	break;
      case -1: break;
      case 0:
	switch (intcmp(p1->GetTotalBlacks(), p2->GetTotalBlacks())) {
	case 1:
	  p1Color = 1;
	  break;
	case -1: break;
	case 0:
	  if((p1->score * 10000 + p1->rating) >= (p2->score * 10000 + p2->rating))
	    p1Color = 1;
	  break;
	}
	break;
      }
    } else {
      switch (intcmp(p1->GetConsecutiveWhites(), p2->GetConsecutiveWhites())) {
      case 1: break;
      case -1:
	p1Color = 1;
	break;
      case 0:
	switch (intcmp(p1->GetTotalWhites(), p2->GetTotalWhites())) {
	case 1: break;
	case -1:
	  p1Color = 1;
	  break;
	case 0:
	  if((p1->score * 10000 + p1->rating) >= (p2->score * 10000 + p2->rating))
	    p1Color = 1;
	  break;
	}
	break;
      }
    }
  }
  LinkListIter<Player> opponentIter1(p1->opponentList);
  LinkListIter<Player> opponentIter2(p2->opponentList);
  while((opp1 = opponentIter1.Next())) {
    if(!strcasecmp(opp1->name, p2->name)) { break; }
  }
  while((opp2 = opponentIter2.Next())) {
    if(!strcasecmp(opp2->name, p1->name)) { break; }
  }
  cerr << "assigned color = " << p1Color << endl;
  if(p1Color) { 
    p1->AddWhite(); p2->AddBlack();
    opp1->value = 1;
    g = new Game(p1->name, p2->name, params.time, params.inc, rated, 'r');
  } else { 
    p1->AddBlack(); p2->AddWhite();
    opp2->value = 1;
    g = new Game(p2->name, p1->name, params.time, params.inc, rated, 'r');
  }
  gameList.Append(g);
}

//- GetStatus --------------------------------------------------------
int Tourney::GetStatus(void) {
  return status;
}

//- EndTourney -----------------------
void Tourney::EndTourney(void) {
  status = DONE;
}//- End EndTourney

//- IsPaused --------------------------------------------------------
int Tourney::IsPaused(void) {
  return paused;
}

//- SetPause -----------------------
void Tourney::SetPause(int x) {
  paused = x;
}//- End SetPause

//- Announce ----------------------------------------------------------
void Tourney::Announce(void) {
  char temp[128];
  char *announce;
  long now=0;

  announce = new char[MAX_LINE_SIZE];
  memset(announce, '\0', MAX_LINE_SIZE);
  sprintf(announce, "*****Tourney Announcement***** %80s Trny #%d  %d %d %c ", 
	  "", number, params.time, params.inc, params.mode);
  if(params.style == 's') { strcat(announce, " SWISS"); } else { strcat(announce, params.style == 'm' ? " McMahon" : " RR"); }
  switch(params.variant) {
  case 'w':
    strcat(announce, " Wild ");
    strcat(announce, GetWild(params.wild));
    break;
  case 'b':
    strcat(announce, " Bug"); break;
  case 's':
    strcat(announce, " Suicide"); break;
  default:
    break;
  }
  memset(temp, '\0', 128);
  sprintf(temp, " %i-%i %i plr(s).  tell %s join %d. Avg: %5.1f", 
	  params.ratingLow, params.ratingHigh, GetPlayerCount(), gMamer.username, number, averageRating);
  strcat(announce, temp);

  printf("%s  + cha 49\n", announce);
  fflush(stdout);
  
  gMamer.XServerCom("%s %i %s%s", "tell", gMamer.channelNumber, announce, "\n");

  now = time(0);
  if((now - lastCshouted) > (SEC_BETWEEN_CSHOUTS)) {
    gMamer.XServerCom("%s %s%s", "cshout", announce, "\n");
    lastCshouted = now;
  }

  delete(announce);
}

//- SetVariable ---------------------------------------------------------------
void Tourney::SetVariable(int why, int newValue) {

  switch (why) {
  case 0:
    if((newValue >= 0) && (newValue <= MAX_TIME))
      params.time = newValue;
    break;
  case 1:
    if((newValue >= 0) && (newValue <= MAX_INCREMENT))
      params.inc = newValue;
    break;
  case 2:
    params.rounds = newValue;
    params.rounds = MIN(params.rounds, MAX_ROUNDS);
    params.rounds = MIN((params.maxPlayers - 1), params.rounds);
    break;
  case 6:
    if(((newValue >= 0) && (newValue <= 5)) || 
       ((newValue >= 8) || (newValue <= 10)))
      params.wild = newValue;
    break;
  case 7:
    params.ratingLow = newValue;
    params.ratingLow = MAX(0, params.ratingLow);
    if(params.ratingLow >= (params.ratingHigh - 200)) 
      params.ratingLow = params.ratingHigh - 200;
    break;
  case 8:
    params.ratingHigh = newValue;
    if(params.ratingHigh <= (params.ratingLow + 200))
      params.ratingHigh = params.ratingLow + 200;
    break;
  case 9:
    params.maxPlayers = newValue;
    params.maxPlayers = MAX(params.maxPlayers, MINIMUM_PLAYERS);
    params.maxPlayers = MAX(params.maxPlayers, (params.rounds + 1));
    break;
  default:
    break;
  }
}//- End SetVariable

//- SetVariable ---------------------------------------------------------------
void Tourney::SetVariable(int why, const char *newValue) {

  switch (why) {
  case 3:
    if((newValue[0] == 's') || (newValue[0] == 'r') || (newValue[0] == 'm')) 
      params.style = newValue[0];
    break;
  case 4:
    if((newValue[0] == 'r') || (newValue[0] == 'w') || (newValue[0] == 'b') || (newValue[0] == 's'))
      params.variant = newValue[0];
    break;
  case 5:
    if((newValue[0] == 'r') || (newValue[0] == 'u'))
      params.mode = newValue[0];
    break;
  default:
    break;
  }
}//- End SetVariable

//- Begin GetWild - take a int return a string
const char *Tourney::GetWild(int w) {
  switch (w) {
  case 0:
    return "0";
  case 1:
    return "1";
  case 2: 
    return "2";
  case 3:
    return "3";
  case 4:
    return "4";
  case 5:
    return "5";
  case 8:
    return "8";
  case 9:
    return "8a";
  case 10:
    return "fr";
  default:
    return "";
  }
}//- end GetWild

//- TellThemWhoTheyPlay ------------------------------------------
void Tourney::TellThemWhoTheyPlay() {
  Game *g = NULL;
  LinkListIter<Game> gameIter(gameList);  // List of games in this tourney
  char *Variant=new char[MAX_LINE_SIZE];

  memset(Variant, '\0', MAX_LINE_SIZE);

  if(params.variant == 'w')
    sprintf(Variant, "wild %2s", GetWild(params.wild));
  else if(params.variant == 's')
    sprintf(Variant, "suicide");
  else if(params.variant == 'b')
    sprintf(Variant, "bug");

  while((g = gameIter.Next())) {
	  /* note that we rely on rmatch and on the ; separated commands from lasker */
	  sleep(2);
	  gMamer.XServerCom("rmatch %s %s %i %i %c %s white ; rmatch %s %s %i %i %c %s black\n", 
			    g->whiteName, g->blackName, g->time, g->inc, params.mode, Variant,
			    g->blackName, g->whiteName, g->time, g->inc, params.mode, Variant);
  }
  delete(Variant);
}//- end TellThemWhoTheyPlay --------------------------------------

//- SetGameResult --------------------------------------------
int Tourney::SetGameResult(const char *white, const char *black, int result) {
  Player *opp1 = NULL, *opp2 = NULL;
  TourneyPlayers *tp1 = NULL, *tp2 = NULL;
  Game *g = NULL;
  int found=0;

  tp1 = GetPlayer(white);
  tp2 = GetPlayer(black);

  if((NULL == tp1) || (NULL == tp2)) { return 0; }

  LinkListIter<Player> opponentIter1(tp1->opponentList);  // List of opponents this player has had
  while((opp1 = opponentIter1.Next())) {
    if(!strcasecmp(opp1->name, black)) { break; }
  }
  LinkListIter<Player> opponentIter2(tp2->opponentList);
  while((opp2 = opponentIter2.Next())) {
    if(!strcasecmp(opp2->name, white)) { break; }
  }
  if((NULL == opp1) || (NULL == opp2)) { return -1; }

  switch (result) {    // set the result
  case 1:
    opp1->floatValue = 1.0; 
    opp2->floatValue = 0.0; 
    break;
  case 0:
    opp1->floatValue = 0.0; 
    opp2->floatValue = 1.0; 
    break;
  case 2:
    opp1->floatValue = 0.5; 
    opp2->floatValue = 0.5; 
    break;
  default:
    return 0;
  }
  tp1->CalculateScore();  
  tp2->CalculateScore();

  LinkListIter<Game> gameIter(gameList);  // List of games in this tourney
  while((g = gameIter.Next())) {
    if(!(strcasecmp(g->whiteName, white)) && !(strcasecmp(g->blackName, black))) {
      gameList.Delete(g);
      found=1;
      break;
    }
  }
  if(found) {
    gameIter.Reset();
    if((g = gameIter.Next())) {
      return 1;
    } else {
      return 2;
    }
  } else {
    return 1;
  }
} //- End SetGameResult --------------------------------------

//- GetStartDate ---------------------------------------------
long Tourney::GetStartDate() {
  return startDate;
} //- end of GetStartDate ------------------------------------

//- GetEndDate ---------------------------------------------
long Tourney::GetEndDate() {
  return endDate;
} //- end of GetEndDate ------------------------------------
