//--------------------------------------------------------------------------
// Command.hh - Class header for the Command class
//
// Matthew E. Moses
//
// $Revision: 1.5 $
// $Date: 1998/09/10 19:58:31 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: Command.hh,v $
// Revision 1.5  1998/09/10 19:58:31  mlong
// *** empty log message ***
//
// Revision 1.4  1997/05/15 18:29:12  chess
//  added pending and TourneyPlayers support
// added HandleGetPlayerInfo & HandleGetGameInfo
//
// Revision 1.3  1997/04/13 03:21:32  chess
// constructor changed to accomodate the params list
//
// Revision 1.2  1997/04/07 22:21:43  chess
// *** empty log message ***
//
// Revision 1.1  1996/10/01 20:14:43  moses
// Initial revision
//
//
//--------------------------------------------------------------------------

#ifndef _COMMAND_
#define _COMMAND_

extern "C" {
    #include <stdlib.h>
    #include <string.h>
}

#include "link.hh"
#include "types.h"
#include "User.hh"

class Command : public Link {
public:
  Command();
  Command(char *, char *, ranks, char *, char *, USERFP);
  Command(char *, char *, ranks, char *, char *, TOURNFP);
  ~Command();
  
  int IsCommand(char *);
  ranks GetManagerLevel();
  void SetManagerLevel(ranks);
  char *GetCommandName();
  char *GetCommandAlias();
  char *GetCommandDescription();

 private:
    
 public:
    USERFP  userFunction;
    TOURNFP tournFunction;

  char parameterList[MAXNUMPARAMS];
  param_list params;
  reasons badUsage;

 private:
    char *name;
    char *alias;
 
    ranks managerLevel;
    char *description;

};

#endif
