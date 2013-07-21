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


#include "includes.h"

#define COMPUTER_FILE LISTS_DIR "/computer"

static char *rnames[] = {"std", "blitz", "wild", "lightning"};

typedef struct _ratings {
  int rating;
  int num;
  double rd;
  long ltime;
} ratings;

typedef struct _Entry {
  char name[MAX_LOGIN_NAME+2];
  ratings r[4];
  int computer;
} ENTRY;

static ENTRY **list;
static ENTRY **sortme;

unsigned chessd_reload_flag;

static void copy_stats(ratings *r, struct statistics *s)
{
	r->num = s->num;
	r->rating = s->rating;
	r->rd = s->sterr;
	r->ltime = s->ltime;
}

/*
  read a player file using the new generic and extensible format 
*/
static int GetPlayerInfo(char *fileName, ENTRY *e)
{
	char *s, *s2;       
	struct player player;
	unsigned version;

	s = file_load(fileName, NULL);
	if (!s) {
		d_printf("Error reading player file '%s'\n", fileName);
		return 0;
	}

	if (sscanf(s, "v %u", &version) != 1 || 
	    (version != 100 && version != 9)) {
		free(s);
		return 0;
	}

	/* skip first line */
	s2 = strchr(s, '\n');

	ZERO_STRUCT(player);

	if (!s2 || unmarshall_player(&player, s2) != 0) {
		d_printf("Error unmarshalling player data in '%s'!\n", fileName);
		free(s);
		return 0;
	}
	free(s);

	strcpy(e->name, player.name);
	copy_stats(&e->r[0], &player.s_stats);
	copy_stats(&e->r[1], &player.b_stats);
	copy_stats(&e->r[2], &player.w_stats);
	copy_stats(&e->r[3], &player.l_stats);

	player_free(&player);

	return 1;
}

static int LoadEntries(void)
{
  int listsize;
  FILE *fpPlayerList;
  char letter1;
  char *command;
  char pathInput[80];
  ENTRY e;
  int len, n = 0;

  listsize = 100;
  list = malloc(sizeof(ENTRY *)*listsize);

  for (letter1 = 'a'; letter1 <= 'z'; letter1++) {
    printf("Loading %c's.\n", letter1);
    sprintf(pathInput, "%s/%c", PLAYER_DIR, letter1);
    asprintf(&command, "ls -1 %s", pathInput);
    fpPlayerList = popen(command, "r");
    free(command);
 
    if (fpPlayerList == NULL)
      continue;
    while (1) {
      fgets(e.name, MAX_LOGIN_NAME, fpPlayerList);
      if (feof(fpPlayerList))
	break;
      len = strlen(e.name);
      e.name[len - 1] = '\0';
      if (e.name[0] != letter1)
	printf("File %c/%s:  wrong directory.\n", letter1, e.name);
      else {
	sprintf(pathInput, "%s/%c/%s", PLAYER_DIR, letter1, e.name);
	if (GetPlayerInfo(pathInput, &e)) {
	  if ((list[n] = malloc(sizeof(ENTRY))) == NULL) {
	    d_printf( "malloc() failed!\n");
	  } else {
	    memcpy(list[n], &e, sizeof(ENTRY));
	    n++;
	    if (n == listsize) {
	      listsize += 100;
	      list = realloc(list, listsize*sizeof(ENTRY *));
	    }
	  }
	}
      }
    }
    pclose(fpPlayerList);
  }
  return (n);
}

static int SetComputers(int n)
{
  FILE *fpComp;
  int i = 0;
  char *line, comp[30];

  asprintf(&line, "sort -f %s", COMPUTER_FILE);
  fpComp = popen(line, "r");
  free(line);

  if (fpComp == NULL)
    return 0;
  while (i < n) {
    fgets(comp, 29, fpComp);
    if (feof(fpComp))
      break;
    comp[strlen(comp) - 1] = '\0';

    while (i < n && strcasecmp(list[i]->name, comp) < 0)
      i++;

    if (i < n && strcasecmp(list[i]->name, comp) == 0) {
      list[i++]->computer = 1;
    }
  }
  pclose(fpComp);
  return(1);
}

static int rtype;

static int sortfunc(const void *i, const void *j)
{
  int n = (*(ENTRY **)j)->r[rtype].rating - (*(ENTRY **)i)->r[rtype].rating;
  return n ? n : strcasecmp((*(ENTRY **)i)->name, (*(ENTRY **)j)->name);
}

static void makerank(void)
{
  int sortnum, sortmesize, i, n;
  FILE *fp;
  char *fName;
  long now = time(NULL);

  printf("Loading players\n");
  n = LoadEntries();
  printf("Found %d players.\n", n);
  printf("Setting computers.\n");
  SetComputers(n);

  for (rtype = 0; rtype < 4; rtype++) {
    sortnum = 0; sortmesize = 100;
    sortme = malloc(sizeof(ENTRY *)*sortmesize);

    for (i = 0; i < n; i++) {
      if (list[i]->r[rtype].rating) {
        sortme[sortnum++] = list[i];
	if (sortnum == sortmesize) {
	  sortmesize += 100;
	  sortme = realloc(sortme, sortmesize*sizeof(ENTRY *));
	}
      }
    }
    printf("Sorting %d %s.\n", sortnum, rnames[rtype]);
    qsort(sortme, sortnum, sizeof(ENTRY *), sortfunc);
    printf("Saving to file.\n");
    asprintf(&fName, "%s/rank.%s", STATS_DIR, rnames[rtype]);
    fp = fopen_s(fName, "w");
    free(fName);

    for (i = 0; i < sortnum; i++)
      fprintf(fp, "%s %d %d %d %f\n", sortme[i]->name,
              sortme[i]->r[rtype].rating, sortme[i]->r[rtype].num,
              sortme[i]->computer, current_sterr(sortme[i]->r[rtype].rd,
                                            now - sortme[i]->r[rtype].ltime));
    fclose(fp);
    free(sortme);
  }
}

int main(int argc, char **argv)
{
  if (argc > 1) {
    printf("usage: %s.\n", argv[0]);
    exit(0);
  } else {
    makerank();
  }
  return (0);
}

