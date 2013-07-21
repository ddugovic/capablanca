//--------------------------------------------------------------------------
// types.h - Header file for types
//
// Matthew E. Moses & Michael A. Long
//
// $Revision: 1.11 $
// $Date: 1998/09/10 19:58:20 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: types.h,v $
// Revision 1.11  1998/09/10 19:58:20  mlong
// *** empty log message ***
//
// Revision 1.10  1998/06/18 18:42:09  mlong
// prepairing for yet another move.
//
// Revision 1.9  1998/06/04 19:56:02  mlong
// *** empty log message ***
//
// Revision 1.8  1998/04/18 19:00:07  mlong
// fixed delete bug and added delete tourney fuction
//
// Revision 1.7  1998/02/12 18:44:34  mlong
// *** empty log message ***
//
// Revision 1.6  1997/11/06 20:51:29  chess
// *** empty log message ***
//
// Revision 1.5  1997/10/08 21:03:08  chess
// no log message
//
// Revision 1.4  1997/05/15 18:30:27  chess
// *** empty log message ***
//
// Revision 1.3  1997/04/13 03:24:42  chess
// added several enumerated types for params stuff and for TellUser outputs
//
// Revision 1.2  1997/04/07 22:22:26  chess
// added enum ranks
// and added enum reasons for why we are calling a centralized telluser function
//
// Revision 1.1  1996/10/01 20:17:34  moses
// Initial revision
//
//
//--------------------------------------------------------------------------

#ifndef _TYPES_
#define _TYPES_

class Mamer;
class User;
class Tourney;
class CommandEntry;
class Player;
class Storage;

typedef enum {
  USER=0,
  DIRECTOR=10,
  MANAGER=25,
  VICE=50,
  PRESIDENT=100
} ranks;

typedef enum {
  BadCommand,
  MultiCommand,
  CanNotChange,
  ChangedManagerLevel,
  ChangedCommandLevel,
  ChangedInfo,
  ChangedAbuse,
  GenericTell,
  JoinedTourney,
  WillKeepTourney,
  NoPermissions,
  NotEnoughPlayers,
  NotFound,
  NotKeepTourney,
  NoPlayers,
  PlayerRemoved,
  TourneyDeleted,
  TourneyNotFound,
  TourneyNotNew,
  TourneyNotOpen,
  TourneyNotClosed,
  TourneyDone,
  TourneyClosed,
  GameResultNotFound,
  GameResultSet
} reasons;

#define COM_OK 0
#define COM_FAILED 1
#define COM_AMBIGUOUS 2
#define COM_BADPARAMETERS 3
#define COM_BADCOMMAND 4

typedef enum {
  ONLINE,
  GONE,
} locations;

typedef enum {
  TYPE_NONE=-1,
  TYPE_UNTIMED, 
  TYPE_BLITZ, 
  TYPE_STAND, 
  TYPE_NONSTANDARD,
  TYPE_WILD, 
  TYPE_LIGHT, 
  TYPE_BUGHOUSE, 
  TYPE_FR,
  TYPE_SUICIDE
} gametype;

typedef enum {
  NEW,
  OPEN,
  CLOSED,
  DONE
} status;


typedef enum {
  TYPE_NULL,
  TYPE_WORD,
  TYPE_STRING,
  TYPE_INT
} types;

typedef struct u_parameter {
  types type;
  union {
    char *word;
    char *string;
    int integer;
  } val;
} parameter;

struct string_list {
  char *string;
  int number;
};

typedef struct string_list strings;

#define MAXNUMPARAMS     10
typedef parameter param_list[MAXNUMPARAMS];
typedef int (Mamer::*USERFP)  (User *, param_list);
typedef int (Mamer::*TOURNFP) (User *, param_list, Tourney *);

#endif

