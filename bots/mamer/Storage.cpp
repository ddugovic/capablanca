//--------------------------------------------------------------------------
// Storage.cpp - Source file for the Storage class
//
// Matthew E. Moses && Michael A. Long
//
// $Revision: 1.1 $
// $Date: 1998/09/10 19:57:17 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: Storage.cpp,v $
// Revision 1.1  1998/09/10 19:57:17  mlong
// Initial revision
//
//
//--------------------------------------------------------------------------

// static char RCSid[] = "$Id: Storage.cpp,v 1.1 1998/09/10 19:57:17 mlong Exp $";

#include "Storage.hh"

//- Constructor -----------------------------------------------------------
Storage::Storage(char *n, int wt) {
  strcpy(name, n);
  value = wt;
}

//- Constructor -----------------------------------------------------------
Storage::Storage(char *n, float wt) {
  strcpy(name, n);
  floatValue = wt;
}

//- Constructor -----------------------------------------------------------
Storage::Storage(char *n, double wt) {
  strcpy(name, n);
  floatValue = wt;
}

//- Constructor -----------------------------------------------------------
Storage::Storage(char *n, float f, int i) {
  strcpy(name, n);
  floatValue = f;
  value = i;
}

//- Constructor -----------------------------------------------------------
Storage::Storage(char *n, float f, int i, int r) {
  strcpy(name, n);
  floatValue = f;
  value = i;
  rating = r;
}

//- DeConstructor ---------------------------------------------------------
Storage::~Storage() {
}
