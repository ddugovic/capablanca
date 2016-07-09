/*
   Copyright (c) 1993 Richard V. Nash.
   Copyright (c) 2000 Dan Papasian
   Copyright (C) Andrew Tridgell 2002
   
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

typedef struct {
  char ECO[4];
  char FENpos[74]; _NULLTERM
} ECO_entry;

typedef struct {
  char NIC[6];
  char FENpos[74]; _NULLTERM
} NIC_entry;

typedef struct {
  char LONG[100]; // [HGM] used to be 80, which gave a buffer overrun on a 95-char long line in long9999.idx
  char FENpos[74]; _NULLTERM
} LONG_entry;

static ECO_entry *ECO_book[1096];
static NIC_entry *NIC_book[1096];
static LONG_entry *LONG_book[4096];

static int ECO_entries, NIC_entries, LONG_entries;

/* This coverts a FEN position to a FICS board
   it assumes FENpos contains a valid position 
   the color tag has been separated and put in color*/

void FEN_to_board(char* FENpos, struct game_state_t* gs)
{
  int f,r;
  char next;
printf("FEN='%s', var='%s'\n", FENpos, gs->variant);
  for (r=gs->ranks-1; r >=0; r--) {
    f=0;
    while (f<gs->files) {
      next = *(FENpos++);
      if (isalpha(next))
        gs->board[f++][r] = CharToPiece(next, gs->variant);
      else if (next != '/') {
        int t = (next - '0');
	  if(*FENpos >= '0' && *FENpos <= '9') // [HGM] can be double-digit
	      t = 10*t + *(FENpos++) - '0';
        do
          gs->board[f++][r] = NOPIECE;
        while (--t && f < gs->files);
      }
    }
  }
  if (*(FENpos + 1) == 'w') /* the char after the space */
    gs->onMove = WHITE;
  else
    gs->onMove = BLACK;
}

/* converts a board to a FEN pos */

static void board_to_FEN(char* FENpos, struct game_state_t* gs)
{
  int f,r,count;
  char piece;  

  for (r=gs->ranks-1; r>=0; r--) {
    count = 0;
    for (f=0;  f<gs->files; f++) {
      if ((piece = PieceToChar(gs->board[f][r])) != ' ') {
        if (count) { 
	    if(count > 9) { count -= 10; *(FENpos++) = '1'; }
          *(FENpos++) = count + '0';
          count = 0;
        }
        *(FENpos++) = piece;
      } else {
        if (f == gs->files-1) {
	    if(count > 8) { count -= 10; *(FENpos++) = '1'; }
          *(FENpos++) = count + '0' + 1;
        } else
          count++;
      }
    }
    *(FENpos++) = '/';
  }

  *(--FENpos) = ' ';

  if (gs->onMove == WHITE)
    *(++FENpos) = 'w';
  else
    *(++FENpos) = 'b';
  *(++FENpos) = '\0';
}

char *boardToFEN(int g)
{
  static char FENstring[74];

  board_to_FEN (FENstring,&game_globals.garray[g].game_state);
  return FENstring;
}

static void ECO_init()
{
  FILE *fp;
  char tmp[1024];
  char *ptmp= tmp;
  char FENpos[74], ECO[4], onMove[2];
  int i=0;

  fp= fopen_s(BOOK_DIR "/eco999.idx", "r");
  if (!fp) {
    d_printf( "Could not open ECO file\n");
    exit(1);
  }
  while (!feof(fp)) {
    strcpy(ptmp, "");
    fgets(ptmp, 1024, fp);
    if (feof(fp)) continue;
    sscanf(ptmp, "%[\x21-z] %s", FENpos, onMove);
    sprintf(FENpos, "%s %s", FENpos, onMove);
    strcpy(ptmp, "");
    fgets(ptmp, 1024, fp);
    if (feof(fp)) continue;
    sscanf(ptmp, "%[0-z]", ECO);
    ECO_book[i]= (ECO_entry *) malloc(sizeof(ECO_entry));
    if (ECO_book[i]==NULL) {
      d_printf( "Cound not alloc mem for ECO entry %d.\n", i);
      exit(1);
    }
    strcpy(ECO_book[i]->ECO, ECO);
    strcpy(ECO_book[i]->FENpos, FENpos);
    ++i;
  }
  fclose(fp);
  ECO_book[i]=NULL;
  ECO_entries = i;
  
  while (--i >= 0)
    if (ECO_book[i] == NULL)
      d_printf( "ERROR!  ECO book position number %d is NULL.", i);
}

static void NIC_init()
{
  FILE *fp;
  char tmp[1024];
  char *ptmp= tmp;
  char FENpos[74], NIC[6], onMove[2];
  int i=0;

  fp= fopen_p("%s/nic999.idx", "r", BOOK_DIR);
  if (!fp) {
    d_printf( "Could not open NIC file\n");
    exit(1);
  }
  while (!feof(fp)) {
    strcpy(ptmp, "");
    fgets(ptmp, 1024, fp);
    if (feof(fp)) continue;
    sscanf(ptmp, "%[\x21-z] %s", FENpos, onMove);
    sprintf(FENpos, "%s %s", FENpos, onMove);
    strcpy(ptmp, "");
    fgets(ptmp, 1024, fp);
    if (feof(fp)) continue;
    sscanf(ptmp, "%[.-z]", NIC);
    NIC_book[i]= (NIC_entry *) malloc(sizeof(NIC_entry));
    if (NIC_book[i]==NULL) {
      d_printf( "Cound not alloc mem for NIC entry %d.\n", i);
      exit(1);
    }
    strcpy(NIC_book[i]->NIC, NIC);
    strcpy(NIC_book[i]->FENpos, FENpos);
    ++i;
  }
  fclose(fp);
  NIC_book[i]=NULL;
  NIC_entries = i;
}

static void LONG_init()
{
  FILE *fp;
  char tmp[1024];
  char *ptmp= tmp;
  char FENpos[74], LONG[256], onMove[2];
  int i=0;

  fp= fopen_p("%s/long999.idx", "r", BOOK_DIR);
  if (!fp) {
    d_printf( "Could not open LONG file\n");
    exit(1);
  }
  while (!feof(fp)) {
    strcpy(ptmp, "");
    fgets(ptmp, 1024, fp);
    if (feof(fp)) continue;
    sscanf(ptmp, "%[\x21-z] %s", FENpos, onMove);
    sprintf(FENpos, "%s %s", FENpos, onMove);
    strcpy(ptmp, "");
    fgets(ptmp, 1024, fp);
    if (feof(fp)) continue;
    sscanf(ptmp, "%[^*\n]", LONG);
    LONG_book[i]= (LONG_entry *) malloc(sizeof(LONG_entry));
    if (LONG_book[i]==NULL) {
      d_printf( "Cound not alloc mem for LONG entry %d.\n", i);
      exit(1);
    }
    strcpy(LONG_book[i]->LONG, LONG);
    strcpy(LONG_book[i]->FENpos, FENpos);
    ++i;
  }
  fclose(fp);
  LONG_book[i]=NULL;
  LONG_entries = i;
}

/* free up any memory used by books */
void book_close(void)
{
	int i;

	for (i=0;ECO_book[i];i++) {
		free(ECO_book[i]);
		ECO_book[i] = NULL;
	}
	for (i=0;NIC_book[i];i++) {
		free(NIC_book[i]);
		NIC_book[i] = NULL;
	}
	for (i=0;LONG_book[i];i++) {
		free(LONG_book[i]);
		LONG_book[i] = NULL;
	}
}

void book_open(void)
{
	ECO_init();
	NIC_init(); 
	LONG_init(); 
	d_printf("CHESSD: Loaded books\n");
}

char *getECO(int g)
{
  static char ECO[4];

  int i, flag, l = 0, r = ECO_entries - 1, x;


    if (game_globals.garray[g].type == TYPE_WILD) {
      strcpy(ECO, "---");
      return ECO;
    } else if (game_globals.garray[g].moveList == NULL) {
      strcpy(ECO, "***");
      return ECO;
    } else {
      strcpy(ECO, "A00");
    }

  for (flag=0,i=game_globals.garray[g].numHalfMoves; (i>0 && !flag); i--) {
    l = 0;
    r = ECO_entries - 1;
    while ((r >= l) && !flag) {
      x = (l+r)/2;
      if ((strcmp(game_globals.garray[g].moveList[i].FENpos, ECO_book[x]->FENpos)) < 0)
	r = x - 1;
      else
	l = x + 1;
      if (!strcmp(game_globals.garray[g].moveList[i].FENpos, ECO_book[x]->FENpos)) {
        strcpy(ECO, ECO_book[x]->ECO);
        flag=1;
      }
    }
  }

  return ECO;
}

int com_eco(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	int i, flag = 0, x, l, r;
	int g1, p1;
	
	
	if (param[0].type == TYPE_NULL) {  /* own game */
		if (pp->game < 0) {
			pprintf(p, "You are not playing or examining a game.\n");
			return COM_OK;
		}
		g1=pp->game;
		if (((game_globals.garray[g1].status != GAME_EXAMINE) &&
		     (game_globals.garray[g1].status != GAME_SETUP)) && !pIsPlaying(p))
			return COM_OK;
	} else {
		g1 = GameNumFromParam (p, &p1, &param[0]);
		if (g1 < 0) return COM_OK;
		if ((g1 >= game_globals.g_num) || ((game_globals.garray[g1].status != GAME_ACTIVE) &&
						   (game_globals.garray[g1].status != GAME_EXAMINE) && 
						   (game_globals.garray[g1].status != GAME_SETUP))) {
			pprintf(p, "There is no such game.\n");
			return COM_OK;
		}
	}

	if (game_globals.garray[g1].status == GAME_SETUP) {
		pprintf (p,"The postion is still being set up.\n");
		return COM_OK;
	}

	if (game_globals.garray[g1].private &&
	    (p != game_globals.garray[g1].white) && (p != game_globals.garray[g1].black) &&
	    !check_admin(p, 1)) {
		pprintf(p, "Sorry - that game is private.\n");
		return COM_OK;
	} else {
		if (game_globals.garray[g1].type == TYPE_WILD) {
			pprintf(p, "That game is a wild game.\n");
			return COM_OK;      
		}
	}
	
	pprintf(p, "Info about game %d: \"%s vs. %s\"\n\n", g1+1, 
		game_globals.garray[g1].white_name,    
		game_globals.garray[g1].black_name);
	
	if (game_globals.garray[g1].moveList==NULL) {
		return COM_OK;
	}
	
	for (flag=0,i=game_globals.garray[g1].numHalfMoves; (i>0 && !flag); i--) {
		l = 0;
		r = ECO_entries - 1;
		while ((r >= l) && !flag) {
			x = (l+r)/2;
			if ((strcmp(game_globals.garray[g1].moveList[i].FENpos, ECO_book[x]->FENpos)) < 0)
				r = x - 1;
			else
				l = x + 1;
			if (!strcmp(game_globals.garray[g1].moveList[i].FENpos, ECO_book[x]->FENpos)) {
				pprintf(p, "  ECO[%3d]: %s\n", i, ECO_book[x]->ECO);
				flag=1;
			}
		}
	}
	
	for (flag=0, i=game_globals.garray[g1].numHalfMoves; ((i>0) && (!flag)); i--) {
		l = 0;
		r = NIC_entries - 1;
		while ((r >=l) && !flag) {
			x = (l+r)/2;
			if ((strcmp(game_globals.garray[g1].moveList[i].FENpos, NIC_book[x]->FENpos)) < 0)
				r = x - 1;
			else
				l = x + 1;
			if (!strcmp(game_globals.garray[g1].moveList[i].FENpos, NIC_book[x]->FENpos)) {
				pprintf(p, "  NIC[%3d]: %s\n", i, NIC_book[x]->NIC);
				flag=1;
			}
		}
	}
	
	for (flag=0, i=game_globals.garray[g1].numHalfMoves; ((i>0) && (!flag)); i--) {
		l = 0;
		r = LONG_entries - 1;
		while ((r >=l) && !flag) {
			x = (l+r)/2;
			if ((strcmp(game_globals.garray[g1].moveList[i].FENpos, LONG_book[x]->FENpos)) < 0)
				r = x - 1;
			else
				l = x + 1;
			if (!strcmp(game_globals.garray[g1].moveList[i].FENpos, LONG_book[x]->FENpos)) {
				pprintf(p, " LONG[%3d]: %s\n", i, LONG_book[x]->LONG);
				flag=1;
			}
		}
	}
	
	return COM_OK;
}
