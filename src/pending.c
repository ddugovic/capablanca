/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* 
  pending.c

  Sat Feb 10 - created - DAV

*/
#include "includes.h"

extern const char *colorstr[];

/* iterator for player p */
static struct pending* next_pending(struct pending* current)
{

 if (current == NULL) {
   d_printf("NULL pointer passed to next_pending\n");
   return NULL;
 } else return current->next;
}
 
/* deletor for pending - returns next item*/
struct pending *delete_pending(struct pending *current)
{
	struct pending *p;

	if (current == NULL) { 
		d_printf("NULL pointer passed to delete_pending\n");
		return NULL;
	}
 
	player_globals.parray[current->whofrom].number_pend_from--;
	player_globals.parray[current->whoto].number_pend_to--;

	if (current->category != NULL) {
		free(current->category);
	}
	if (current->board_type != NULL) {
		free(current->board_type);
	}
	
	if (current == seek_globals.pendlist) {
		seek_globals.pendlist = current->next;
		free(current);
		return seek_globals.pendlist;
	}

	for (p=seek_globals.pendlist; p && p->next != current; p=p->next) ;

	if (!p) {
		d_printf("current not on pending list??");
		return NULL;
	}

	p->next = current->next;
	free(current);
	return p->next;
}

/* kills the whole pending list on shutdown */
void destruct_pending(void)
{
	struct pending* current = seek_globals.pendlist;
	
	while (current != NULL)
		current = delete_pending(current); 
} 

struct pending *add_pending(int from,int to,int type)
{
	struct pending* new = (struct pending*) malloc(sizeof(struct pending));

	new->whofrom = from;
	new->whoto = to;
	new->type = type;
	new->next = seek_globals.pendlist;
	new->category = NULL;
	new->board_type = NULL;
	
	player_globals.parray[from].number_pend_from++;
	player_globals.parray[to].number_pend_to++;
	
	seek_globals.pendlist = new;

	return new;
}

static int check_current_pend(struct pending* current, int p, int p1, int type)
{
    if ((p != -1) && ((current->whofrom) != p))
      return 0;
    if (current->whoto != p1 && p1 != -1)
      return 0;
    if ((type == PEND_ALL) || (current->type == type))
      return 1;
    if ((type < 0) && (current->type != -type))
      return 1;
/* The above "if" allows a type of -PEND_SIMUL to match every request
    EXCEPT simuls, for example.  I'm doing this because Heringer does
    not want to decline simul requests when he makes a move in a sumul.
    -- hersco. */

    return 0;
}

/* from p to p1 */
struct pending *find_pend(int p, int p1, int type)
{
	struct player *pp = &player_globals.parray[p];
	/* -ve type can be used to decline all but that type */
	struct pending* current = seek_globals.pendlist;
	
	if (((p != -1) && (!pp->number_pend_from)) || 
	    ((p1 != -1) && (!player_globals.parray[p1].number_pend_to)))
		return NULL;
	
	while (current != NULL) {
		if (check_current_pend(current,p,p1, type))
			return current;
		current = next_pending(current);
	}
	return NULL;
}

/* from p to p1 */
struct pending *add_request(int p, int p1, int type)
{
	struct player *pp = &player_globals.parray[p];
	struct pending* new = NULL;

	if ((pp->number_pend_from) && 
	    (player_globals.parray[p1].number_pend_to)) {
		if ((new = find_pend(p, p1, type)) != NULL)
			return new;                  /* Already exists */
	}

	if ((new = add_pending(p,p1,type)) == NULL)
		return NULL;
	return new;
}

/* removes all requests from p to p1 */
void remove_request(int p, int p1, int type)
{
	struct pending* next;
	struct pending* current = seek_globals.pendlist;

	while (current != NULL) {
		next = next_pending(current);
		if (check_current_pend(current,p,p1,type))
			delete_pending(current);
		current = next;
	}
}

/* made into 1 functions for speed */

/* decline offers to p and withdraw offers from p */
static void do_decline_withdraw_offers(int p,int p1, int offers, int partners, int wd)
{
  struct player *pp = &player_globals.parray[p];
  struct pending* offer = seek_globals.pendlist;
  int type, p2;
  char *pName = pp->name, *p2Name;
  int decline;

  if ((wd & DO_DECLINE) && (wd & DO_WITHDRAW)) { /* cut search times */
    if (((p != -1)  && (!pp->number_pend_to)
                    && (!pp->number_pend_from))
        || ((p1 != -1) && (!player_globals.parray[p1].number_pend_to)
                       && (!player_globals.parray[p1].number_pend_from)))
      return;
  }

  if ((wd & DO_DECLINE) && (!(wd & DO_WITHDRAW))) { /* cut search times */
    if (((p != -1) && (!pp->number_pend_to)) ||
      ((p1 != -1) && (!player_globals.parray[p1].number_pend_from)))
      return;
  }

  if ((!(wd & DO_DECLINE)) && (wd & DO_WITHDRAW)) { /* cut search times */
    if (((p != -1) && (!pp->number_pend_from)) ||
      ((p1 != -1) && (!player_globals.parray[p1].number_pend_to)))
      return;
  }

  while (offer != NULL) {

    decline = 0;
    if ((wd & DO_DECLINE) && (check_current_pend(offer, p1, p, offers))) {
      p2 = offer->whofrom;
      decline = 1;
    } else if ((wd & DO_WITHDRAW) && (check_current_pend(offer,p, p1, offers))) {
      p2 = offer->whoto;
    } else {
      offer = offer->next;
      continue;
    }
    
    type = offer->type;
    p2Name = player_globals.parray[p2].name;

    switch (type) {
      case PEND_MATCH:
        if (partners) {
          if (offer->game_type != TYPE_BUGHOUSE) {
            offer = offer->next;
            continue;
          } else if (decline) {
            if (offer->whoto == p) {
              pprintf(p,
                "Either your opponent's partner or your partner has joined another match.\n");
              pprintf_prompt(p,
                "Removing the bughouse offer from %s.\n",
                p2Name);

              pprintf(p2,
                "Either your opponent's partner or your partner has joined another match.\n");
              pprintf_prompt(p2,
                "Removing the bughouse offer to %s.\n",
                pName);

            } else {
              pprintf_prompt(p,
                "Your partner declines the bughouse offer from %s.\n",
                p2Name);
              pprintf_prompt(p2,
                "%s declines the bughouse offer from your partner.\n",
                pName);
            }

          } else {
            if (offer->whofrom == p) {
              pprintf(p,
                "Either your opponent's partner or your partner has joined another match.\n");
              pprintf_prompt(p,
                "Removing the bughouse offer to %s.\n",
                p2Name);

              pprintf(p2,
                "Either your opponent's partner or your partner has joined another match.\n");
              pprintf_prompt(p2,
                "Removing the bughouse offer from %s.\n",
                pName);

            } else {
              pprintf_prompt(p,
                "Your partner withdraws the bughouse offer to %s.\n",
                p2Name);
              pprintf_prompt(p2,
                "%s withdraws the bughouse offer to your partner.\n",
                pName);
            }
          }
        } else {

          if (decline) {
            pprintf_prompt(p2, "\n%s declines the match offer.\n", pName);
            pprintf(p, "You decline the match offer from %s.\n", p2Name);
          } else {
            pprintf_prompt(p2, "\n%s withdraws the match offer.\n", pName);
            pprintf(p, "You withdraw the match offer to %s.\n", p2Name);
          }
        }
        break;
      case PEND_DRAW:
        if (decline) {
          pprintf_prompt(p2, "\n%s declines draw request.\n", pName);
          pprintf(p, "You decline the draw request from %s.\n", p2Name);
        } else {
          pprintf_prompt(p2, "\n%s withdraws draw request.\n", pName);
          pprintf(p, "You withdraw the draw request to %s.\n", p2Name);
        }
        break;

      case PEND_PAUSE:
        if (decline) {
          pprintf_prompt(p2, "\n%s declines pause request.\n", pName);
          pprintf(p, "You decline the pause request from %s.\n", p2Name);
        } else {
          pprintf_prompt(p2, "\n%s withdraws pause request.\n", pName);
          pprintf(p, "You withdraw the pause request to %s.\n", p2Name);
        }
        break;

      case PEND_UNPAUSE:
        if (decline) {
          pprintf_prompt(p2, "\n%s declines unpause request.\n", pName);
          pprintf(p, "You decline the unpause request from %s.\n", p2Name);
        } else {
          pprintf_prompt(p2, "\n%s withdraws unpause request.\n", pName);
          pprintf(p, "You withdraw the unpause request to %s.\n", p2Name);
        }
        break;

      case PEND_ABORT:
        if (decline) {
          pprintf_prompt(p2, "\n%s declines abort request.\n", pName);
          pprintf(p, "You decline the abort request from %s.\n", p2Name);
        } else {
          pprintf_prompt(p2, "\n%s withdraws abort request.\n", pName);
          pprintf(p, "You withdraw the abort request to %s.\n", p2Name);
        }
        break;

      case PEND_TAKEBACK:
        if (decline) {
          pprintf_prompt(p2, "\n%s declines the takeback request.\n", pName);
          pprintf(p, "You decline the takeback request from %s.\n", p2Name);
        } else {
          pprintf_prompt(p2, "\n%s withdraws the takeback request.\n", pName);
          pprintf(p, "You withdraw the takeback request to %s.\n", p2Name);
        }
        break;

      case PEND_ADJOURN:
        if (decline) {
          pprintf_prompt(p2, "\n%s declines the adjourn request.\n", pName);
          pprintf(p, "You decline the adjourn request from %s.\n", p2Name);
        } else {
          pprintf_prompt(p2, "\n%s withdraws the adjourn request.\n", pName);
          pprintf(p, "You withdraw the adjourn request to %s.\n", p2Name);
        }
        break;

      case PEND_SWITCH:
        if (decline) {
          pprintf_prompt(p2, "\n%s declines the switch sides request.\n", pName);
          pprintf(p, "You decline the switch sides request from %s.\n", p2Name);
        } else {
          pprintf_prompt(p2, "\n%s withdraws the switch sides request.\n", pName);
          pprintf(p, "You withdraw the switch sides request to %s.\n", p2Name);
        }
        break;

      case PEND_SIMUL:
        if (decline) {
          pprintf_prompt(p2, "\n%s declines the simul offer.\n", pName);
          pprintf(p, "You decline the simul offer from %s.\n", p2Name);
        } else { 
          pprintf_prompt(p2, "\n%s withdraws the simul offer.\n", pName);
          pprintf(p, "You withdraw the simul offer to %s.\n", p2Name);
        }
        break;

      case PEND_PARTNER:
        if (decline) {
          pprintf_prompt(p2, "\n%s declines your partnership request.\n", pName);
          pprintf(p, "You decline the partnership request from %s.\n", p2Name);
        } else {
          pprintf_prompt(p2, "\n%s withdraws partnership request.\n", pName);
          pprintf(p, "You withdraw the partnership request to %s.\n", p2Name);
        }
        break;
    }
    offer = delete_pending(offer);
  }
}

/* wd is DO_DECLINE to decline DO_WITHDRAW to withdraw  and both for mutual */
void decline_withdraw_offers(int p, int p1, int offerType,int wd)
{
  struct player *pp = &player_globals.parray[p];
  /* First get rid of bughouse offers from partner. */
  /* last param in if is a safety check - shouldn't happen */

 int partner;

  if (p1 == -1)
    partner = -1;
  else
    partner = player_globals.parray[p1].partner;

  if ((offerType == PEND_MATCH || offerType == PEND_ALL)
      && pp->partner >= 0 && player_globals.parray[pp->partner].partner == p)
    do_decline_withdraw_offers(pp->partner, partner , PEND_MATCH,1,wd);


  do_decline_withdraw_offers(p, p1, offerType,0,wd);
}

/* find nth offer from p */
static struct pending* find_nth_pendfrom(int p,int num)
{
  struct pending* current = seek_globals.pendlist;

  while (num) {
    if (check_current_pend(current,p,-1,PEND_ALL))
      num--;
    if (num > 0)
      current = next_pending(current);
  }
  return current;
}

/* find nth offer to p */
static struct pending* find_nth_pendto(int p,int num)
{
  struct pending* current = seek_globals.pendlist;

  while (num) {
    if (check_current_pend(current,-1,p,PEND_ALL))
      num--;
    if (num > 0)
      current = next_pending(current);
  }
  return current;
}

static int WordToOffer (int p, char *Word, int *type, int *p1)
{
  /* Convert draw adjourn match takeback abort pause
     simmatch switch partner or <name> to offer type. */
  *p1 = -1;
  if (!strcmp(Word, "match")) {
    *type = PEND_MATCH;
  } else if (!strcmp(Word, "draw")) {
    *type = PEND_DRAW;
  } else if (!strcmp(Word, "pause")) {
    *type = PEND_PAUSE;
  } else if (!strcmp(Word, "unpause")) {
    *type = PEND_UNPAUSE;
  } else if (!strcmp(Word, "abort")) {
    *type = PEND_ABORT;
  } else if (!strcmp(Word, "takeback")) {
    *type = PEND_TAKEBACK;
  } else if (!strcmp(Word, "adjourn")) {
    *type = PEND_ADJOURN;
  } else if (!strcmp(Word, "switch")) {
    *type = PEND_SWITCH;
  } else if (!strcmp(Word, "simul")) {
    *type = PEND_SIMUL;
  } else if (!strcmp(Word, "partner")) {
    *type = PEND_PARTNER;
  } else if (!strcmp(Word, "all")) {
  } else {
    *p1 = player_find_part_login(Word);
    if (*p1 < 0) {
      pprintf(p, "No user named \"%s\" is logged in.\n", Word);
      return 0;
    }
  }
  return 1;
}

int com_accept(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int type = -1;
  int p1;
  int number; 
  struct pending* current = NULL;

  if (!(number = pp->number_pend_to)) {
    pprintf(p, "You have no offers to accept.\n");
    return COM_OK;
  }

  if (param[0].type == TYPE_NULL) {
    if (number != 1) {
      pprintf(p, "You have more than one offer to accept.\nUse \"pending\" to see them and \"accept n\" to choose which one.\n");
      return COM_OK;
    }
    current = find_pend(-1,p,PEND_ALL);

  } else if (param[0].type == TYPE_INT) {
    if ((param[0].val.integer < 1) || (param[0].val.integer > number)) {
      pprintf(p, "Out of range. Use \"pending\" to see the list of offers.\n");
      return COM_OK;
    }

    current = find_nth_pendto(p,param[0].val.integer);

  } else if (param[0].type == TYPE_WORD) {
    if (!WordToOffer (p,param[0].val.word,&type,&p1))
      return COM_OK;
    if ((p1 < 0) && ((current = find_pend(-1, p, type)) == NULL)) {
      pprintf(p, "There are no pending %s offers.\n", param[0].val.word);
      return COM_OK;
    } else if ((p1 >= 0) && (current = find_pend(p1, p, PEND_ALL)) == NULL) {
      pprintf(p, "There are no pending offers from %s.\n", player_globals.parray[p1].name);
      return COM_OK;
    }
  }

  switch (current->type) {
    case PEND_MATCH:
      accept_match(current, p, current->whofrom);
      return (COM_OK);
      break;
    case PEND_DRAW:
      pcommand(p, "draw");
      break;
    case PEND_PAUSE:
      pcommand(p, "pause");
      break;
    case PEND_UNPAUSE:
      pcommand(p, "unpause");
      break;
    case PEND_ABORT:
      pcommand(p, "abort");
      break;
    case PEND_TAKEBACK:
      pcommand(p, "takeback %d", current->wtime);
      break;
    case PEND_SIMUL:
      pcommand(p, "simmatch %s", player_globals.parray[current->whofrom].name);
      break;
    case PEND_SWITCH:
      pcommand(p, "switch");
      break;
    case PEND_ADJOURN:
      pcommand(p, "adjourn");
      break;
    case PEND_PARTNER:
      pcommand(p, "partner %s", player_globals.parray[current->whofrom].name);
      break;
  }
  return COM_OK_NOPROMPT;
}

static int com_decline_withdraw(int p, param_list param,int wd)
{
  struct player *pp = &player_globals.parray[p];
  int type = -1;
  int p1;
  int number;
  struct pending* current = seek_globals.pendlist;

  if (wd & DO_DECLINE) {
    if (!(number = pp->number_pend_to)) {
      pprintf(p, "You have no pending offers from other players.\n");
      return COM_OK;
    }
  } else {
    if (!(number = pp->number_pend_from)) {
      pprintf(p, "You have no pending offers to other players.\n");
      return COM_OK;
    }
  }


  if (param[0].type == TYPE_NULL) {
    if (number == 1) {
      if (wd & DO_DECLINE) {
        current = find_pend(-1,p,PEND_ALL);
        p1 = current->whofrom;
      } else {
        current = find_pend(p,-1,PEND_ALL);
        p1 = current->whoto;
      }
       type = current->type;
    } else { 
      if (wd & DO_DECLINE)
        pprintf(p, "You have more than one pending offer.\nUse \"pending\" to see them and \"decline n\" to choose which one.\n");
      else 
        pprintf(p, "You have more than one pending offer.\nUse \"pending\" to see them and \"withdraw n\" to choose which one.\n");
      return COM_OK;
    }

  } else if (param[0].type == TYPE_INT) {
    if ((param[0].val.integer < 1) || (param[0].val.integer > number)) {
      pprintf(p, "Out of range. Use \"pending\" to see the list of offers.\n");
      return COM_OK;
    }
    if (wd & DO_DECLINE) {
      current = find_nth_pendto(p,param[0].val.integer);
      p1 = current->whofrom;
    } else {
      current = find_nth_pendfrom(p,param[0].val.integer);
      p1 = current->whoto;
    }
    type = current->type;

  } else if (!WordToOffer (p, param[0].val.word, &type, &p1))
    return COM_OK;

  decline_withdraw_offers(p, p1, type,wd);
  return COM_OK;
}

int com_decline(int p, param_list param)
{
 return com_decline_withdraw(p,param,DO_DECLINE);
}

int com_withdraw(int p, param_list param)
{
 return com_decline_withdraw(p,param,DO_WITHDRAW);
}

static void pend_print(int p, struct pending *pend)
{

  if (p == pend->whofrom) {
    pprintf(p, "You are offering %s ", player_globals.parray[pend->whoto].name);
  } else {
    pprintf(p, "%s is offering ", player_globals.parray[pend->whofrom].name);
  }

  switch (pend->type) {
  case PEND_MATCH:
    pprintf(p, "a challenge: %s (%s) %s%s (%s) %s.\n",
          player_globals.parray[pend->whofrom].name,
          ratstrii(GetRating(&player_globals.parray[pend->whofrom], pend->game_type),
                   pend->whofrom),
          colorstr[pend->seek_color + 1], 
          player_globals.parray[pend->whoto].name,
          ratstrii(GetRating(&player_globals.parray[pend->whoto], pend->game_type),
                   pend->whoto),
          game_str(pend->rated, pend->wtime * 60, pend->winc,
                   pend->btime * 60, pend->binc, pend->category, pend->board_type));
    break;
  case PEND_DRAW:
    pprintf(p, "a draw.\n");
    break;
  case PEND_PAUSE:
    pprintf(p, "to pause the clock.\n");
    break;
  case PEND_UNPAUSE:
    pprintf(p, "to unpause the clock.\n");
    break;
  case PEND_ABORT:
    pprintf(p, "to abort the game.\n");
    break;
  case PEND_TAKEBACK:
    pprintf(p, "to takeback the last %d half moves.\n", pend->wtime);
    break;
  case PEND_SIMUL:
    pprintf(p, "to play a simul match.\n");
    break;
  case PEND_SWITCH:
    pprintf(p, "to switch sides.\n");
    break;
  case PEND_ADJOURN:
    pprintf(p, "an adjournment.\n");
    break;
  case PEND_PARTNER:
    pprintf(p, "to be bughouse partners.\n");
    break;
  }
}

int com_pending(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  struct pending* current = seek_globals.pendlist;
  int num = 0;
  int num2 = 0; 
  int total = 0;

  if ((total = pp->number_pend_from)) {
    while ((current != NULL) && (num != total)) {
      if (check_current_pend(current,p,-1,PEND_ALL)) {
        if (!num)
          pprintf(p, "Offers TO other players:\n");
        num++;
        pprintf(p, " %d: ",num);
        pend_print(p, current); 
      }
      current = next_pending(current);
    }
    pprintf(p, "\nIf you wish to withdraw any of these offers type 'withdraw n'\nor just 'withdraw' if there is only one offer.\n\n");
  } else
    pprintf(p, "There are no offers pending TO other players.\n\n");

  current = seek_globals.pendlist;

  if ((total = pp->number_pend_to)) {
    while ((current != NULL) && (num2 != total)) {
      if (check_current_pend(current,-1,p,PEND_ALL)) {
        if (!num2)
          pprintf(p, "Offers FROM other players:\n");
        num2++;
        pprintf(p, " %d: ",num2);
        pend_print(p, current);
      }
      current = next_pending(current);
    }
    pprintf(p, "\nIf you wish to accept any of these offers type 'accept n'\nor just 'accept' if there is only one offer.\n");
  } else 
    pprintf(p, "There are no offers pending FROM other players.\n");

  return COM_OK;
}

void pend_join_match (int p, int p1)
{
  struct player *pp = &player_globals.parray[p];
  struct pending* current;
  struct pending* next = seek_globals.pendlist; 

  if ((pp->number_pend_from == 0) && (pp->number_pend_to == 0)
      && (player_globals.parray[p1].number_pend_from == 0) && (player_globals.parray[p1].number_pend_to == 0))
    return;

  while (next != NULL) {
    current = next;
    next = next_pending(current);
    if (check_current_pend(current,p,-1,-PEND_PARTNER)) {
      pprintf_prompt(current->whoto, "\n%s, who was challenging you, has joined a match with %s.\n", pp->name, player_globals.parray[p1].name); 
      pprintf(p, "Challenge to %s withdrawn.\n", player_globals.parray[current->whoto].name);
      delete_pending(current);
      continue;
    }

    if (check_current_pend(current,p1,-1,-PEND_PARTNER)) {
      pprintf_prompt(current->whoto, "\n%s, who was challenging you, has joined a match with %s.\n", player_globals.parray[p1].name, pp->name);
      pprintf(p1, "Challenge to %s withdrawn.\n", player_globals.parray[current->whoto].name);
      delete_pending(current);
      continue;
    }

    if (check_current_pend(current,-1,p,-PEND_PARTNER)) {
      pprintf_prompt(current->whofrom, "\n%s, whom you were challenging, has joined a match with %s.\n", pp->name, player_globals.parray[p1].name);
      pprintf(p, "Challenge from %s removed.\n", player_globals.parray[current->whofrom].name);
      delete_pending(current);
      continue;
    }

    if (check_current_pend(current,-1,p1,-PEND_PARTNER)) {
      pprintf_prompt(current->whofrom, "\n%s, whom you were challenging, has joined a match with %s.\n", player_globals.parray[p1].name, pp->name);
      pprintf(p1, "Challenge from %s removed.\n",
              player_globals.parray[current->whofrom].name);
      delete_pending(current);
      continue;
    }
  }
}
