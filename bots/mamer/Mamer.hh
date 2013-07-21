//--------------------------------------------------------------------------
// Mamer.hh - Class header for the Mamer class
//
// Matthew E. Moses & Michael A. Long
//
// $Revision: 1.11 $
// $Date: 1998/09/10 19:58:41 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: Mamer.hh,v $
// Revision 1.11  1998/09/10 19:58:41  mlong
// lots of little bug fixes and a few new features.
//
// Revision 1.10  1998/06/18 18:41:52  mlong
// prepairing for yet another move.
//
// Revision 1.9  1998/04/29 15:24:07  mlong
// prepairing for the move to daimi
// new sorting routine.
//
// Revision 1.8  1998/04/18 18:46:31  mlong
// fixed delete bug and
// added delete tourney function
//
// Revision 1.5  1997/05/15 18:29:12  chess
//  added pending and TourneyPlayers support
// added HandleGetPlayerInfo & HandleGetGameInfo
//
// Revision 1.4  1997/04/13 03:21:32  chess
// TellUser and ParseParams added
//
// Revision 1.3  1997/03/21 15:31:49  moses
// added some defines to support the solaris sun compile
//
// Revision 1.2  1996/10/01 20:14:43  moses
// Changes to support the new command list,
// to correctly have the commands, I had to derive this class from the
// CommandEntry class
//
// Revision 1.1  1996/09/30  20:52:48  moses
// Initial revision
//
//--------------------------------------------------------------------------

#ifndef _MAMER_
#define _MAMER_

#include <fstream>
#include <iostream>

using namespace std;

extern "C" {
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/signal.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <time.h>
#include <unistd.h>
#include <stdarg.h>
}

#ifdef SUN
#define INADDR_NONE -1
#endif

#include "config.h"
#include "link.hh"
#include "linklist.hh"
#include "User.hh"
#include "Tourney.hh"
#include "Command.hh"
#include "CommandEntry.hh"
#include "Player.hh"
#include "Game.hh"
#include "Storage.hh"
#include "types.h"

extern char *optarg;
extern int optind;

#define TRUE 1
#define FALSE 0

//#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
//#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

class Mamer : public CommandEntry {
 public:

  Mamer();
  ~Mamer();
  
  int Initialize(int argc, char **argv);
  char *s_and_r(char *, char *, char *);
  int OpenServerConnection(void);
  void ListenLoop(void);
  void Shutdown(void);
  int GivePlace(int, Tourney *);
  void AnnounceTourneyEnd(Tourney *);
  void savePlayerData(Tourney *);
  
  void DumpConfiguration(void);
  
  int XServerCom(char *, ...);
  void TellUser(reasons, char *);
  void TellUser(Command *, char *);
  void TellUser(reasons, char *, char *);
  void TellUser(reasons, char *, int);
  void TellUser(reasons, char *, char *, int);
  void TellUser(reasons, char *, char *, char *);
  void TellUser(reasons, char *, char *, char *, int);

#define MAX_WORD_SIZE 1024
#define MAX_LINE_SIZE 1024

  int UserIsLoaded(char *);
  int ParseParams(Command *, char *);
  int isWhiteSpace(int);
  char *getWord(char *);
  char *eatWord(char *);
  char *eatWhite(char *);
  char *eatTailWhite(char *);
  char *nextWord(char *);
  char *stolower(char *);

  int GenerateTourneyNumber(void);

  Tourney *FindTourney(int);
  User    *FindUser(char *);
  Command *FindCommand(char *, char *);
  
  void NextRound();

private:
  void Usage(void);
  void LoadConfigurationFile(void);
  void BuildCommandList(void);
  void CheckUser(char *);
  void AdjustManagerList(int, char *);
  
  int HandleQtell(char *);
  int HandleTell(char *);
  int HandleChannel(char *);
  int HandleConnect(char *);
  int HandleDisconnect(char *);
  int HandleGame(char *);
  int HandleGameInfo(char *);
  int HandlePlayerInfo(char *);

  Player  *FindPending(char *);
  
public:
  char configFilename[MAXPATHLEN];
  
  char hostname[256];
  int  portNumber;
  int  channelNumber;
  char username[80];
  char password[80];
  
  int serverSocket;
  
  int  debugLevel;

  char logFilename[MAXPATHLEN];  
  char dataFilePath[MAXPATHLEN];
  char homeFilePath[MAXPATHLEN];
  char userFilePath[MAXPATHLEN];
  char helpFilePath[MAXPATHLEN];
  
  short loggedInFlag;
  
  long countOfTourneysSinceRestart;
  
  TourneyParameters tourneyParams;
  
  LinkList<Tourney> tourneyList;
  LinkList<Player>  pendingList;
  LinkList<User>    userList;
  LinkList<Command> commandList;
  LinkList<Storage> storageList;
};

#endif
