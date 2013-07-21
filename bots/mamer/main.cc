//--------------------------------------------------------------------------
// main.cc - main source file that starts Mamer
//
// Matthew E. Moses
//
// $Revision: 1.4 $
// $Date: 1998/09/10 19:57:17 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: main.cc,v $
// Revision 1.4  1998/09/10 19:57:17  mlong
// lots of little bug fixes and a few new features
//
// Revision 1.3  1998/04/18 20:05:14  mlong
// *** empty log message ***
//
// Revision 1.2  1998/04/18 18:46:04  mlong
// fixed delete bug &
// added delete tourney function
//
// Revision 1.1  1996/09/30 20:52:48  moses
// Initial revision
//
//
//--------------------------------------------------------------------------

//static char RCSid[] = "$Id: main.cc,v 1.4 1998/09/10 19:57:17 mlong Exp $";

#include "Mamer.hh"

//- Globals ----------------------------------------------------------------
Mamer gMamer;

//- Prototypes -------------------------------------------------------------
int main(int, char **);

//- main -------------------------------------------------------------------
int main(int argc, char **argv) {

    if(gMamer.Initialize(argc, argv)) 
	if(gMamer.OpenServerConnection()) {
	    gMamer.ListenLoop();
	    gMamer.Shutdown();
	}
	else
//	    cerr << "ERROR: couldn't open server connection!" << endl;
;
    else
//	cerr << "ERROR: couldn't initialize mamer!" << endl;
;
    exit(0);
} //- End of main

//- HandleSignals ---------------------------------------------------------
void HandleSignals(int theSignal) {
    switch(theSignal) {
     case SIGTERM:
     case SIGKILL:
	gMamer.Shutdown();
	exit(0);
	break;
     case SIGUSR1:
	gMamer.Shutdown();
	exit(0);	
     case SIGUSR2:
	gMamer.DumpConfiguration();
     default:
	break;
    }
} //- End of HandleSignals
