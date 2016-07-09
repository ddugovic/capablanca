//--------------------------------------------------------------------------
// linklist.hh - Class header for the template linklist class
//
// Matthew E. Moses
//
// $Revision: 1.3 $
// $Date: 1998/09/10 19:58:41 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: linklist.hh,v $
// Revision 1.3  1998/09/10 19:58:41  mlong
// lots of little bug fixes and a few new features.
//
// Revision 1.2  1998/04/15 16:56:34  mlong
// *** empty log message ***
//
// Revision 1.2  1998/03/16 19:48:17  chess
// *** empty log message ***
//
// Revision 1.1  1998/03/14 03:14:15  chess
// Initial revision
//
// Revision 1.1  1996/09/30 20:52:48  moses
// Initial revision
//
//
//--------------------------------------------------------------------------

#ifndef _linklist_
#define _linklist_

#include "link.hh"

//----------------------------------------------------------------------------
// Generic LinkList Class - derived from class LinkListBase
//

template <class T>
class LinkList : public LinkListBase {
public:
  void Insert(T *p) { LinkListBase::Insert(p); }
  void Insert(T *p, T *q) { LinkListBase::Insert(p, q); }
  void Append(T *p) { LinkListBase::Append(p); }
  void Delete(void) { LinkListBase::Delete(); }
  void Delete(T *p) { LinkListBase::Delete(p); }
  T *Head(void) { return((T *)LinkListBase::Head()); }
};

//----------------------------------------------------------------------------
// Generic LinkListIterator Class - derived from class LinkListIterBase
//

template <class T>
class LinkListIter : public LinkListIterBase {
public:
  LinkListIter(LinkList<T>& list) : LinkListIterBase(list) { }
  inline void Reset(void) { LinkListIterBase::Reset(); }
  inline T *Next(void) { return((T *) LinkListIterBase::Next()); }
};

#endif

