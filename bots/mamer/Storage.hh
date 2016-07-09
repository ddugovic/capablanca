//--------------------------------------------------------------------------
// Storage.hh - Class header for the Storage class
//
// Matthew E. Moses & Michael A. Long
//
// $Revision: 1.1 $
// $Date: 1998/09/10 19:58:41 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: Storage.hh,v $
// Revision 1.1  1998/09/10 19:58:41  mlong
// Initial revision
//
// Revision 1.3  1998/02/12 18:44:25  mlong
// *** empty log message ***
//
// Revision 1.2  1997/11/11 16:48:06  chess
// *** empty log message ***
//
// Revision 1.2  1997/10/23 19:56:12  chess
// *** empty log message ***
//
// Revision 1.1  1997/05/16 03:22:36  chess
// Initial revision
//
//
//
//--------------------------------------------------------------------------

#ifndef _Storage_
#define _Storage_

#include <fstream>
#include <iostream>

extern "C" {
#include <sys/param.h>
#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include <time.h>
#include <unistd.h>
}

#include "config.h"
#include "User.hh"
#include "link.hh"
#include "linklist.hh"

class Storage : public Link {
public:

  Storage(char *, int);
  Storage(char *, float);
  Storage(char *, double);
  Storage(char *, float, int);
  Storage(char *, float, int, int);
  ~Storage();
private:

public:

  char name[NAMELEN];
  int  value;
  double floatValue;
  int rating;

private:

};


#endif






