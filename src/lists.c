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

/* lists.c
 *  New global lists code
 *
 *
 *  Added by Shaney, 29 May 1995  :)
 *
*/

#include "includes.h"

static struct List *firstGlobalList = NULL;

static ListTable ListArray[] =
{{P_HEAD, "admin"},
 {P_GOD, "removedcom"},
 {P_ADMIN, "filter"},
 {P_ADMIN, "ban"},
 {P_ADMIN, "abuser"},
 {P_ADMIN, "muzzle"},
 {P_ADMIN, "cmuzzle"},
 {P_ADMIN, "c1muzzle"}, /* possible FICS trouble spots */
 {P_ADMIN, "c24muzzle"}, /* would prefer two param addlist - DAV */
 {P_ADMIN, "c46muzzle"}, /* is a temp solution */
 {P_ADMIN, "c49muzzle"},
 {P_ADMIN, "c50muzzle"},
 {P_ADMIN, "c51muzzle"},
 {P_PUBLIC, "fm"},
 {P_PUBLIC, "im"},
 {P_PUBLIC, "gm"},
 {P_PUBLIC, "wgm"},
 {P_PUBLIC, "blind"},
 {P_PUBLIC, "teams"},
 {P_PUBLIC, "computer"},
 {P_PUBLIC, "td"},
 {P_PERSONAL, "censor"},
 {P_PERSONAL, "gnotify"},
 {P_PERSONAL, "noplay"},
 {P_PERSONAL, "notify"},
 {P_PERSONAL, "channel"},
 {P_PERSONAL, "follow"},
 {P_PERSONAL, "remote"},
 {0, NULL}};

/* free up memory used by lists */
void lists_close(void)
{
	struct List *l;

	for (l=firstGlobalList; l; ) {
		struct List *next = l->next;
		int i;
		for (i=0;i<l->numMembers;i++) {
			FREE(l->m_member[i]);
		}
		FREE(l->m_member);
		l->numMembers = 0;
		free(l);
		l = next;
	}

	firstGlobalList = NULL;
}


/* find a list.  does not load from disk */
static struct List *list_find1(int p, enum ListWhich l)
{
	struct player *pp = &player_globals.parray[p];
	struct List *prev, *tempList, **starter;
	int personal;
	
	personal = ListArray[l].rights == P_PERSONAL;
	starter = personal ? &(pp->lists) : &firstGlobalList;

	for (prev = NULL, tempList = *starter; tempList != NULL; tempList = tempList->next) {
		if (l == tempList->which) {
			if (prev != NULL) {
				prev->next = tempList->next;
				tempList->next = *starter;
				*starter = tempList;
			}
			return tempList;
		}
		if (tempList->which >= L_LASTLIST) {
			/* THIS SHOULD NEVER HAPPEN!  But has been in personal list: bugs! */
			d_printf( "fics: ERROR!! in lists code\n");
			abort();
		}
		prev = tempList;
	}

	return NULL;
}

/* find a list.  loads from disk if not in memory. */
static struct List *list_find(int p, enum ListWhich l)
{
	struct List *lst;
	FILE *fp;
	char listmember[100];
	int count=0;

	lst = list_find1(p, l);
	if (lst) {
		return lst;
	}

	/* create the base list */
	list_add(p, l, NULL);

	if (ListArray[l].rights == P_PERSONAL) {
		list_find1(p, l);
	}

	fp = fopen_p(LISTS_DIR "/%s", "r", ListArray[l].name);
	if (!fp) {
		return NULL;
	}
	while (!feof(fp)) {
		if (fgets(listmember, sizeof(listmember), fp) != NULL) {
			if (list_add(p, l, listmember) == 0) {
				count++;
			}
		}
	}
	fclose(fp);
	
	/* we've added some, retry */
	if (count) {
		return list_find1(p, l);
	}
	
	return NULL;
}


/* add item to list */
int list_add(int p, enum ListWhich l, const char *s)
{
	struct player *pp = &player_globals.parray[p];
	struct List *gl = list_find1(p, l);

	if (!gl) {
		gl = calloc(1, sizeof(*gl));
		gl->which = l;
		if (ListArray[l].rights == P_PERSONAL) {
			gl->next = pp->lists;
			pp->lists = gl;
		} else {
			gl->next = firstGlobalList;
			firstGlobalList = gl;
		}
	}

	if (!s) return 0;

	if (ListArray[l].rights == P_PERSONAL &&
	    gl->numMembers >= config_get_int("MAX_USER_LIST_SIZE", DEFAULT_MAX_USER_LIST_SIZE)) {
		return 1;
	}

	while (isspace(*s)) s++;
	    
	gl->m_member = (char **)realloc(gl->m_member, sizeof(char *) * (gl->numMembers+1));
	gl->m_member[gl->numMembers] = strndup(s, strcspn(s, " \t\r\n"));
	gl->numMembers++;

	return 0;
}

/* remove item from list */
static int list_sub(int p, enum ListWhich l, char *s)
{
	struct List *gl = list_find(p, l);
	int i;

	if (!gl) {
		return 1;
	}
	for (i = 0; i < gl->numMembers; i++) {
		if (!strcasecmp(s, gl->m_member[i])) {
			break;
		}
	}
	if (i == gl->numMembers) {
		return 1;
	}

	FREE(gl->m_member[i]);

	for (; i < (gl->numMembers - 1); i++) {
		gl->m_member[i] = gl->m_member[i+1];
	}

	gl->numMembers--;
	gl->m_member = (char **)realloc(gl->m_member, sizeof(char *) * gl->numMembers);
	return 0;
}

/* find list by name, doesn't have to be the whole name */
struct List *list_findpartial(int p, char *which, int gonnado)
{
  struct player *pp = &player_globals.parray[p];
  struct List *gl;
  int i, foundit, slen;

  slen = strlen(which);
  for (i = 0, foundit = -1; ListArray[i].name != NULL; i++) {
    if (!strncasecmp(ListArray[i].name, which, slen)) {
      if (foundit == -1)
	foundit = i;
      else
	return NULL;		/* ambiguous */
    }
  }

  if (foundit != -1) {
    int rights = ListArray[foundit].rights;
    int youlose = 0;

    switch (rights) {		/* check rights */
    case P_HEAD:
      if (gonnado && !player_ishead(p))
	youlose = 1;
      break;
    case P_GOD:
      if ((gonnado && (pp->adminLevel < ADMIN_GOD)) ||
	  (!gonnado && (pp->adminLevel < ADMIN_ADMIN)))
	youlose = 1;
      break;
    case P_ADMIN:
      if (pp->adminLevel < ADMIN_ADMIN)
	youlose = 1;
      break;
    case P_PUBLIC:
      if (gonnado && (pp->adminLevel < ADMIN_ADMIN))
	youlose = 1;
      break;
    }
    if (youlose) {
      pprintf(p, "\"%s\" is not an appropriate list name or you have insufficient rights.\n", which);
      return NULL;
    }
    gl = list_find(p, foundit);
  } else {
    pprintf(p, "\"%s\" does not match any list name.\n", which);
    return NULL;
  }
  return gl;
}

/* see if something is in a list */
int in_list(int p, enum ListWhich which, char *member)
{
	struct List *gl;
	int i;
	int filterList = (which == L_FILTER);

	gl = list_find(p, which);
	if ((gl == NULL) || (member == NULL))
		return 0;
	for (i = 0; i < gl->numMembers; i++) {
		if (filterList) {
			if (!strncasecmp(member, gl->m_member[i], strlen(gl->m_member[i])))
				return 1;
		} else {
			if (!strcasecmp(member, gl->m_member[i]))
				return 1;
		}
	}
	return 0;
}

/* add or subtract something to/from a list */
int list_addsub(int p, char* list, char* who, int addsub)
{
  struct player *pp = &player_globals.parray[p];
  int p1, connected, loadme, personal, ch;
  char *listname, *member, junkChar;
  struct List *gl;
  char *yourthe, *addrem;

  gl = list_findpartial(p, list, addsub);
  if (!gl)
    return COM_OK;

  personal = ListArray[gl->which].rights == P_PERSONAL;
  loadme = (gl->which != L_FILTER) && (gl->which != L_REMOVEDCOM) && (gl->which != L_CHANNEL);
  listname = ListArray[gl->which].name;
  yourthe = personal ? "your" : "the";
  addrem = (addsub == 1) ? "added to" : "removed from";

  if (loadme) {
    if (!FindPlayer(p, who, &p1, &connected)) {
      if (addsub == 1)
        return COM_OK;
      member = who;		/* allow sub removed/renamed player */
      loadme = 0;
    } else
      member = player_globals.parray[p1].name;
  } else {
    member = who;
  }

  if (addsub == 1) {		/* add to list */

   if (gl->which == L_CHANNEL) {

     if (sscanf (who,"%d%c",&ch, &junkChar) == 1 && ch >= 0 && ch < 255) {
       if ((ch == 0) && (!in_list(p,L_ADMIN,pp->name))) {
         pprintf(p, "Only admins may join channel 0.\n");
         return COM_OK;
       }
     } else {
         pprintf (p,"The channel to add must be a number between 0 and %d.\n",MAX_CHANNELS - 1);
         return COM_OK;
  	}
  }	 
  if (in_list(p, gl->which, member)) {
     pprintf(p, "[%s] is already on %s %s list.\n", member, yourthe, listname);
     if (loadme && !connected)
       player_remove(p1);
     return COM_OK;
    }
    if (list_add(p, gl->which, member)) {
      pprintf(p, "Sorry, %s %s list is full.\n", yourthe, listname);
      if (loadme && !connected)
	player_remove(p1);
      return COM_OK;
    }
  } else if (addsub == 2) {	/* subtract from list */
    if (!in_list(p, gl->which, member)) {
      pprintf(p, "[%s] is not in %s %s list.\n", member, yourthe, listname);
      if (loadme && !connected)
	player_remove(p1);
      return COM_OK;
    }
    list_sub(p, gl->which, member);
  }
  pprintf(p, "[%s] %s %s %s list.\n", member, addrem, yourthe, listname);

  if (!personal) {
    FILE *fp;
    char filename[MAX_FILENAME_SIZE];

    switch (gl->which) {
    case L_MUZZLE:
    case L_CMUZZLE:
    case L_C1MUZZLE:
    case L_C24MUZZLE:
    case L_C46MUZZLE:
    case L_C49MUZZLE:
    case L_C50MUZZLE:
    case L_C51MUZZLE:
    case L_ABUSER:
    case L_BAN:
      pprintf(p, "Please leave a comment to explain why %s was %s the %s list.\n", member, addrem, listname);
      pcommand(p, "addcomment %s %s %s list.\n", member, addrem, listname);
      break;
    case L_COMPUTER:
      if (player_globals.parray[p1].b_stats.rating > 0)
	UpdateRank(TYPE_BLITZ, member, &player_globals.parray[p1].b_stats, member);
      if (player_globals.parray[p1].s_stats.rating > 0)
	UpdateRank(TYPE_STAND, member, &player_globals.parray[p1].s_stats, member);
      if (player_globals.parray[p1].w_stats.rating > 0)
	UpdateRank(TYPE_WILD, member, &player_globals.parray[p1].w_stats, member);
      break;
    case L_ADMIN:
      if (addsub == 1) {	/* adding to list */
	player_globals.parray[p1].adminLevel = 10;
	pprintf(p, "%s has been given an admin level of 10 - change with asetadmin.\n", member);
      } else {
	player_globals.parray[p1].adminLevel = 0;
      }
      break;
    case L_FILTER:
    case L_REMOVEDCOM:
    default:
      break;
    }

    if (loadme && connected)
        pprintf_prompt(p1, "You have been %s the %s list by %s.\n",
                                    addrem, listname, pp->name);

    sprintf(filename, LISTS_DIR "/%s", listname);
    fp = fopen_s(filename, "w");
    if (fp == NULL) {
      d_printf( "Couldn't save %s list.\n", listname);
    } else {
      int i;
      for (i = 0; i < gl->numMembers; i++)
	fprintf(fp, "%s\n", gl->m_member[i]);
      fclose(fp);
    }
  }
  if (loadme || (gl->which == L_ADMIN)) {
    player_save(p1);
  }
  if (loadme && !connected) {
    player_remove(p1);
  }
  return COM_OK;
}

int com_addlist(int p, param_list param)
{
	return list_addsub(p, param[0].val.word, param[1].val.word, 1);
}

int com_sublist(int p,param_list param)
{
	return list_addsub(p, param[0].val.word, param[1].val.word, 2);
}

/* toggle being in a list */
int com_togglelist(int p,param_list param)
{
	char *list = param[0].val.word;
	char *val = param[1].val.word;
	struct List *gl = list_findpartial(p, list, 0);
	if (!gl) {
		pprintf(p, "'%s' does not match any list name.\n", list);
		return COM_FAILED;
	}
	if (in_list(p, gl->which, val)) {
		return com_sublist(p, param);
	}
	return com_addlist(p, param);
}

int com_showlist(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  struct List *gl;
  int i, rights;

  char *rightnames[] = {"EDIT HEAD, READ ADMINS", "EDIT GODS, READ ADMINS", "READ/WRITE ADMINS", "PUBLIC", "PERSONAL"};

  if (param[0].type == 0) {	/* Show all lists */
    pprintf(p, "Lists:\n\n");
    for (i = 0; ListArray[i].name != NULL; i++) {
      rights = ListArray[i].rights;
      if ((rights > P_ADMIN) || (pp->adminLevel >= ADMIN_ADMIN))
	pprintf(p, "%-20s is %s\n", ListArray[i].name, rightnames[rights]);
    }
  } else {			/* find match in index */
    gl = list_findpartial(p, param[0].val.word, 0);
    if (!gl) {
      return COM_OK;
    }
    rights = ListArray[gl->which].rights;
    /* display the list */
    {
      multicol *m = multicol_start(gl->numMembers);

      pprintf(p, "-- %s list: %d %s --", ListArray[gl->which].name,
      gl->numMembers,
      ((!(strcmp(ListArray[gl->which].name,"filter"))) ? "ips" : (!(strcmp(ListArray[gl->which].name,"removedcom"))) ? "commands" : (!(strcmp(ListArray[gl->which].name,"channel"))) ? "channels" : "names"));
      for (i = 0; i < gl->numMembers; i++)
	multicol_store_sorted(m, gl->m_member[i]);
      multicol_pprint(m, p, pp->d_width, 2);
      multicol_end(m);
    }
  }
  return COM_OK;
}

int list_channels(int p,int p1)
{
  struct player *pp = &player_globals.parray[p];
  struct List *gl;
  int i, rights;

    gl = list_findpartial(p1, "channel", 0);
    if (!gl) {
      return 1;
    }
    rights = ListArray[gl->which].rights;
    /* display the list */
    if (gl->numMembers == 0)
      return 1;
    {
      multicol *m = multicol_start(gl->numMembers);

      for (i = 0; i < gl->numMembers; i++)
        multicol_store_sorted(m, gl->m_member[i]);
      multicol_pprint(m, p, pp->d_width, 1);
      multicol_end(m);
    }
  return 0;
}

/* free the memory used by a list */
void list_free(struct List * gl)
{
	int i;
	struct List *temp;

	while (gl) {
		for (i = 0; i < gl->numMembers; i++) {
			FREE(gl->m_member[i]);
		}
		FREE(gl->m_member);
		temp = gl->next;
		free(gl);
		gl = temp;
	}
}

/* check lists for validity - pure paranoia */
void lists_validate(int p)
{
	struct player *pp = &player_globals.parray[p];
	struct List *gl;

	for (gl=pp->lists; gl; gl=gl->next) {
		if (gl->numMembers && !gl->m_member) {
			gl->numMembers = 0;
		}
	}
}


int titled_player(int p,char* name)
{
	if ((in_list(p, L_FM, name)) || 
	    (in_list(p, L_IM, name)) || 
	    (in_list(p, L_GM, name)) || 
	    (in_list(p, L_WGM, name))) {
		return 1;
	}
	return 0;
}

