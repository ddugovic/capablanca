//--------------------------------------------------------------------------
// config.h - default configuration parameters if the config file doesn't
//            exist, or the value isn't in the config file.  Can be
//            overridden during the build by using the DEFINES flags.
//
// Matthew E. Moses & Michael A. Long
//
// $Revision: 1.5 $
// $Date: 1998/09/10 19:58:20 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: config.h,v $
// Revision 1.5  1998/09/10 19:58:20  mlong
// *** empty log message ***
//
// Revision 1.4  1998/06/18 18:42:09  mlong
// prepairing for yet another move.
//
// Revision 1.3  1998/04/18 19:00:07  mlong
// y
//
// Revision 1.2  1997/10/08 21:03:08  chess
// no log message
//
// Revision 1.1  1996/09/30 20:52:48  moses
// Initial revision
//--------------------------------------------------------------------------

#ifndef _CONFIG_
#define _CONFIG_

#ifndef CONFIG_FILENAME
#define CONFIG_FILENAME "./mamer.config"
#endif

#ifndef DEFAULT_PATH
#define DEFAULT_PATH "/usr/local/chessd/mamer"
#endif

#ifndef DEFAULT_HELP_FILE_PATH
#define DEFAULT_HELP_FILE_PATH "help"
#endif

#ifndef DEFAULT_DATA_PATH
#define DEFAULT_DATA_PATH "data"
#endif

#ifndef DEFAULT_LOG_FILE 
#define DEFAULT_LOG_FILE "logs/mamer"
#endif

#ifndef DEFAULT_USER_PATH
#define DEFAULT_USER_PATH "players"
#endif

#ifndef DEFAULT_HOSTNAME
#define DEFAULT_HOSTNAME "localhost"
#endif
 
#ifndef DEFAULT_PORT
#define DEFAULT_PORT 5000
#endif

#ifndef DEFAULT_CHANNEL
#define DEFAULT_CHANNEL 49
#endif

#ifndef DEFAULT_USERNAME
#define DEFAULT_USERNAME "mamer"
#endif

#ifndef DEFAULT_PASSWORD
#define DEFAULT_PASSWORD "password"
#endif

#ifndef ABUSE_INCREMENT_VALUE
#define ABUSE_INCREMENT_VALUE 1
#endif

#ifndef ABUSE_RESET_VALUE
#define ABUSE_RESET_VALUE 5
#endif

#ifndef FIRSTPLACEVALUE 
#define FIRSTPLACEVALUE 5
#endif

#ifndef MAX_ROUNDS
#define MAX_ROUNDS 10
#endif

#ifndef MAX_INCREMENT
#define MAX_INCREMENT 60
#endif

#ifndef MAX_TIME
#define MAX_TIME 9000
#endif

#ifndef DEFAULT_MAX_PLAYERS
#define DEFAULT_MAX_PLAYERS 16
#endif

#ifndef DEFAULT_TIME
#define DEFAULT_TIME 3
#endif

#ifndef DEFAULT_INCREMENT
#define DEFAULT_INCREMENT 2
#endif

#ifndef DEFAULT_ROUNDS
#define DEFAULT_ROUNDS 0
#endif

#ifndef PENALTY_PER_ROUND
#define PENALTY_PER_ROUND 10
#endif

#ifndef KEEP_TOURNEY_TIME
#define KEEP_TOURNEY_TIME 7200
#endif

#ifndef SEC_BETWEEN_CSHOUTS
#define SEC_BETWEEN_CSHOUTS 240
#endif

#ifndef MAX_CHAOS_POINTS
#define MAX_CHAOS_POINTS 100
#endif

// MINIMUM_ROUNDS should always be less than MINIMUM_PLAYERS

#ifndef MINIMUM_ROUNDS
#define MINIMUM_ROUNDS 3
#endif

#ifndef MINIMUM_PLAYERS
#define MINIMUM_PLAYERS 4
#endif

#endif



