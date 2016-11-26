//--------------------------------------------------------------------------
// CommandEntry.cpp - Source file for the CommandEntry
//
// Matthew E. Moses & Michael A. Long
//
// $Revision: 1.13 $
// $Date: 1998/09/10 19:57:17 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: CommandEntry.cpp,v $
// Revision 1.13  1998/09/10 19:57:17  mlong
// lots of little bug fixes and a few new features
//
// Revision 1.12  1998/06/18 18:41:30  mlong
// prepairing for yet another move.
//
// Revision 1.11  1998/06/08 20:41:17  mlong
// changes to the list tournies function
//
// Revision 1.10  1998/04/29 15:23:19  mlong
// prepairing for the move to daimi
// new sorting routine.
//
// Revision 1.9  1998/04/18 18:46:04  mlong
// fixed delete bug &
// added delete tourney function
//
// Revision 1.5  1997/10/08 21:03:35  chess
// preparing for move to oracle machine at eworks.
//
// Revision 1.4  1997/05/15 18:27:53  chess
// added Player and TourneyPlayers support
// added HandleGetPlayerInfo & HandleGetGameInfo
//
// Revision 1.3  1997/04/13 03:14:35  chess
// commands to do user statistic manipulationn added:
// setinfo - sets a whole line of information
// setstat - sets a particular stat
// addabuse - adds (or deletes) abuse points.
//
// Revision 1.2  1997/03/21 15:32:36  moses
// added the shutdown command.
//
// Revision 1.1  1996/10/01  20:14:43  moses
// Initial revision
//
//--------------------------------------------------------------------------

// static char RCSid[] = "$Id: CommandEntry.cpp,v 1.13 1998/09/10 19:57:17 mlong Exp $";

#include "CommandEntry.hh"
#include "Mamer.hh"

extern Mamer gMamer;

//- AddAbuse -------------------------------------------------------
int CommandEntry::AddAbuse(User *user, param_list params) {
    User *u = NULL;
    char name[MAX_WORD_SIZE];
    int needToDelete=0;

    if(params[0].type == TYPE_WORD) {
      strcpy(name, params[0].val.word);
    }
    
    u = gMamer.FindUser(name);
    if(u == NULL) {
      u = new User();
      u->LoadPlayer(gMamer.userFilePath, name);  // Loads the player info into memory and saves current info to disk
      needToDelete = 1;
    }
    
    if((u->GetManagerLevel() >= user->GetManagerLevel()) &&
       (user->GetManagerLevel() != PRESIDENT)) {
      if(needToDelete) delete u;
      gMamer.TellUser(NoPermissions, user->name);
      return 0;
    }

    if(u != NULL) {
      if(params[1].type == TYPE_INT) {
	u->AddAbuse(params[1].val.integer);
      } else {
	u->AddAbuse(10);
      }
      u->SavePlayer(gMamer.userFilePath);
      if(u->GetAbuse() >= MAX_CHAOS_POINTS)
	gMamer.XServerCom("%s %s%s", "+ censor", u->name, "\n");
      gMamer.TellUser(ChangedAbuse, user->name, u->name, u->GetAbuse());
    } else {
      gMamer.TellUser(NotFound, user->name, name);
    }
    if(needToDelete) delete u;

    return(1);
} //- End of AddAbuse

//- FingerUser -------------------------------------------------------
int CommandEntry::FingerUser(User *user, param_list params) {
    User *u = NULL;
    int notFound=1;
    char name[32];

    memset(name, '\0', 32);
    if(params[0].type == TYPE_WORD) {
      strncpy(name, params[0].val.word, MIN(31, (int)strlen(params[0].val.word)));
    } else {
      strcpy(name, user->name);
    }
    
    u = gMamer.FindUser(name);
    if(u != NULL) notFound = 0;
    
    if(notFound) { 
      u = new User();
      notFound = u->LoadPlayer(gMamer.userFilePath, name);
      notFound = !notFound;
    }
    if(!notFound) {
      gMamer.XServerCom("%s %s %s%s%s", "qtell", user->name, "\\n", u->name, "'s Stats:\\n\\n");
      gMamer.XServerCom("%-17s %5s %4s %4s %3s %3s %3s %3s %5s %6s%-17s %4s %3s %3s %3s %3s %3s %3s %5s %6s",
			"Name", " Tnys", "  W ", "  L ", " D ", "1st", "2nd", "3rd", "Chaos", "Rating\\n",
			"-----------------", "-----", "----", "----", "---", "---", "---", "---", "-----","------\\n");
      gMamer.XServerCom("%-17s %5ld %4ld %4ld %3ld %3ld %3ld %3ld %5d %6.2f %s",
			u->name, u->GetPlayedTourneys(), u->GetWins(), u->GetLosses(), u->GetDraws(),
			u->GetFirsts(), u->GetSeconds(), u->GetThirds(), u->GetAbuse(), u->GetRating(), "\\n");
      if(u->GetManagerLevel() > USER) {
	gMamer.XServerCom("%s %d %s %d %s", "\\nManager Level:", u->GetManagerLevel(), 
			                    "    Managed Tourneys:", u->GetManagedTourneys(), "\\n");
      }
      gMamer.XServerCom("%s", "\n");
    } else {
      gMamer.TellUser(NotFound, user->name, name);
    }
    if(notFound) delete u;

    return(1);
} //- End of FingerUser

//- ListRank ----------------------------------------------------------
int CommandEntry::ListRank(User *user, param_list params) {
  float rating=0;
  int start=0, i=0, end=0, tourneys=0, counter=0;
  char filename[128], name[32], name2[32];
  FILE *theFile;

  memset(name, '\0', 32);
  memset(name2, '\0', 32);
  switch (params[0].type) {
  case TYPE_WORD:
    strcpy(name, params[0].val.word);
    break;
  case TYPE_INT:
    start = params[0].val.integer;
    if(start <= 0) {start = 1;}
    break;
  default:
    strcpy(name, user->name);
    break;
  }
  
  sprintf(filename, "%s/rank", gMamer.dataFilePath);
  if(params[0].type != TYPE_INT) {
    if((theFile = fopen(filename, "r"))) {
      while(fscanf(theFile, "%s %d %f", name2, &tourneys, &rating) > 0) {
	if(!(strncasecmp(name2, name, strlen(name)))) {
	  start = i+1;
	  break;
	}
	i++;
      }
      fclose(theFile);
    } else {
      gMamer.TellUser(NotFound, user->name, filename);
      return(0);
    }
  }
  if(!(start)) {
    gMamer.TellUser(NotFound, user->name, name);
    return(1);
  }
  start = start - 10;
  end = start + 20;
  if(start <= 0) {start = 1; end = 21;}
  gMamer.XServerCom("%s %s %s %s%s %-5s %-18s %5s %6s %5s %-18s %5s %6s", "qtell", user->name, "\\n", gMamer.username, 
		    "'s Rankings:\\n\\n", "Rank", "Name", "Trnys", "Rating\\n", 
		    "-----", "------------------", "-----", "------\n");
  gMamer.XServerCom("%s %s %s", "qtell", user->name, "\\n");
  if(!(theFile = fopen(filename, "r"))) { return(0); }
  i = 1; counter = 1;
  while(fscanf(theFile, "%s %d %f", name2, &tourneys, &rating) > 0) {
    if((i >= start) && (i < end)) {
      if(i == start) { counter = 1; }
      gMamer.XServerCom(" %-5d %-18s %5d %6.2f\\n", i, name2, tourneys, rating);
      if(((counter % 10) == 0) && (counter > 0)) {
	gMamer.XServerCom("%s", "\n");
	if(i != (end -1)) {
	  gMamer.XServerCom("%s %s %s", "qtell", user->name, "\\n");
	}
      }
    }
    if(i > end) { break; }
    i++; counter++;
  }
  fclose(theFile);
  
  if(i <= end) { gMamer.XServerCom("%s", "\n"); }

  return(1);
}

//- ListManagers ------------------------------------------------------
int CommandEntry::ListManagers(User *user, param_list params) {
    int i=1, needToDelete=0;
    long last;
    char filename[256], manager[NAMELEN], date[16];
    User *u=NULL, *newUser = NULL;
    struct tm *tmpDate;
    FILE *theFile;

    i = params[0].type; // just to get rid of the compiler messages
    sprintf(filename, "%s/managers", gMamer.dataFilePath);
    if((theFile = fopen(filename, "r"))) {
      gMamer.XServerCom("%s %s %s %s%s", "qtell", user->name, "\\n", gMamer.username, "'s Manager List:\\n\\n");
      gMamer.XServerCom("%2s%-18s %3s %4s %-8s%2s%-18s %3s %4s %-8s%s",
			"","Name","Lvl","Tnys","Last", "","Name","Lvl","Tnys","Last", "\\n");
      gMamer.XServerCom("  %-18s %3s %4s %-8s%2s%-18s %3s %4s %-8s%s",
			"-----------------", "---", "----", "--------", "",
			"-----------------", "---", "----", "--------", "", "\\n");
      i=1;
      memset(date, '\0', 16);
      gMamer.XServerCom("\n%s %s ", "qtell", user->name);
      while(fscanf(theFile, "%s", manager) > 0) {
	needToDelete = 0;
	u = gMamer.FindUser(manager);
	if(u == NULL) {
	  needToDelete = 1;
	  newUser = new User();
	  u = newUser;
	}
	if(0 != u->LoadPlayer(gMamer.userFilePath, manager)) {
	  last = u->GetLast();
	  if(last) {
	    tmpDate = localtime(&last);	  
	    sprintf(date, "%02d/%02d/%02d", tmpDate->tm_mon+1, tmpDate->tm_mday, tmpDate->tm_year);
	  } else {
	    sprintf(date, "none");
	  }
	} else {
	  sprintf(date, "%s", "none");
	}
	gMamer.XServerCom("%2s%-18s %3d %4d %8s%s",
			  ((gMamer.UserIsLoaded(manager)) ? "+" : " "),
			  manager, u->GetManagerLevel(), u->GetManagedTourneys(), date, (i%2)==0 ? "\\n":"");
	if(((i % 20) == 0) && (i > 0)) {
	  i = 0;
	  gMamer.XServerCom("%s %s %s %s", "\n", "qtell", user->name, "\\n");
	}
	i++;
	if(needToDelete) {
	  u = NULL;
	  delete(newUser);	  
	}
      }
      fclose(theFile);
      gMamer.XServerCom("%s", "\n");
      
      return(1);
    }    
    gMamer.TellUser(NotFound, user->name, filename);
    return(0);
}

//- LoadedUsers -------------------------------------------------------
int CommandEntry::LoadedUsers(User *user, param_list params) {
  User *u = NULL;
  LinkListIter<User> userIter(gMamer.userList);
  int i, count=0;
  
  i = params[0].type;
  i = 1;
  
  gMamer.XServerCom("qtell %s \\nLoaded Users:\\n\\n", user->name);
  while((u = userIter.Next())) {
    count++;
    gMamer.XServerCom("  %18s%s", u->name, (i%3)==0 ? "\\n":"");
    if(((i % 30) == 0) && (i > 0)) {
      i = 0;
      gMamer.XServerCom("%s %s %s %s", "\n", "qtell", user->name, "\\n");
    }    
    i++;
  }
  gMamer.XServerCom("%sTotal: %i%s", "\\n", count, "\n");
  
  return(1);
} //- End of LoadedUser

//- SetCommandLevel ----------------------------------------------------
int CommandEntry::SetCommandLevel(User *user, param_list params) {
  Command *c = NULL;

  c = gMamer.FindCommand(params[0].val.word, user->name);

  if(c != NULL) {
    if((c->GetManagerLevel() <= user->GetManagerLevel()) && 
       (params[1].val.integer <= user->GetManagerLevel())) {
      c->SetManagerLevel((ranks)params[1].val.integer);
      gMamer.TellUser(ChangedCommandLevel, user->name, params[0].val.word, params[1].val.integer);
    } else
      gMamer.TellUser(NoPermissions, user->name);
  } else 
    return 0;
  
  return 1;
}

//- SetInfo -------------------------------------------------------
int CommandEntry::SetInfo(User *user, param_list params) {
    User *u = NULL;
    int notFound=1, i;
    char name[32];

    memset(name, '\0', 32);
    if(params[0].type == TYPE_WORD) { 
      strncpy(name, params[0].val.word, MIN(31, (int)strlen(params[0].val.word))); 
    }

    u = gMamer.FindUser(name);
    if(u != NULL) notFound = 0;

    if(notFound) {
      u = new User();
      u->LoadPlayer(gMamer.userFilePath, name);
    }

    if(u != NULL) {
      if((u->GetManagerLevel() >= user->GetManagerLevel()) &&
	 (user->GetManagerLevel() != PRESIDENT)) {
	if(notFound) delete u;
	gMamer.TellUser(NoPermissions, user->name);
	return 0;
      }
      for(i=1; i<=7; i++)
	u->SetStatistic(i, params[i].val.integer);
      u->SavePlayer(gMamer.userFilePath);
      gMamer.TellUser(ChangedInfo, user->name, u->name);
    } else {
      gMamer.TellUser(NotFound, user->name, name);
    }
    if(notFound) delete u;

    return(1);
} //- End of SetInfo

//- SetManagerLevel -------------------------------------------------------
int CommandEntry::SetManagerLevel(User *user, param_list params) {
  User *u = NULL;
  int notFound=1, new_level=1, length=0, i=0;
  char name[32];
  
  if(params[0].type == TYPE_WORD) { 
    length = strlen(params[0].val.word);
    memset(name, '\0', 32);
    while((i < 31) && (i < length)) {
      name[i] = tolower(params[0].val.word[i]);
      i++;
    }
  }
  new_level = params[1].val.integer;
  
  u = gMamer.FindUser(name);
  if(u != NULL) notFound = 0;
  
  if(notFound) {
    u = new User();
    u->LoadPlayer(gMamer.userFilePath, name);
  }
  if(((u->GetManagerLevel() >= user->GetManagerLevel()) ||
      (new_level >= user->GetManagerLevel())) &&
     (user->GetManagerLevel() != PRESIDENT)) {
    if(notFound) delete u;
    gMamer.TellUser(NoPermissions, user->name);
    return 0;
  }

  if(u != NULL) {
    u->ChangeManagerLevel(new_level);
    u->SavePlayer(gMamer.userFilePath);
    gMamer.TellUser(ChangedManagerLevel, user->name, u->name, new_level);
  } else {
    gMamer.TellUser(NotFound, user->name, name);
  }
  if(notFound) delete u;
  
  if(new_level > 0)
    return(1);
  else
    return -1;
} //- End of SetManagerLevel


//- SetStat -------------------------------------------------------
int CommandEntry::SetStat(User *user, param_list params) {
    User *u = NULL;
    int notFound=1, new_value, i, ret=0, size=0, counter=0;
    char which_stat[64], name[32];
    strings statAliases[] = {
      {"tourneys", 1}, {"tnys", 1},
      {"wins", 2},
      {"losses", 3}, {"lose", 3},
      {"draws", 4},
      {"firsts", 5}, {"1sts", 5},
      {"seconds", 6}, {"2nds", 6},
      {"thirds", 7}, {"3rds", 7},
      {"abuse", 8}, 
      {"rating", 9},
      {"managedtourneys", 10},
      {NULL}
    };

    memset(which_stat, '\0', 64);
    if(params[0].type == TYPE_WORD) { strcpy(name, params[0].val.word); }
    if(params[1].type == TYPE_WORD) { strncpy(which_stat, params[1].val.word, MIN(63, strlen(params[1].val.word))); }    
    size = strlen(which_stat);
    new_value = params[2].val.integer;

    u = gMamer.FindUser(name);
    if(u != NULL) notFound = 0;    

    if(notFound) { 
      u = new User(); 
      u->LoadPlayer(gMamer.userFilePath, name);
    }
    if(u != NULL) {
      if((u->GetManagerLevel() >= user->GetManagerLevel()) &&
	 (user->GetManagerLevel() != PRESIDENT)) {
	if(notFound) delete u;
	gMamer.TellUser(NoPermissions, user->name);
	return 0;
      }
      i=0;
      while(statAliases[i].string != NULL) {
	if (!(strncasecmp(statAliases[i].string, which_stat, MIN(size, (int)strlen(statAliases[i].string))))) {
	  counter++;
	  if(counter > 1) break;
	}       
	i++;
      }
      if(counter > 1) {
	gMamer.TellUser(CanNotChange, user->name, u->name, which_stat, new_value);
      } else if(counter == 0) {
	gMamer.TellUser(NotFound, user->name, which_stat);
      } else {
	i=0;
	while(statAliases[i].string != NULL) {
	  if (!(strncasecmp(statAliases[i].string, which_stat, MIN(size, (int)strlen(statAliases[i].string))))) {
	    ret = u->SetStatistic(statAliases[i].number, new_value);
	    memset(which_stat, '\0', 64);
	    strcpy(which_stat, statAliases[i].string);
	    break;
	  }
	  i++;
	}
	u->SavePlayer(gMamer.userFilePath);
	if(ret)
	  gMamer.TellUser(ChangedInfo, user->name, u->name, which_stat, new_value);
	else 
	  gMamer.TellUser(NotFound, user->name, which_stat);
      }
    } else {
      gMamer.TellUser(NotFound, user->name, name);
    }
    if(notFound) delete u;

    return(1);
} //- End of SetStat

//- ShowCommands --------------------------------------------
int CommandEntry::ShowCommands(User *user, param_list params) {
  Command *c = NULL;
  LinkListIter<Command> commIter(gMamer.commandList);
  char *command;
  int i;

  if(params[0].type == TYPE_WORD) {
    command = params[0].val.word;
    while((c = commIter.Next())) if(1 == c->IsCommand(command)) break;
    if(c == NULL) {
      gMamer.TellUser(NotFound, user->name, command);
      return 0;
    }
    gMamer.XServerCom("qtell %s %s Notes: %-16s | %-5s | %3d | %s \n", 
	    user->name, gMamer.username, c->GetCommandName(), 
	    c->GetCommandAlias(), c->GetManagerLevel(), c->GetCommandDescription());
    return(1);
  }
  gMamer.XServerCom("qtell %s %s's Command List:\\n\\n", user->name, gMamer.username);
  i = 0;
  while((c = commIter.Next())) {
    gMamer.XServerCom(" %-16s | %-5s | %3d | %s\\n",
	    c->GetCommandName(), c->GetCommandAlias(), c->GetManagerLevel(), c->GetCommandDescription());
    if((!(i % 9)) && (i > 0)) {
        i = 0;
        gMamer.XServerCom("%s", "\n");
        gMamer.XServerCom("qtell %s \\n", user->name);
    }
    i++;
  }
  gMamer.XServerCom("%s", "\n");
  return(1);
}

//- ShowHelp -----------------------------------------------
int CommandEntry::ShowHelp(User *user, param_list params) {
  int i=1;
  char tmpBuffer[1024], request[128], filename[256];
  FILE *theFile;

  memset(request, '\0', 128);
  if(params[0].type == TYPE_WORD) { 
    strcpy(request, params[0].val.word);
  } else { 
    strcpy(request, "index"); 
  }
  sprintf(filename, "%s/%s", gMamer.helpFilePath, request);
  if((theFile = fopen(filename, "r"))) {
    gMamer.XServerCom("qtell %s \\nThe %s Help File:\\n\\n", user->name, request);
    i=1;
    memset(filename, '\0', 256);
    while(fgets(filename, 79, theFile)) {    /* Just reusing the variable filename could be any char [] */      
      memset(tmpBuffer, '\0', 1024);
      strcpy(tmpBuffer, gMamer.s_and_r(filename, "\n", "\\n"));
      gMamer.XServerCom("%s", tmpBuffer);
      if(((i % 10) == 0) && (i > 0)) {
	i = 0;
	gMamer.XServerCom("\nqtell %s \\n", user->name);
      }
      i++;
      memset(filename, '\0', 256);
    }
    fclose(theFile);
    gMamer.XServerCom("\n");
    
    return(1);
  }
  gMamer.TellUser(NotFound, user->name, request);
  return(0);
}//- End of ShowHelp

//- CreateTourney ------------------------------------------------------------
int CommandEntry::CreateTourney(User *user, param_list params) {
  Tourney *t = NULL;
  int num = gMamer.GenerateTourneyNumber();

  params[0].type = params[0].type;  // Just to stop the annoying unused variable messages during compile.
  t = new Tourney(num, user, &(gMamer.tourneyParams));
  gMamer.tourneyList.Append(t);
  gMamer.XServerCom("%s %s %s %d %s", "xtell", user->name, "Created tourney number: ", t->number, "\n");  
  return(1);
}//- End CreateTourney

//- OpenTourney ------------------------------------------------------------
int CommandEntry::OpenTourney(User *user, param_list params) {
  Tourney *tourn = NULL;

  tourn = gMamer.FindTourney(params[0].val.integer);
  if(NULL != tourn) {
    if(tourn->Open()) {
      tourn->Announce();
      return(1);
    }
  }
  gMamer.TellUser(NotFound, user->name, "tourney");  
  return(0);
}//- End OpenTourney

//- PauseTourney ------------------------------------------------------------
int CommandEntry::PauseTourney(User *user, param_list params) {
  Tourney *tourn = NULL;

  tourn = gMamer.FindTourney(params[0].val.integer);
  if(NULL != tourn) {
    if(tourn->GetStatus() == CLOSED) {
      tourn->SetPause(TRUE);
    }
    return(1);
  }
  gMamer.TellUser(NotFound, user->name, "tourney");  
  return(0);
}//- End PauseTourney

//- ResumeTourney ------------------------------------------------------------
int CommandEntry::ResumeTourney(User *user, param_list params) {
  Tourney *tourn = NULL;

  tourn = gMamer.FindTourney(params[0].val.integer);
  if(NULL != tourn) {
    if(tourn->GetStatus() == CLOSED && tourn->IsPaused()) {
      tourn->SetPause(FALSE); // unpause
      gMamer.NextRound(); // and start next round
    }
    return(1);
  }
  gMamer.TellUser(NotFound, user->name, "tourney");  
  return(0);
}//- End ResumeTourney

//- AnnounceTourney ----------------------------------------------------------
int CommandEntry::AnnounceTourney(User *user, param_list params) {
  Tourney *tourn = NULL;

  tourn = gMamer.FindTourney(params[0].val.integer);
  if(NULL != tourn) {
    if(tourn->GetStatus() == OPEN) {
      tourn->Announce();
      return(1);
    } else {
      gMamer.TellUser(TourneyNotOpen, user->name, params[0].val.integer);
      return 0;
    }
  } else {
    gMamer.TellUser(NotFound, user->name, "tourney");  
    return(0);
  }
}//- AnnounceTourney ---------------------------------------------------------

//- KeepTourney ------------------------------------------------------------
int CommandEntry::KeepTourney(User *user, param_list params) {
  Tourney *t = NULL;

  t = gMamer.FindTourney(params[0].val.integer);
  if(NULL != t) {
    if(params[1].type != TYPE_NULL) 
      if(params[1].type == TYPE_WORD) {
	if(strncasecmp("y", params[1].val.word, 1) == 0) {
	  t->SetPersist(1);
	  gMamer.TellUser(WillKeepTourney, user->name, params[0].val.integer);
	} else {
	  t->SetPersist(0);
	  gMamer.TellUser(NotKeepTourney, user->name, params[0].val.integer);
	}
      } else if(params[1].type == TYPE_INT) {
	t->SetPersist(params[1].val.integer);
	if(params[1].val.integer)
	  gMamer.TellUser(WillKeepTourney, user->name, params[0].val.integer);
	else
	  gMamer.TellUser(NotKeepTourney, user->name, params[0].val.integer);
      } else {
	gMamer.TellUser(NotFound, user->name, "tourney");  
	return(0);
      }
    return(1);
  }

  gMamer.TellUser(NotFound, user->name, "tourney");  
  return(0);
}//- End KeepTourney

//- DeleteTourney ------------------------------------------------------------
int CommandEntry::DeleteTourney(User *user, param_list params) {
  Tourney *t = NULL;
  TourneyPlayers *tp = NULL;

  t = gMamer.FindTourney(params[0].val.integer);
  if(NULL != t) {
    if(t->GetStatus() != DONE) {
      LinkListIter<TourneyPlayers> playerIter(t->playerList);
      playerIter.Reset();
      while((tp = playerIter.Next())) {
	gMamer.XServerCom("%s %s %d%s", "tournset", tp->name, 0, "\n");
	gMamer.XServerCom("tell %s Tourney#%d has been deleted.%s", tp->name, t->number, "\n");
      }
      gMamer.XServerCom("%s %d %s%d %s", "tell", gMamer.channelNumber, 
 			"Tourney #", params[0].val.integer, "has been deleted.\n"); 
    }
    gMamer.tourneyList.Delete(t);  // delete the tourney
    return(1);
  }
  
  gMamer.TellUser(NotFound, user->name, "tourney");  
  return(0);
}//- End DeleteTourney

//- CloseTourney ------------------------------------------------------------
int CommandEntry::CloseTourney(User *user, param_list params) {
  Tourney *tourn = NULL;

  tourn = gMamer.FindTourney(params[0].val.integer);
  if(NULL != tourn) {
    if(tourn->GetPlayerCount() >= MINIMUM_PLAYERS) {
      if(tourn->GetStatus() == OPEN) {      
	tourn->CloseAndStart();
	gMamer.XServerCom("qtell %s %s Notes: %s %d %s", 
			  user->name,gMamer.username,"Tourney #", 
			  tourn->number, " is now closed.\n");
	return(1);
      } else {
	gMamer.TellUser(TourneyNotOpen, user->name, tourn->number);
      }
    } else {
      gMamer.TellUser(NotEnoughPlayers, user->name, tourn->number);
    }
  } else {
    gMamer.TellUser(TourneyNotFound, user->name, params[0].val.integer);
  }
    
  return(0);
}//- End CloseTourney

int CommandEntry::ListTourneys(User *user, param_list params) {
  Tourney *t = NULL;
  LinkListIter<Tourney> tournIter(gMamer.tourneyList);
  int notourneys = 1, Tstatus=0, i=3;
  long stDate, enDate, timeNow;
  struct tm *start, *end;
  char outStart[128], outEnd[128], outStatus[128];  

  params[0].type = params[0].type;  // Just to stop the annoying unused var messages in compile.
  while((t = tournIter.Next())) notourneys = 0;

  if(notourneys == 0) {
    gMamer.XServerCom("qtell %s %s Notes: \\n", user->name, gMamer.username);
    gMamer.XServerCom(" %3s %3s %3s %4s %3s %2s %4s %9s %6s %-14s %-14s\\n", 
		      "No.","Rds","Sty", "Time", "Inc", "Md", "Vrnt", "Rtng Rnge", "Status","  Started at  ", "   Ended at   ");
    gMamer.XServerCom(" %3s %3s %3s %4s %3s %2s %4s %9s %6s %-14s %-14s\\n", 
		      "---","---","---", "----", "---", "--", "----", "---------", "------","--------------", "--------------");
    tournIter.Reset();
    while((t = tournIter.Next())) {
      stDate = t->GetStartDate();
      enDate = t->GetEndDate();
      Tstatus = t->GetStatus();
      if((Tstatus == DONE) && (t->GetPersist() == 0)){
	timeNow = time(0);
	if((timeNow - enDate) > KEEP_TOURNEY_TIME) {
	  gMamer.tourneyList.Delete(t);
	  continue;
	}
      }
      if(stDate > 0) {
	start = localtime(&stDate);
	sprintf(outStart, "%02d:%02d %02d/%02d/%02d", 
		start->tm_hour, start->tm_min, start->tm_mon+1, start->tm_mday, start->tm_year);
      } else { strcpy(outStart, "n/a"); }      
      if(enDate > 0) {
	end = localtime(&enDate);
	sprintf(outEnd, "%02d:%02d %02d/%02d/%02d", 
		end->tm_hour, end->tm_min, end->tm_mon+1, end->tm_mday, end->tm_year);
      } else { strcpy(outEnd, "n/a"); }
      if(Tstatus == NEW)
	sprintf(outStatus, "%s", "new");
      else if(Tstatus == OPEN) 
	sprintf(outStatus, "%s", "open");
      else if(Tstatus == CLOSED)
	sprintf(outStatus, "%s", t->IsPaused() ? "paused" : "closed");
      else if(Tstatus == DONE)
	sprintf(outStatus, "%s", "done");
      else
	memset(outStatus, '\0', 128);

      gMamer.XServerCom(" %3d %3d %3c %4d %3d %2c %4c %4d-%4d %6s %-14s %-14s\\n", 
			t->number, t->params.rounds, t->params.style, t->params.time, t->params.inc, 
			t->params.mode, t->params.variant, t->params.ratingLow, t->params.ratingHigh, 
			outStatus, outStart, outEnd);
      if(((i % 12) == 0) && (i > 0)) {
	i = 0;
	gMamer.XServerCom("%s %s %s %s", "\n", "qtell", user->name, "\\n");
      }
      i++;      
    }
    gMamer.XServerCom("%s", "\n");
  } else {
    gMamer.XServerCom("qtell %s %s Notes: %s", user->name, gMamer.username, "No tourneys right now.\n");    
  }

  return (1);
}

//- JoinTourney ------------------------------------------------------------
int CommandEntry::JoinTourney(User *user, param_list params) {
  Tourney *tourn = NULL;
  Player *newEntry = NULL;
printf("join\n");
  tourn = gMamer.FindTourney(params[0].val.integer);
  
  if(NULL != tourn) {    
    newEntry = new Player(user->name, 0., params[0].val.integer); // [HGM] signal this was from join
printf("entry=%d\n",newEntry);
    gMamer.pendingList.Append(newEntry);
    gMamer.XServerCom("getpi %s%s", user->name, "\n");
    return(1);
  }
printf("error\n");
  gMamer.TellUser(TourneyNotFound, user->name, params[0].val.integer);
  return(0);
}

//- AddToTourney ------------------------------------------------------------
int CommandEntry::AddToTourney(User *user, param_list params) {
  Tourney *tourn = NULL;
  Player *newEntry = NULL;

  tourn = gMamer.FindTourney(params[1].val.integer);
  
  if(NULL != tourn) {
    newEntry = new Player(params[0].val.word, 1., params[1].val.integer); // [HGM] signal this was from att
    gMamer.pendingList.Append(newEntry);
    gMamer.XServerCom("getpi %s%s", params[0].val.word, "\n");
    return(1);
  }

  gMamer.TellUser(TourneyNotFound, user->name,  params[1].val.integer);
  return(0);
}

//- RemoveFromTourney ------------------------------------------------------------
int CommandEntry::RemoveFromTourney(User *user, param_list params) {
  Tourney *tourn = NULL;
  TourneyPlayers *tp=NULL;
  char name[NAMELEN], reason[64];
  int num=0;
  User *u=NULL;
  int chaosPointsEarned=0, needToDelete=0;

  memset(name, '\0', NAMELEN);
  memset(reason, '\0', 64);
  if(params[0].type == TYPE_INT) {  // if we are withdrawing ourselves
    tourn = gMamer.FindTourney(params[0].val.integer);
    strcpy(name, user->name);
    u = user;
    num = params[0].val.integer;
    strcpy(reason, "withdrew");
  } else {  // if a manager is forfeiting us
    tourn = gMamer.FindTourney(params[1].val.integer);
    strcpy(name, params[0].val.word);
    u = gMamer.FindUser(params[0].val.word);
    num = params[1].val.integer;
    strcpy(reason, "was forfeited");
  }

  if(NULL == tourn) {
    gMamer.TellUser(TourneyNotFound, user->name, num);
    return 0;
  }
  if(tourn->GetStatus() == DONE) {
    gMamer.TellUser(TourneyDone, user->name, num);
    return 0;
  }
  tp = tourn->GetPlayer(name);   //Get the players info
  if(tp == NULL) {
    gMamer.TellUser(NotFound, user->name, name, num);// Player is not found in this tourney
    return 0;
  }

  gMamer.XServerCom("%s %s %d%s", "tournset", name, 0, "\n");
  if(tourn->IsNotClosed()) { //If we get past this check it will cost the user chaos points
    tourn->playerList.Delete(tp);
    tourn->CalculateAverage();
    gMamer.TellUser(PlayerRemoved, user->name, name, num);
    gMamer.XServerCom("%s %d %s %s %s%d %d%s\n","tell", gMamer.channelNumber, name, reason, "from tourney #", 
		      tourn->number, tourn->GetPlayerCount(), " player(s) now");
    return 0; 
  } // otherwise tourney is closed and started

  chaosPointsEarned = tourn->RemovePlayer(name);  // RemovePlayer will return the number of rounds
  if(chaosPointsEarned >= 0) {                    // that were disturbed
    if(NULL == u) {
      u = new User(gMamer.userFilePath, name);  // Make a new user - this will create a file but there
      needToDelete = 1;                         // should already be one cause they are in the tourney
    }
    u->AddAbuse(chaosPointsEarned * PENALTY_PER_ROUND);  // add the choas points and save them
    u->SavePlayer(gMamer.userFilePath);
    if(u->GetAbuse() >= MAX_CHAOS_POINTS)
      gMamer.XServerCom("%s %s%s", "+ censor", u->name, "\n");
    if(needToDelete) delete(u);                 // we created a new user so we should delete him here
    gMamer.TellUser(PlayerRemoved, user->name, name, num);
    gMamer.XServerCom("%s %d %s %s %s%d\n","tell",gMamer.channelNumber,name, reason,
		      "from tourney #", tourn->number);
    return 1;
  }
  return 1;
}

//- ListTourneyGames ------------------------------------------------------------
int CommandEntry::ListTourneyGames(User *user, param_list params) {
  Tourney *t = NULL;
  TourneyPlayers *white, *black;
  int i = 0;
  Game *g = NULL;
  
  t = gMamer.FindTourney(params[0].val.integer);
  if(NULL != t) {
    LinkListIter<Game> gameIter(t->gameList);
    gameIter.Reset();
    gMamer.XServerCom("%s %s %s %d %s",  "qtell", user->name, "Tourney Games for Round #", t->GetRound(), "\\n\\n");
    gMamer.XServerCom("%3s %18s %6s %6s %2s %-18s %6s %6s %s",
		      "", "White", "[SCR ]", "[Rtng]", "vs", "Black", "[SCR ]", "[Rtng]",
                      "\\n---------------------------------------------------------------------------\\n");
    while((g = gameIter.Next())) {
      if(!(i % 10) && (i>0)) {
        gMamer.XServerCom("\nqtell %s %s", user->name, "\\n");
      }

      white = t->GetPlayer(g->whiteName);
      black = t->GetPlayer(g->blackName);

      if(g->gameNumber > 0) {
	gMamer.XServerCom("%3d %18s [%4.1f] [%4i] vs %-18s [%4.1f] [%4i] %3i%s",
			  i+1, g->whiteName, white->score, white->rating,
			  g->blackName, black->score, black->rating, g->gameNumber, "\\n");
      } else {
	gMamer.XServerCom("%3d %18s [%4.1f] [%4i] vs %-18s [%4.1f] [%4i] none%s",
			  i+1, g->whiteName, white->score, white->rating,
			  g->blackName, black->score, black->rating, "\\n");
      }
      i++;
    }
    gMamer.XServerCom("%s", "\\n\n");
    return(1);
  }

  gMamer.TellUser(TourneyNotFound, user->name, params[0].val.integer);
  return(0);
}//- End of ListTourneyGames

//- ListTourneyPlayers ------------------------------------------------------------
int CommandEntry::ListTourneyPlayers(User *user, param_list params) {
  Tourney *t = NULL;
  Player *p = NULL, *opp=NULL;
  TourneyPlayers *tp = NULL;
  char color, result;
  int i = 0, counter = 0;

  t = gMamer.FindTourney(params[0].val.integer);
  if(NULL != t) {
    if(t->GetPlayerCount() == 0) {
      gMamer.TellUser(NoPlayers, user->name, params[0].val.integer);
      return 0;
    }
    t->SortPlayers();
    gMamer.XServerCom("%s %s %s %s %d %s %d %s %3s %-17s %6s %5s %6s %-6s %-7s %s %3s %-17s %6s %5s %6s %6s %-7s %s", 
		      "qtell", user->name, "Tourney Players:", "Round", t->GetRound(), "of", t->params.rounds, 
		      "\\n\\n", "", "Name", "Rating", "Score", "Perfrm", "Upset ", "Results", "\\n",
		      "", "-----------------", "------", "-----", "------", "------", "-------", "\\n");
    LinkListIter<TourneyPlayers> playerIter(t->playerList);
    playerIter.Reset();
    while((tp = playerIter.Next())) { counter++; }  // count the number of players
    for(i=1; i<=counter; i++) {
      p = t->GetSortPlayer(i);
      tp = t->GetPlayer(p->name);
      if(tp->activeFlag > 0) 
	gMamer.XServerCom("%3d %s%-17s [%4d]  %3.1f  [%4d] [%4d] ", 
			  i, ((tp->location == ONLINE) ? "+" : "-"), 
			  tp->name, tp->rating, tp->score, tp->perform, tp->upset);
      else 
	gMamer.XServerCom("%3d %s%-17s [%4s]  %3.1f  [%4d] [%4d] ", 
			  i, ((tp->location == ONLINE) ? "+" : "-"), 
			  tp->name, "forf", tp->score, tp->perform, tp->upset);
      LinkListIter<Player> opponentIter(tp->opponentList);  // List of opponents this player has had
      opponentIter.Reset();
      while((opp = opponentIter.Next())) {
	p = t->GetSortPlayer(opp->name);
	if(opp->value) { color = 'w'; } else { color = 'b'; }
	if(opp->floatValue == 1.0) {
	  result = '+';
	} else if(opp->floatValue == 0.5) { 
	  result = '='; 
	} else if(opp->floatValue == 0.0) { 
	  result = '-'; 
	} else {
	  result = '*'; 
	}
      	gMamer.XServerCom("%c%-0.2d%c ", result, p->value, color);
      }
      if(((i % 8) == 0) && (i > 0)) {
	gMamer.XServerCom("%s %s %s %s", "\n", "qtell", user->name, "\\n");
      } else {
	gMamer.XServerCom("%s", "\\n");
      }
    }
    gMamer.XServerCom("%-24s %6.1f %s", "\\n     Average Rating", t->GetAverageRating(), "\\n\n");
    return(1);
  }

  gMamer.TellUser(TourneyNotFound, user->name, params[0].val.integer);
  return(0);
}//- End of ListTourneyPlayers

//- ListTourneyVars -----------------------------------------------------
int CommandEntry::ListTourneyVars(User *user, param_list params) {
  Tourney *tourn = NULL;

  tourn = gMamer.FindTourney(params[0].val.integer);

  if(NULL != tourn) {
    gMamer.XServerCom("%s %s %s", "qtell", user->name, "\\n");
    gMamer.XServerCom(" %18s %4d %s", "(T)ime: ", tourn->params.time, "\\n");
    gMamer.XServerCom(" %18s %4d %s", "(I)ncrement: ", tourn->params.inc, "\\n");
    gMamer.XServerCom(" %18s %4d %s", "(R)ounds: ", tourn->params.rounds, "\\n");
    gMamer.XServerCom(" %18s %4d %s", "Max (P)layers: ", tourn->params.maxPlayers, "\\n");
    gMamer.XServerCom(" %18s %4c %s", "(M)ode: ", tourn->params.mode, "(r)ated or (u)nrated\\n");
    gMamer.XServerCom(" %18s %4c %s", "(S)tyle: ", tourn->params.style, "(s)wiss or (r)oundrobin\\n");
    gMamer.XServerCom(" %18s %4c %s","(V)ariant: ",tourn->params.variant, "(w)ild, (r)egular, (b)ug, or (s)uicide\\n");
    if(tourn->params.variant == 'w')
      gMamer.XServerCom(" %18s %4d %s", 
			"(W)ild Type: ", 
			tourn->params.wild, "(0), (1), (2), (3), (4), (5), (8), (9)8a, (10)fr\\n");
    gMamer.XServerCom(" %18s %4d %s", "Rating (L)ow: ", tourn->params.ratingLow, "\\n");
    gMamer.XServerCom(" %18s %4d %s", "Rating (H)igh: ", tourn->params.ratingHigh, "\\n\\n");
    gMamer.XServerCom(" %18s %-18s %s", "Manager: ", tourn->manager, "\\n\n");
  } else {
    gMamer.TellUser(NotFound, user->name, "tourney");
  }

  return 1;
}//- End ListTourneyVars

//- MessageManagers -----------------------------------------------------
int CommandEntry::MessageManagers(User *user, param_list params) {
    int i, level, tourneys;
    long last;
    char filename[256], manager[NAMELEN];
    FILE *theFile;
    
    i = 1;
    level = params[0].type;
    sprintf(filename, "%s/managers", gMamer.dataFilePath);
    if((theFile = fopen(filename, "r"))) {
      while(fscanf(theFile, "%s %d %d %ld", manager, &level, &tourneys, &last) > 0) {
	gMamer.XServerCom("%s %s %s %s", "message", manager, params[0].val.string, "\n");
      }
      fclose(theFile);
      
      return(1);
    }
    gMamer.TellUser(NotFound, user->name, "Manager List");
    return(0);
}//- MessageManagers

//- SetResult ------------------------------------------------------------
int CommandEntry::SetResult(User *user, param_list params) {
  Tourney *t;
  int result, return_val = 0;
  char answer[128];

  t = gMamer.FindTourney(params[0].val.integer);
  
  if(NULL != t) {
    switch (params[3].type) {    // try and set the result
    case TYPE_INT:
      switch(params[3].val.integer) {
        case 1: result = 1; break;
        case 0: result = 0; break;

      }
      break;
    case TYPE_WORD:
      if(!strcmp("=", params[3].val.word)) { result = 2;
      } else if(!strcmp("draw", params[3].val.word)) { result = 2;
      } else if(!strcmp("win", params[3].val.word)) { result = 1;
      } else if(!strcmp("white", params[3].val.word)) {	result = 1;
      } else if(!strcmp("loss", params[3].val.word)) { result = 0;
      } else if(!strcmp("black", params[3].val.word)) {	result = 0;
      } else { 
	gMamer.TellUser(GameResultNotFound, user->name, params[3].val.word); 
	return 0;
      }
      break;
    default:
      gMamer.TellUser(GameResultNotFound, user->name, params[3].val.string); 
      return 0;
      break;
    }
    return_val = t->SetGameResult(params[1].val.word, params[2].val.word, result);
  }
  switch (return_val) {
  case 0:
    sprintf(answer, "a game with %s as white and %s as black", params[1].val.word, params[2].val.word);
    gMamer.TellUser(NotFound, user->name, answer);
    break;
  default:
    gMamer.TellUser(GameResultSet, user->name, params[1].val.word, params[2].val.word, result);
    switch (result) {
    case 1:
      sprintf(answer, "1-0");
      break;
    case 0:
      sprintf(answer, "0-1");
      break;
    default:
      sprintf(answer, "1/2-1/2");
      break;
    }
    gMamer.XServerCom("%s %d The game %s vs. %s in tourney #%d has been set %s by %s\n", "tell", gMamer.channelNumber,
		      params[1].val.word, params[2].val.word, t->number, answer, user->name);
    break;
  }
  return return_val;
}//- End of SetResult

//- SetTourneyVariable -----------------------------------------------------
int CommandEntry::SetTourneyVariable(User *user, param_list params) {
  Tourney *tourn = NULL;
  int i=0;
  char which_var[16];
  strings varAliases[] = {
    {"time", 0},      {"t", 0}, {"inc", 1},        {"i", 1}, {"rounds", 2}, {"r", 2},
    {"style", 3},     {"s", 3}, {"variant", 4},    {"v", 4}, {"mode", 5},   {"m", 5},
    {"wild", 6}, {"w", 6},
    {"ratingLow", 7}, {"l", 7}, {"ratingHigh", 8}, {"h", 8}, {"maxplayers", 9}, {"p", 9}, {NULL} };

  tourn = gMamer.FindTourney(params[0].val.integer);   // what tourney are we talking about
  if(NULL == tourn) {
    gMamer.TellUser(NotFound, user->name, "tourney");  // wrong tourney number
    return 0;
  }
  if(FALSE == tourn->IsNotClosed()) {
    gMamer.TellUser(TourneyClosed, user->name, params[0].val.integer);
    return 0;
  }
  if(TRUE == tourn->IsNotNew()) {
    if((0 != strncasecmp(params[1].val.word, "rounds", strlen(params[1].val.word))) &&  // even if it is open
       (0 != strncasecmp(params[1].val.word, "r", strlen(params[1].val.word))) &&       // we can still change rounds
       (0 != strncasecmp(params[1].val.word, "maxplayers", strlen(params[1].val.word))) &&   // or max players
       (0 != strncasecmp(params[1].val.word, "p", strlen(params[1].val.word))) &&   // 
       (0 != strncasecmp(params[1].val.word, "style", strlen(params[1].val.word))) &&   // or style (rr) to (ss)
       (0 != strncasecmp(params[1].val.word, "s", strlen(params[1].val.word)))) {
      gMamer.TellUser(TourneyNotNew, user->name, params[0].val.integer);
      return(0);
    }
  }
  if(strcasecmp(user->name, tourn->manager) != 0) {
    gMamer.TellUser(NoPermissions, user->name, params[0].val.integer);
    return(0);
  }
  while(varAliases[i].string != NULL) {
    if (!(strcasecmp(varAliases[i].string, params[1].val.word))) {   //lets check the vars
      if(strlen(varAliases[i].string) == 1)
	strcpy(which_var, varAliases[i-1].string);                   
      else                                                       //copy the whole word
	strcpy(which_var, varAliases[i].string);
      
      if((varAliases[i].number <= 2) || (varAliases[i].number >= 6)) 
	if(params[2].type == TYPE_INT) {                     // if var is one that should be int
	  tourn->SetVariable(varAliases[i].number, params[2].val.integer);
	  gMamer.TellUser(ChangedInfo, user->name, which_var, params[2].val.integer);
	  return 1;
	} else {
	  gMamer.TellUser(CanNotChange, user->name, which_var, params[2].val.word);
	  return 0;
	}
      else 
	if(params[2].type == TYPE_WORD) {
	  tourn->SetVariable(varAliases[i].number, params[2].val.word);
	  gMamer.TellUser(ChangedInfo, user->name, which_var, params[2].val.word);
	  return 1;
	} else {
	  gMamer.TellUser(CanNotChange, user->name, which_var, params[2].val.integer);
	  return 0;
	}
    }
    i++;
  }

  gMamer.TellUser(NotFound, user->name, params[1].val.word);  // Bad Variable  
  return 0;
}

//- ShutdownCommand ----------------------------------------------------------
int CommandEntry::Shutdown(User *user, param_list params) {
  int i;

  i = params[0].type;
  i = user->GetManagerLevel();

  gMamer.Shutdown();
  exit(0);
  
  return(1);
} //- end of Shutdown
