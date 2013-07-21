//--------------------------------------------------------------------------
// link.hh - Class header for the link and linklistbase classes
//
// Matthew E. Moses
//
// $Revision: 1.2 $
// $Date: 1998/04/15 16:56:34 $
//
// $Author: mlong $
// $Locker:  $
//
// $Log: link.hh,v $
// Revision 1.2  1998/04/15 16:56:34  mlong
// *** empty log message ***
//
// Revision 1.2  1998/03/16 19:48:17  chess
// *** empty log message ***
//
// Revision 1.1  1998/03/14 03:14:09  chess
// Initial revision
//
// Revision 1.1  1996/09/30 20:52:48  moses
// Initial revision
//
//
//--------------------------------------------------------------------------

#ifndef _link_
#define _link_

//--------------------------------------------
// Base class for a link list element
//

class Link
{
public:
    Link *next;
    
    Link(void);
    Link(Link *p);
    ~Link(void);
};

//-----------------------------------------------
// Base class for a Link List
//

class LinkListBase {
public:
  
  friend class LinkListIterBase;
  
  LinkListBase(void);
  ~LinkListBase(void);
  
  void Insert(Link *p);
  void Insert(Link *p, Link *q);
  void Append(Link *p);
  void Delete(void);
  void Delete(Link *p);
  
  Link *Head(void);
  
private:
  Link *head;
};

//-----------------------------------
// Base class for a link list iterator
//

class LinkListIterBase
{
private:
    Link         *currentItem;
    LinkListBase *currentList;
    
public:
    LinkListIterBase(LinkListBase& list);
    void Reset(void);
    Link *Next(void);
};

#endif

