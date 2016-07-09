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


const char *wpstring[] = {" ", "P", "N", "B", "R", "A", "C", "M", "Q", "E", "B", "Q", "W", "H", "N", "D", "H", "L", 
			  "C", "S", "G", "H", "A", "F", "E", "H", "M", "S", "E", "W", "O", "G", "V", "S", "E", "A",
			  "K", "H", "E", "W", "G", "L", "C", "H"};
const char *bpstring[] = {" ", "p", "n", "b", "r", "a", "c", "m", "q", "e", "b", "q", "w", "h", "n", "d", "h", "l", 
			  "c", "s", "g", "h", "a", "f", "e", "h", "m", "s", "e", "w", "o", "g", "v", "s", "e", "a",
			  "k", "h", "e", "w", "g", "l", "c", "h"};

int pieceValues[PIECES] = {0, 1, 3, 3, 5, 8, 9, 3, 9, 1, 1, 2, 2, 2, 1, 6, 5, 2, 3, 3, 3, 1, 5, 2, 1, 7, 7, 3, 3, 3, 7, 7, 7, 8, 9, 12,
			   0, 8, 9, 8, 7, 3, 3, 1};

static const int mach_type = (1<<7) | (1<<8) | (1<<9) | (1<<10) | (1<<11);
#define IsMachineStyle(n) (((1<<(n)) & mach_type) != 0)

static char bstring[MAX_BOARD_STRING_LENGTH];

static int board_read_file(char *category, char *gname, struct game_state_t *gs);
static void wild_update(board_t b, int style);

static int style1(struct game_state_t *b, struct move_t *ml);
static int style2(struct game_state_t *b, struct move_t *ml);
static int style3(struct game_state_t *b, struct move_t *ml);
static int style4(struct game_state_t *b, struct move_t *ml);
static int style5(struct game_state_t *b, struct move_t *ml);
static int style6(struct game_state_t *b, struct move_t *ml);
static int style7(struct game_state_t *b, struct move_t *ml);
static int style8(struct game_state_t *b, struct move_t *ml);
static int style9(struct game_state_t *b, struct move_t *ml);
static int style10(struct game_state_t *b, struct move_t *ml);
static int style11(struct game_state_t *b, struct move_t *ml);
static int style12(struct game_state_t *b, struct move_t *ml);
static int style13(struct game_state_t *b, struct move_t *ml);

static int (*styleFuncs[MAX_STYLES])() = {
	style1,
	style2,
	style3,
	style4,
	style5,
	style6,
	style7,
	style8,
	style9,
	style10,
	style11,
	style12,
	style13
};


static void reset_board_vars(struct game_state_t *gs)
{
 int f,r;

  if(gs->files <= 0) gs->files = 8; // [HGM] for pristine board, set default size
  if(gs->ranks <= 0) gs->ranks = 8;
  for (f = 0; f < 2; f++) {
    for (r = 0; r < BW; r++)
      gs->ep_possible[f][r] = 0;
    for (r = PAWN; r <= PIECES-1; r++)
      gs->holding[f][r-PAWN] = 0;
  }
  gs->wkmoved = gs->wqrmoved = gs->wkrmoved = -1; // [HGM] castle: no rights
  gs->bkmoved = gs->bqrmoved = gs->bkrmoved = -1;
  gs->onMove = WHITE;
  gs->moveNum = 1;
  gs->lastIrreversable = -1;
  gs->gameNum = -1;
}

void board_clear(struct game_state_t *gs)
{
 int f,r;

 for (f = 0; f < BW; f++)
    for (r = 0; r < BH; r++)
      gs->board[f][r] = NOPIECE;
 reset_board_vars(gs);
}

void board_standard(struct game_state_t *gs)
{
 int f,r;

 for (f = 0; f < BW; f++)
    for (r = 0; r < BH; r++)
      gs->board[f][r] = NOPIECE;
 for (f = 0; f < gs->files; f++)
   gs->board[f][gs->ranks-7] = W_PAWN;
 for (f = 0; f < gs->files; f++)
   gs->board[f][6] = B_PAWN;
 gs->board[0][0] = W_ROOK;
 gs->board[1][0] = W_KNIGHT;
 gs->board[2][0] = W_BISHOP; 
 gs->board[3][0] = W_QUEEN;
 gs->board[gs->files/2][0] = W_KING;
 gs->board[gs->files-3][0] = W_BISHOP;
 gs->board[gs->files-2][0] = W_KNIGHT;
 gs->board[gs->files-1][0] = W_ROOK;
 gs->board[0][gs->ranks-1] = B_ROOK;
 gs->board[1][gs->ranks-1] = B_KNIGHT;
 gs->board[2][gs->ranks-1] = B_BISHOP;
 gs->board[3][gs->ranks-1] = B_QUEEN;
 gs->board[gs->files/2][gs->ranks-1] = B_KING;
 gs->board[gs->files-3][gs->ranks-1] = B_BISHOP;
 gs->board[gs->files-2][gs->ranks-1] = B_KNIGHT;
 gs->board[gs->files-1][gs->ranks-1] = B_ROOK;
#if 1
 if(gs->files == 10) {
  gs->board[6][0] = W_CARDINAL;
  gs->board[4][0] = W_MARSHALL;
  gs->board[6][gs->ranks-1] = B_CARDINAL;
  gs->board[4][gs->ranks-1] = B_MARSHALL;
 }
 if(gs->royalKnight) {
   gs->board[1][0] = W_MAN;
   gs->board[gs->files-2][0] = W_MAN;
   gs->board[1][gs->ranks-1] = B_MAN;
   gs->board[gs->files-2][gs->ranks-1] = B_MAN;
 }
#endif

 reset_board_vars(gs);
 // [HGM] castle: standard setup has rights for corner Rooks and central King
 gs->wkmoved = gs->files/2;
 gs->bkmoved = gs->files/2;
 gs->wkrmoved = gs->files-1;
 gs->bkrmoved = gs->files-1;
 gs->wqrmoved = 0;
 gs->bqrmoved = 0;
}

int board_init(int g,struct game_state_t *b, char *category, char *board)
{
  int retval = 0;
  int wval, i, j;

  b->files = b->ranks = 8;
  b->pawnDblStep = (!category || strcmp(category, "shatranj")); 
  b->royalKnight = (category && !strcmp(category, "knightmate"));
  b->capablancaPieces = 0;
  b->holdings = 0;
  b->drops = 0;
  b->castlingStyle = 1;
  b->palace = 0;
  b->setup = 0;
  b->bareKingLoses = 0;
  b->stalemate = 1;
  b->promoType = 1;
  b->promoZone = 1;
  b->variant[0] = 0; // [HGM] variant: default is normal, if variant name is missing
  if (!category || !board || !category[0] || !board[0]) 
  				/* accounts for bughouse too */
    board_standard(b);
  else {
    if(category && category[0]) strcpy(b->variant, category); // [HGM] variant: remember category name
    if (!strcmp(category, "wild") && sscanf(board, "%d", &wval) == 1) {
	if(wval >= 1 && wval <= 4)
            wild_update(b->board, wval);
	sprintf(b->variant, "wild/%d", wval);
    }

    if (!strcmp(category, "knightmate")) {
      board_standard(b);
    } else if (!strcmp(category, "super")) {
      board_standard(b);
      b->holdings = 1;
      b->promoType = 2;
      for(i=CENTAUR; i<=AMAZON; i++) {
	int placed = 0;
	do { int p, newp;
	  j = random() % 8;
	  if((p = piecetype(b->board[j][0])) >= CENTAUR) continue; // includes King
	  b->holding[1][p-PAWN] = ++b->holding[0][p-PAWN]; // piece to holding
	  if(board && !strcmp(board, "1")) newp = i - CENTAUR + WOODY; else newp = i;
	  if(board && !strcmp(board, "2")) newp = WOODY + random()%7;
	  b->board[j][0] = newp | WHITE; // place replacements
	  b->board[j][7] = newp | BLACK;
	  placed = 1;
	} while(!placed);
      }
      b->setup = 1;
    } else if (!strcmp(category, "fischerandom")) {
      wild_update(b->board, 22);
      b->castlingStyle = 2;
      b->setup = 1; // [HGM] FRC: even the default is a setup position, for which an initial board has to be printed
    } else if (!strcmp(category, "caparandom")) {
      b->files = 10;
      wild_update(b->board, 46);
      b->castlingStyle = 2;
      b->setup = 1; 
    } else retval = board_read_file(category, board, b); 
  }
  if(b->setup && game_globals.garray[g].FENstartPos[0])  // [HGM] use pre-existing start position, if one available
    FEN_to_board(game_globals.garray[g].FENstartPos, b); //       (could be wild board, or shuffle variant)
  if(b->castlingStyle == 1) {
    b->wkmoved = b->files/2;
    b->bkmoved = b->files/2;
    b->wkrmoved = b->files-1;
    b->bkrmoved = b->files-1;
    b->wqrmoved = 0;
    b->bqrmoved = 0;
  } else if(b->castlingStyle == 2) {
    for(i=j=0; i < b->files; i++) {
      int p = b->board[i][0];
      if(p == W_ROOK || p == W_KING) {
	switch(j++) {
	  case 0: b->wqrmoved = b->bqrmoved = i; break;
	  case 1: b->wkmoved  = b->bkmoved  = i; break;
	  case 2: b->wkrmoved = b->bkrmoved = i; break;
	}
      }
    }
  }

  MakeFENpos(g, game_globals.garray[g].FENstartPos);

  return retval;
}

void board_calc_strength(struct game_state_t *b, int *ws, int *bs)
{
  int r, f;
  int *p;

  *ws = *bs = 0;
  for (f = 0; f < b->ranks; f++) {
    for (r = 0; r < b->files; r++) {
      if (colorval(b->board[r][f]) == WHITE)
	p = ws;
      else
	p = bs;
      *p += pieceValues[piecetype(b->board[r][f])];
    }
  }
  for (r = PAWN; r < PIECES; r++) {
    *ws += b->holding[0][r-1] * pieceValues[r];
    *bs += b->holding[1][r-1] * pieceValues[r];
  }
}

static char *holding_str(int *holding)
{
	static char tmp[80];
	int p,i,j;

	i = 0;
	for (p = PAWN; p < PIECES; p++) {
		for (j = 0; j < holding[p-1]; j++) {
			tmp[i++] = wpstring[p][0];
		}
	}
	tmp[i] = '\0';
	return tmp;
}

static char *append_holding_machine(char *buf, int g, int c, int p)
{
  struct game_state_t *gs = &game_globals.garray[g].game_state;
  char tmp[160];

  sprintf(tmp, "<b1> game %d white [%s] black [", g+1, holding_str(gs->holding[0]));
  strcat(tmp, holding_str(gs->holding[1]));
  strcat(buf, tmp);
  if (p) {
    sprintf(tmp, "] <- %c%s\n", "WB"[c], wpstring[p]);
    strcat(buf, tmp);
  } else
    strcat(buf, "]\n");
  return buf;
}

static char *append_holding_display(char *buf, struct game_state_t *gs, int white)
{
  if (white)
    strcat(buf, "White holding: [");
  else
    strcat(buf, "Black holding: [");
  strcat(buf, holding_str(gs->holding[white ? 0 : 1]));
  strcat(buf, "]\n");
  return buf;
}

void update_holding(int g, int pieceCaptured)
{
  int p = piecetype(pieceCaptured);
  int c = colorval(pieceCaptured);
  struct game_state_t *gs = &game_globals.garray[g].game_state;
  int pp, pl;
  char tmp1[160], tmp2[160];

  if (c == WHITE) {
    c = 0;
    pp = game_globals.garray[g].white;
  } else {
    c = 1;
    pp = game_globals.garray[g].black;
  }
  gs->holding[c][p-1]++;
  tmp1[0] = '\0';
  append_holding_machine(tmp1, g, c, p);
  sprintf(tmp2, "Game %d %s received: %s -> [%s]\n", g+1,
          player_globals.parray[pp].name, wpstring[p], holding_str(gs->holding[c]));
  for (pl = 0; pl < player_globals.p_num; pl++) {
    if (player_globals.parray[pl].status == PLAYER_EMPTY)
      continue;
    if (player_is_observe(pl, g) || (player_globals.parray[pl].game == g)) {
      pprintf_prompt(pl, IsMachineStyle(player_globals.parray[pl].style) ? tmp1 : tmp2);
	}
  }
}


/* Globals used for each board */
static int wTime, bTime;
static int orient;
static int forPlayer;
static int myTurn;		/* 1 = my turn, 0 = observe, -1 = other turn */
 /* 2 = examiner, -2 = observing examiner */
 /* -3 = just send position (spos/refresh) */

char *board_to_string(char *wn, char *bn,
		      int wt, int bt,
		      struct game_state_t *b, struct move_t *ml, int style,
		      int orientation, int relation,
		      int p)
{
  int bh = (b->gameNum >= 0 && game_globals.garray[b->gameNum].link >= 0
             || b->holdings || b->drops == 2); // [HGM] zh: make sure holdings are printed (also in Seirawan)
  orient = orientation;
  myTurn = relation;

  wTime = 0;
  bTime = 0;

  /* when examining we calculate times based on the time left when the 
     move happened, not current time */
  if (game_globals.garray[b->gameNum].status == GAME_EXAMINE) {
	  unsigned nhm = game_globals.garray[b->gameNum].numHalfMoves;

	  if (nhm > 0) {
		  wTime = ml[nhm - 1].wTime;
		  bTime = ml[nhm - 1].bTime;
	  } else {
		  wTime = game_globals.garray[b->gameNum].wInitTime;
		  bTime = game_globals.garray[b->gameNum].bInitTime;
	  }
  }

  /* cope with old stored games */
  if (wTime == 0) wTime = wt;
  if (bTime == 0) bTime = bt;

  forPlayer = p;
  if ((style < 0) || (style >= MAX_STYLES))
    return NULL;

  if (style != 11) {		/* game header */
    sprintf(bstring, "Game %d (%s vs. %s)\n\n",
	  b->gameNum + 1,
	  game_globals.garray[b->gameNum].white_name,
	  game_globals.garray[b->gameNum].black_name);
  } else
    bstring[0] = '\0';

  if (bh && !IsMachineStyle(style))
    append_holding_display(bstring, b, orientation==BLACK);

  if (styleFuncs[style] (b, ml))
    return NULL;

  if (bh) {
    if (IsMachineStyle(style))
      append_holding_machine(bstring, b->gameNum, 0, 0);
    else
      append_holding_display(bstring, b, orientation==WHITE);
  }
  return bstring;
}

char *move_and_time(struct move_t *m)
{
	static char tmp[20];
#if 0
	if(m->depth>0)
	     sprintf(tmp, "%-7s (%s%.2f/%d)", m->algString, /* tenth_str(m->tookTime, 0), */
					m->score>0 ? "+" : "", m->score, m->depth);
	else 
#endif
	sprintf(tmp, "%-7s (%s)", m->algString, tenth_str(m->tookTime, 0));
	return tmp;
}

/* The following take the game state and whole move list */

void Enlarge(char *a, int ss, int w)
{
  int l, i;
  char *p, *q;
  if(strlen(a) < ss) return;
  for(i=8; i<w; i++) {
    l = strlen(a);
    p = a + l; q = p + ss;
    while(q != a+l-ss) *q-- = *p--;
  }
}

static int genstyle(struct game_state_t *b, struct move_t *ml, const char *wp[], const char *bp[],
		    const char *wsqr, const char *bsqr,
		    const char *top, const char *mid, const char *start, const char *end, 
		    const char *label,const char *blabel)
{
  int f, r, count, i;
  char tmp[80], mylabel[80], *p, *q, myTop[80], myMid[80];
  int firstR, lastR, firstF, lastF, inc;
  int ws, bs, sqrSize = strlen(wp[0]);

  board_calc_strength(b, &ws, &bs);
  if (orient == WHITE) {
    firstR = b->ranks-1;
    firstF = b->files-1;
    lastR = lastF = 0;
    inc = -1;
  } else {
    firstR = firstF = 0;
    lastR = b->ranks-1;
    lastF = b->files-1;
    inc = 1;
  }
  strcpy(myTop, top);
  strcpy(myMid, mid);
  Enlarge(myTop, sqrSize, b->files);
  Enlarge(myMid, sqrSize, b->files);
  strcat(bstring, myTop);
  for (f = firstR, count = b->ranks-1; f != lastR + inc; f += inc, count--) {
    sprintf(tmp, "     %d  %s", f + (b->ranks < 10), start);
    strcat(bstring, tmp);
    for (r = lastF; r != firstF - inc; r = r - inc) {
      if (square_color(r, f) == WHITE)
	strcat(bstring, wsqr);
      else
	strcat(bstring, bsqr);
      if (piecetype(b->board[r][f]) == NOPIECE) {
	if (square_color(r, f) == WHITE)
	  strcat(bstring, bp[0]);
	else
	  strcat(bstring, wp[0]);
      } else {
	int piece = piecetype(b->board[r][f]);
//	if(piece > QUEEN) piece = ELEPHANT + (piece == KING); // All fairies become elephants in ascii styles
	if (colorval(b->board[r][f]) == WHITE)
	  strcat(bstring, wp[piece]);
	else
	  strcat(bstring, bp[piece]);
      }
    }
    sprintf(tmp, "%s", end);
    strcat(bstring, tmp);
    switch (count) {
    case 7:
      sprintf(tmp, "     Move # : %d (%s)", b->moveNum, CString(b->onMove));
      strcat(bstring, tmp);
      break;
    case 6:
/*    if ((b->moveNum > 1) || (b->onMove == BLACK)) {  */
/* The change from the above line to the one below is a kludge by hersco. */
      if (game_globals.garray[b->gameNum].numHalfMoves > 0) {
/* loon: think this fixes the crashing ascii board on takeback bug */
	sprintf(tmp, "     %s Moves : '%s'", CString(CToggle(b->onMove)),
		move_and_time(&ml[game_globals.garray[b->gameNum].numHalfMoves - 1]));
	strcat(bstring, tmp);
      }
      break;
    case 5:
      break;
    case 4:
      sprintf(tmp, "     Black Clock : %s", tenth_str(((bTime > 0) ? bTime : 0), 1));
      strcat(bstring, tmp);
      break;
    case 3:
      sprintf(tmp, "     White Clock : %s", tenth_str(((wTime > 0) ? wTime : 0), 1));
      strcat(bstring, tmp);
      break;
    case 2:
      sprintf(tmp, "     Black Strength : %d", bs);
      strcat(bstring, tmp);
      break;
    case 1:
      sprintf(tmp, "     White Strength : %d", ws);
      strcat(bstring, tmp);
      break;
    case 0:
      break;
    }
    strcat(bstring, "\n");
    if (count != 0)
      strcat(bstring, myMid);
    else
      strcat(bstring, myTop);
  }
  q = mylabel; i = 0;
  if (orient == WHITE) {
    p = label;
    while(*p) {
	switch(*p) {
	  case ' ':
	  case '\t':
	  case '\n':
		*q++ = *p++; break;
	  default:
		if(++i > b->files) { *q++ = '\n'; *q++ = 0; }
		*q++ = *p++;
	}
    }
  } else {
    p = blabel;
    while(*p) {
	switch(*p) {
	  case ' ':
	  case '\t':
	  case '\n':
		*q++ = *p++; break;
	  default:
		*q++ = *p++ + b->files - 12;
		if(++i >= b->files) { *q++ = '\n'; *q++ = 0; }
	}
    }
  }
  *q++ = 0;
  strcat(bstring, mylabel);
  return 0;
}

/* Experimental ANSI board for colour representation */
static int style13(struct game_state_t *b, struct move_t *ml)
{
  static const char *wp[] = {"   ", "\033[37m\033[1m P ", "\033[37m\033[1m N ", "\033[37m\033[1m B ", "\033[37m\033[1m R ", "\033[37m\033[1m A ", "\033[37m\033[1m C ", "\033[37m\033[1m M ", "\033[37m\033[1m Q ", "\033[37m\033[1m E ", "\033[37m\033[1m K "};
  static const char *bp[] = {"   ", "\033[21m\033[37m P ", "\033[21m\033[37m N ", "\033[21m\033[37m B ", "\033[21m\033[37m R ", "\033[21m\033[37m A ", "\033[21m\033[37m C ", "\033[21m\033[37m M ", "\033[21m\033[37m Q ", "\033[21m\033[37m E ", "\033[21m\033[37m K "};
  static const char *wsqr = "\033[40m";
  static const char *bsqr = "\033[45m";
  static const char *top = "\t+------------------------+\n";
  static const char *mid = "";
  static const char *start = "|";
  static const char *end = "\033[0m|";
  static const char *label = "\t  a  b  c  d  e  f  g  h  i  j  k  l\n";
  static const char *blabel = "\t  l  k  j  i  h  g  f  e  d  c  b  a\n";
return 0;
  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* Standard ICS */
static int style1(struct game_state_t *b, struct move_t *ml)
{
  static const char *wp[] = {"   |", " P |", " N |", " B |", " R |", " A |", " C |", " M |", " Q |", " E |", " B |", " Q |", 
			     " W |", " H |", " N |", " D |", " H |", " L |", " C |", " S |", " G |", " H |", " A |", " F |",
			     " E |", " H |", " M |", " S |", " E |", " W |", " O |", " G |", " V |", " S |", " E |", " A |",
			     " K |", " H |", " E |", " W |", " G |", " L |", " C |", " H |"};
  static const char *bp[] = {"   |", " *P|", " *N|", " *B|", " *R|", " *A|", " *C|", " *M|", " *Q|", " *E|", " *B|", " *Q|", 
			     " *W|", " *H|", " *N|", " *D|", " *H|", " *L|", " *C|", " *S|", " *G|", " *H|", " *A|", " *F|",
			     " *E|", " *H|", " *M|", " *S|", " *E|", " *W|", " *O|", " *G|", " *V|", " *S|", " *E|", " *A|",
			     " *K|", " *H|", " *E|", " *W|", " *G|", " *L|", " *C|", " *H|"};
  static char *wsqr = "";
  static char *bsqr = "";
  static char *top = "\t---------------------------------\n";
  static char *mid = "\t|---+---+---+---+---+---+---+---|\n";
  static char *start = "|";
  static char *end = "";
  static char *label = "\t  a   b   c   d   e   f   g   h   i   j   k   l\n";
  static char *blabel = "\t  l   k   j   i   h   g   f   e   d   c   b   a\n";

  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* USA-Today Sports Center-style board */
static int style2(struct game_state_t *b, struct move_t *ml)
{
  static const char *wp[] = {"-  ", "P  ", "N  ", "B  ", "R  ", "A  ", "C  ", "M  ", "Q  ", "E  ", "B  ", "Q  ",
			     "W  ", "H  ", "N  ", "D  ", "H  ", "L  ", "C  ", "S  ", "G  ", "H  ", "A  ", "F  ",
			     "E  ", "H  ", "M  ", "S  ", "E  ", "W  ", "O  ", "G  ", "V  ", "S  ", "E  ", "A  ",
			     "K  ", "H  ", "E  ", "W  ", "G  ", "L  ", "C  ", "H  "};
  static const char *bp[] = {"+  ", "p' ", "n' ", "b' ", "r' ", "a' ", "c' ", "m' ", "q' ", "e' ", "b' ", "q' ",
			     "w' ", "h' ", "n' ", "d' ", "h' ", "l' ", "c' ", "s' ", "g' ", "h' ", "a' ", "f' ",
			     "e' ", "h' ", "m' ", "s' ", "e' ", "w' ", "o' ", "g' ", "v' ", "s' ", "e' ", "a' ",
			     "k' ", "h' ", "e' ", "w' ", "g' ", "l' ", "c' ", "h' "};
  static char *wsqr = "";
  static char *bsqr = "";
  static char *top = "";
  static char *mid = "";
  static char *start = "";
  static char *end = "";
  static char *label = "\ta  b  c  d  e  f  g  h  i  j  k  l\n";
  static char *blabel = "\tl  k  j  i  h  g  f  e  d  c  b  a\n";

  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* Experimental vt-100 ANSI board for dark backgrounds */
static int style3(struct game_state_t *b, struct move_t *ml)
{
  static const char *wp[] = {"   ", " P ", " N ", " B ", " R ", " A ", " C ", " M ", " Q ", " E ", " B ", " Q ", 
			     " W ", " H ", " N ", " D ", " H ", " L ", " C ", " S ", " G ", " H ", " A ", " F ",
			     " E ", " H ", " M ", " S ", " E ", " W ", " O ", " G ", " V ", " S ", " E ", " A ",
			     " K ", " H ", " E ", " W ", " G ", " L ", " C ", " H "};
  static const char *bp[] = {"   ", " *P", " *N", " *B", " *R", " *A", " *C", " *M", " *Q", " *E", " *B", " *Q", 
			     " *W", " *H", " *N", " *D", " *H", " *L", " *C", " *S", " *G", " *H", " *A", " *F",
			     " *E", " *H", " *M", " *S", " *E", " *W", " *O", " *G", " *V", " *S", " *E", " *A",
			     " *K", " *H", " *E", " *W", " *G", " *L", " *C", " *H"};
  static char *wsqr = "\033[0m";
  static char *bsqr = "\033[7m";
  static char *top = "\t+------------------------+\n";
  static char *mid = "";
  static char *start = "|";
  static char *end = "\033[0m|";
  static char *label = "\t  a  b  c  d  e  f  g  h  i  j  k  l\n";
  static char *blabel = "\t  l  k  j  i  h  g  f  e  d  c  b  a\n";

  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* Experimental vt-100 ANSI board for light backgrounds */
static int style4(struct game_state_t *b, struct move_t *ml)
{
  static const char *wp[] = {"   ", " P ", " N ", " B ", " R ", " A ", " C ", " M ", " Q ", " E ", " B ", " Q ", 
			     " W ", " H ", " N ", " D ", " H ", " L ", " C ", " S ", " G ", " H ", " A ", " F ",
			     " E ", " H ", " M ", " S ", " E ", " W ", " O ", " G ", " V ", " S ", " E ", " A ",
			     " K ", " H ", " E ", " W ", " G ", " L ", " C ", " H "};
  static const char *bp[] = {"   ", " *P", " *N", " *B", " *R", " *A", " *C", " *M", " *Q", " *E", " *B", " *Q", 
			     " *W", " *H", " *N", " *D", " *H", " *L", " *C", " *S", " *G", " *H", " *A", " *F",
			     " *E", " *H", " *M", " *S", " *E", " *W", " *O", " *G", " *V", " *S", " *E", " *A",
			     " *K", " *H", " *E", " *W", " *G", " *L", " *C", " *H"};
  static char *wsqr = "\033[7m";
  static char *bsqr = "\033[0m";
  static char *top = "\t+------------------------+\n";
  static char *mid = "";
  static char *start = "|";
  static char *end = "\033[0m|";
  static char *label = "\t  a  b  c  d  e  f  g  h  i  j  k  l\n";
  static char *blabel = "\t  l  k  j  i  h  g  f  e  d  c  b  a\n";

  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* Style suggested by ajpierce@med.unc.edu */
static int style5(struct game_state_t *b, struct move_t *ml)
{
  static const char *wp[] = {"    ", "  o ", " :N:", " <B>", " |R|", " (A)", " [C]", " :M:", " {Q}", " !E!",
			     " <B>", " {Q}", " .W.", " :H:", " :N:", " <H>", " |D|", " |L|", 
			     " |C|", " !S!", " :G:", " :H:", " {A}", " {F}", " !E!", " (H)", " [M]", " :S:",
			     " !E!", " |W|", " *O*", " {G}", " :V:", " (S)", " [E]", " &A&",
			     " =K=", " (H)", " [E]", " (W)", " [G]", " <L>", " |C|", "  h "};
  static const char *bp[] = {"    ", "  p ", " :n:", " <b>", " |r|", " (a)", " [c]", " :m:", " {q}", " !e!",
			     " <b>", " {q}", " .w.", " :h:", " :n:", " <h>", " |d|", " |l|", 
			     " |c|", " !s!", " :g:", " :h:", " {a}", " {f}", " !e!", " (h)", " [m]", " :s:",
			     " !e!", " |w|", " *o*", " {g}", " :v:", " (s)", " [e]", " &a&",
			     " =k=", " (f)", " [e]", " (w)", " [g]", " <l>", " |c|", "  h "};
  static char *wsqr = "";
  static char *bsqr = "";
  static char *top = "        .   .   .   .   .   .   .   .   .\n";
  static char *mid = "        .   .   .   .   .   .   .   .   .\n";
  static char *start = "";
  static char *end = "";
  static char *label = "\t  a   b   c   d   e   f   g   h   i   j   k   l\n";
  static char *blabel = "\t  l   k   j   i   h   g   f   e   d   c   b   a\n";

  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* Email Board suggested by Thomas Fought (tlf@rsch.oclc.org) */
static int style6(struct game_state_t *b, struct move_t *ml)
{
  static const char *wp[] = {"    |", " wp |", " WN |", " WB |", " WR |", " WA |", " WC |", " WM |", " WQ |", 
			     " WE |", " WB |", " WQ |", " WW |", " WH |", " WN |", " WD |", " WH |", " WL |", 
			     " WC |", " WS |", " WG |", " WH |", " WA |", " WF |", " WE |", " WH |", " WM |", 
			     " WS |", " WE |", " WW |", " WO |", " WG |", " WV |", " WS |", " WE |", " WA |",
			     " WK |", " WH |", " WE |", " WW |", " WG |", " WL |", " WC |", " Wh |"};
  static const char *bp[] = {"    |", " bp |", " BN |", " BB |", " BR |", " BA |", " BC |", " BM |", " BQ |", 
			     " BE |", " BB |", " BQ |", " BW |", " BH |", " BN |", " BD |", " BH |", " BL |", 
			     " BC |", " BS |", " BG |", " BH |", " BA |", " BF |", " BE |", " BH |", " BM |", 
			     " BS |", " BE |", " BW |", " BO |", " BG |", " BV |", " BS |", " BE |", " BA |",
			     " BK |", " BH |", " BE |", " BW |", " BG |", " BL |", " BC |", " Bh |"};
  static char *wsqr = "";
  static char *bsqr = "";
  static char *top = "\t-----------------------------------------\n";

  static char *mid = "\t-----------------------------------------\n";
  static char *start = "|";
  static char *end = "";
  static char *label = "\t  A    B    C    D    E    F    G    H    I    J    K    L\n";
  static char *blabel = "\t  L    K    J    I    H    G    F    E    D    C    B    A\n";

  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* Miniature board */
static int style7(struct game_state_t *b, struct move_t *ml)
{
  static const char *wp[] = {"  ", " P", " N", " B", " R", " A", " C", " M", " Q", " E", " B", " Q", " W", " H", " N", " D", " H", " L", 
			     " C", " S", " G", " H", " A", " F", " E", " H", " M", " S", " E", " W", " O", " G", " V", " S", " E", " A",
			     " K", " H", " E", " W", " G", " L", " C", " H"};
  static const char *bp[] = {" -", " p", " n", " b", " r", " a", " c", " m", " q", " e", " b", " q", " w", " h", " n", " d", " h", " l", 
			     " c", " s", " g", " h", " a", " f", " e", " h", " m", " s", " e", " w", " o", " g", " v", " s", " e", " a",
			     " k", " h", " e", " w", " g", " l", " c", " h"};
  static char *wsqr = "";
  static char *bsqr = "";
  static char *top = "\t:::::::::::::::::::::\n";
  static char *mid = "";
  static char *start = "..";
  static char *end = " ..";
  static char *label = "\t   a b c d e f g h i j k l\n";
  static char *blabel = "\t   l k j i h g f e d c b a\n";

  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* ICS interface maker board-- raw data dump */
static int style8(struct game_state_t *b, struct move_t *ml)
{
  char tmp[160];
  int f, r;
  int ws, bs;

  board_calc_strength(b, &ws, &bs);
  sprintf(tmp, "#@#%03d%-16s%s%-16s%s", b->gameNum + 1,
	  game_globals.garray[b->gameNum].white_name,
	  (orient == WHITE) ? "*" : ":",
	  game_globals.garray[b->gameNum].black_name,
	  (orient == WHITE) ? ":" : "*");
  strcat(bstring, tmp);
  for (r = 0; r < b->ranks; r++) {
    for (f = 0; f < b->files; f++) {
      if (b->board[f][r] == NOPIECE) {
	strcat(bstring, " ");
      } else {
	if (colorval(b->board[f][r]) == WHITE)
	  strcat(bstring, wpstring[piecetype(b->board[f][r])]);
	else
	  strcat(bstring, bpstring[piecetype(b->board[f][r])]);
      }
    }
  }
  sprintf(tmp, "%03d%s%02d%02d%05d%05d%-7s(%s)@#@\n",
	  game_globals.garray[b->gameNum].numHalfMoves / 2 + 1,
	  (b->onMove == WHITE) ? "W" : "B",
	  ws,
	  bs,
	  (wTime + 5) / 10,
	  (bTime + 5) / 10,
	  game_globals.garray[b->gameNum].numHalfMoves ?
	  ml[game_globals.garray[b->gameNum].numHalfMoves - 1].algString :
	  "none",
	  game_globals.garray[b->gameNum].numHalfMoves ?
	  tenth_str(ml[game_globals.garray[b->gameNum].numHalfMoves - 1].tookTime, 0) :
	  "0:00");
  strcat(bstring, tmp);
  return 0;
}

/* last 2 moves only (previous non-verbose mode) */
static int style9(struct game_state_t *b, struct move_t *ml)
{
  int i, count;
  char tmp[160];
  int startmove;

  sprintf(tmp, "\nMove     %-23s%s\n",
	  game_globals.garray[b->gameNum].white_name,
	  game_globals.garray[b->gameNum].black_name);
  strcat(bstring, tmp);
  sprintf(tmp, "----     --------------         --------------\n");
  strcat(bstring, tmp);
  startmove = ((game_globals.garray[b->gameNum].numHalfMoves - 3) / 2) * 2;
  if (startmove < 0)
    startmove = 0;
  for (i = startmove, count = 0;
       i < game_globals.garray[b->gameNum].numHalfMoves && count < 4;
       i++, count++) {
    if (!(i & 0x01)) {
      sprintf(tmp, "  %2d     ", i / 2 + 1);
      strcat(bstring, tmp);
    }
    sprintf(tmp, "%-23s", move_and_time(&ml[i]));
    strcat(bstring, tmp);
    if (i & 0x01)
      strcat(bstring, "\n");
  }
  if (i & 0x01)
    strcat(bstring, "\n");
  return 0;
}

/* Sleator's 'new and improved' raw dump format... */
static int style10(struct game_state_t *b, struct move_t *ml)
{
  int f, r;
  char tmp[160];
  int ws, bs;

  board_calc_strength(b, &ws, &bs);
  sprintf(tmp, "<10>\n");
  strcat(bstring, tmp);
  for (r = b->ranks-1; r >= 0; r--) {
    strcat(bstring, "|");
    for (f = 0; f < b->files; f++) {
      if (b->board[f][r] == NOPIECE) {
	strcat(bstring, " ");
      } else {
	if (colorval(b->board[f][r]) == WHITE)
	  strcat(bstring, wpstring[piecetype(b->board[f][r])]);
	else
	  strcat(bstring, bpstring[piecetype(b->board[f][r])]);
      }
    }
    strcat(bstring, "|\n");
  }
  strcat(bstring, (b->onMove == WHITE) ? "W " : "B ");
  if (game_globals.garray[b->gameNum].numHalfMoves) {
    sprintf(tmp, "%d ",
	    ml[game_globals.garray[b->gameNum].numHalfMoves - 1].doublePawn);
  } else {
    sprintf(tmp, "-1 ");
  }
  strcat(bstring, tmp);
  sprintf(tmp, "%d %d %d %d %d\n",
	  (b->wkmoved >= 0 && b->wkrmoved >= 0), // [HGM] castle: inverted the logic, both must have rights
	  (b->wkmoved >= 0 && b->wqrmoved >= 0),
	  (b->bkmoved >= 0 && b->bkrmoved >= 0),
	  (b->bkmoved >= 0 && b->bqrmoved >= 0),
       (game_globals.garray[b->gameNum].numHalfMoves - ((b->lastIrreversable == -1) ? 0 :
					   b->lastIrreversable)));
  strcat(bstring, tmp);
  sprintf(tmp, "%d %s %s %d %d %d %d %d %d %d %d %s (%s) %s %d\n",
	  b->gameNum,
	  game_globals.garray[b->gameNum].white_name,
	  game_globals.garray[b->gameNum].black_name,
	  myTurn,
	  game_globals.garray[b->gameNum].wInitTime / 600,
	  game_globals.garray[b->gameNum].wIncrement / 10,
	  ws,
	  bs,
	  (wTime + 5) / 10,
	  (bTime + 5) / 10,
	  game_globals.garray[b->gameNum].numHalfMoves / 2 + 1,
	  game_globals.garray[b->gameNum].numHalfMoves ?
	  ml[game_globals.garray[b->gameNum].numHalfMoves - 1].moveString :
	  "none",
	  game_globals.garray[b->gameNum].numHalfMoves ?
	  tenth_str(ml[game_globals.garray[b->gameNum].numHalfMoves - 1].tookTime, 0) :
	  "0:00",
	  game_globals.garray[b->gameNum].numHalfMoves ?
	  ml[game_globals.garray[b->gameNum].numHalfMoves - 1].algString :
	  "none",
	  (orient == WHITE) ? 0 : 1);
  strcat(bstring, tmp);
  sprintf(tmp, ">10<\n");
  strcat(bstring, tmp);
  return 0;
}

/* Same as 8, but with verbose moves ("P/e3-e4", instead of "e4") */
static int style11(struct game_state_t *b, struct move_t *ml)
{
  char tmp[160];
  int f, r;
  int ws, bs;

  board_calc_strength(b, &ws, &bs);
  sprintf(tmp, "#@#%03d%-16s%s%-16s%s", b->gameNum,
	  game_globals.garray[b->gameNum].white_name,
	  (orient == WHITE) ? "*" : ":",
	  game_globals.garray[b->gameNum].black_name,
	  (orient == WHITE) ? ":" : "*");
  strcat(bstring, tmp);
  for (r = 0; r < b->ranks; r++) {
    for (f = 0; f < b->files; f++) {
      if (b->board[f][r] == NOPIECE) {
	strcat(bstring, " ");
      } else {
	if (colorval(b->board[f][r]) == WHITE)
	  strcat(bstring, wpstring[piecetype(b->board[f][r])]);
	else
	  strcat(bstring, bpstring[piecetype(b->board[f][r])]);
      }
    }
  }
    sprintf(tmp, "%03d%s%02d%02d%05d%05d%-7s(%s)@#@\n",
	    game_globals.garray[b->gameNum].numHalfMoves / 2 + 1,
	    (b->onMove == WHITE) ? "W" : "B",
	    ws,
	    bs,
	    (wTime + 5) / 10,
	    (bTime + 5) / 10,
	    game_globals.garray[b->gameNum].numHalfMoves ?
	    ml[game_globals.garray[b->gameNum].numHalfMoves - 1].moveString :
	    "none",
	    game_globals.garray[b->gameNum].numHalfMoves ?
	    tenth_str(ml[game_globals.garray[b->gameNum].numHalfMoves - 1].tookTime, 0) :
	    "0:00");
  strcat(bstring, tmp);
  return 0;
}


int kludgeFlag = 0; 
/* Similar to style 10.  See the "style12" help file for information */
static int style12(struct game_state_t *b, struct move_t *ml)
{
  int f, r;
  char tmp[160]; // [HGM] 80 caused problems with long login names
  int ws, bs; 
  int nhm = kludgeFlag ? 0 : game_globals.garray[b->gameNum].numHalfMoves; 
  // [HGM] setup: the number of half moves appeared in this routine an enormous number of times,
  // and had to be dug out of the game_globals, so that this routine could only be used to print
  // a board from a game, and not just any board given by an isolated game_state_t. This was very
  // inconvenient for printing initial boards in move lists of shuffle variants, so I added the
  // global kludgeFlag to signal that we want to print an initial position, and force nhm = 0.

  board_calc_strength(b, &ws, &bs);

  sprintf(bstring, "<12> ");
  for (r = b->ranks-1; r >= 0; r--) {
    for (f = 0; f < b->files; f++) {
      if (b->board[f][r] == NOPIECE) {
	strcat(bstring, "-");
      } else {
	if (colorval(b->board[f][r]) == WHITE)
	  strcat(bstring, wpstring[piecetype(b->board[f][r])]);
	else
	  strcat(bstring, bpstring[piecetype(b->board[f][r])]);
      }
    }
    strcat(bstring, " ");
  }

  strcat(bstring, (b->onMove == WHITE) ? "W " : "B ");
  if (nhm) {
    sprintf(tmp, "%d ",
	    ml[nhm - 1].doublePawn);
  } else {
    sprintf(tmp, "-1 ");
  }
  strcat(bstring, tmp);
  sprintf(tmp, "%d %d %d %d %d ",
	  (b->wkmoved >= 0 && b->wkrmoved >= 0), // [HGM] castle: inverted the logic, both must have rights
	  (b->wkmoved >= 0 && b->wqrmoved >= 0),
	  (b->bkmoved >= 0 && b->bkrmoved >= 0),
	  (b->bkmoved >= 0 && b->bqrmoved >= 0),
	  (nhm - ((b->lastIrreversable == -1) ? 0 : b->lastIrreversable)));
  strcat(bstring, tmp);
  sprintf(tmp, "%d %s %s %d %d %d %d %d %d %d %d %s (%s) %s %d %d\n",
	  b->gameNum + 1,
	  game_globals.garray[b->gameNum].white_name,
	  game_globals.garray[b->gameNum].black_name,
	  myTurn,
	  game_globals.garray[b->gameNum].wInitTime / 600,
	  game_globals.garray[b->gameNum].wIncrement / 10,
	  ws,
	  bs,
	  (wTime / 10),
	  (bTime / 10),
	  nhm / 2 + 1,
	  nhm ?
	  ml[nhm - 1].moveString :
	  "none",
	  nhm ?
	  tenth_str(ml[nhm - 1].tookTime, 0) :
	  "0:00",
	  nhm ?
	  ml[nhm - 1].algString :
	  "none", (orient == WHITE) ? 0 : 1,
	  b->moveNum > 1 ? 1 : 0); /* ticking */

  strcat(bstring, tmp);
  return 0;
}

static int board_read_file(char *category, char *gname, struct game_state_t *gs)
{
  FILE *fp;
  int c;
  int onNewLine = 1;
  int onColor = -1;
  int onPiece = -1;
  int onFile = -1;
  int onRank = -1;

  fp = fopen_p("%s/%s/%s", "r", BOARD_DIR, category, gname);
  if (!fp)
    return 1;

  board_clear(gs);
  gs->setup = 1;
  if (gname && !strcmp(gname, "0"))
	gs->setup = 0; // [HGM] variant: any board in the default file "0" is supposed to be implied by the variant

  while (!feof(fp)) {
    c = fgetc(fp);
    if (onNewLine) {
      if (c == 'W') {
	onColor = WHITE;
	if (gs->onMove < 0)
	  gs->onMove = WHITE;
      } else if (c == 'B') {
	onColor = BLACK;
	if (gs->onMove < 0)
	  gs->onMove = BLACK;
      } else if (c == 'S') { int f=8, r=8;
	// [HGM] rules: read rule modifiers
	fscanf(fp, "%dx%d", &f, &r); gs->files=f; gs->ranks = r;
	while (!feof(fp) && c != '\n') {
	  c = fgetc(fp);
	  switch(c) {
	    case 'r':
		gs->royalKnight = 1;
		break;
	    case 'c':
		gs->capablancaPieces = 1;
		break;
	    case 'd':
		gs->drops = 1;
		break;
	    case 'g':
		gs->drops = 2;
		break;
	    case 'h':
		gs->holdings = -1;     // color-flip holdings
		break;
	    case 'p':
		gs->palace = 3;
		break;
	    case 'P':
		gs->promoType = 2; // only promote to captured pieces
		gs->holdings = 1;  // use holdings to hold own captured pieces
		break;
	    case 'S':
		gs->promoType = 3; // Shogi-type promotions
		break;
	    case 'Z':
		gs->promoZone = 3; // for Grand Chess
		gs->pawnDblStep = 2;
		break;
	    case 'F':
		gs->castlingStyle = 2; // FRC castling
		break;
	    case 'w':
		gs->castlingStyle = 1; // wild castling, from both center files
		break;
	    case 'n':
		gs->castlingStyle = 0; // no castling
		break;
	    case 'f':
		gs->castlingStyle = 3; // free castling
		break;
	    case 'D':
		gs->pawnDblStep = 0; // suppress pawn double step
		break;
	    case 'b':
		gs->bareKingLoses = 1; // apply baring rule
		break;
	    case 's':
		gs->stalemate = 0; // stalemate loses
		break;
	  }
	}	
	continue;
      } else if (c == '#') {
	while (!feof(fp) && c != '\n')
	  c = fgetc(fp);	/* Comment line */
	continue;
      } else {			/* Skip any line we don't understand */
	while (!feof(fp) && c != '\n')
	  c = fgetc(fp);
	continue;
      }
      onNewLine = 0;
    } else {
      switch (c) {
      case 'A':
	onPiece = CARDINAL;
	break;
      case 'B':
	onPiece = BISHOP;
	break;
      case 'C':
	onPiece = MARSHALL;
	break;
      case 'D':
	onPiece = SELEPHANT;
	break;
      case 'E':
	onPiece = ALFIL;
	break;
      case 'F':
	onPiece = FERZ2;
	break;
      case 'G':
	onPiece = GOLD;
	break;
      case 'H':
	onPiece = HORSE;
	break;
      case 'I':
	onPiece = DRAGONHORSE;
	break;
      case 'J':
	onPiece = DRAGONKING;
	break;
      case 'K':
	onPiece = KING;
	break;
      case 'L':
	onPiece = LANCE;
	break;
      case 'M':
	onPiece = MAN;
	break;
      case 'N':
	onPiece = KNIGHT;
	break;
      case 'O':
	onPiece = CANNON;
	break;
      case 'P':
	onPiece = PAWN;
	break;
      case 'Q':
	onPiece = QUEEN;
	break;
      case 'R':
	onPiece = ROOK;
	break;
      case 'S':
	onPiece = SILVER;
	break;
      case 'T':
	onPiece = ELEPHANT;
	break;
      case 'U':
	onPiece = HAWK;
	break;
      case 'V':
	onPiece = CENTAUR;
	break;
      case 'W':
	onPiece = WAZIR;
	break;
      case 'X':
	onPiece = WARLORD;
	break;
      case 'Y':
	onPiece = GENERAL;
	break;
      case 'Z':
	onPiece = AMAZON;
	break;
      case 'm':
	onPiece = MANDARIN;
	break;
      case 'n':
	onPiece = HONORABLEHORSE;
	break;
      case 'o':
	onPiece = MODERNELEPHANT;
	break;
      case 'p':
	onPiece = PRIESTESS;
	break;
      case 'q':
	onPiece = FERZ;
	break;
      case 'r':
	onPiece = MINISTER;
	break;
      case 's':
	onPiece = MASTODON;
	break;
      case 't':
	onPiece = ALFIL2;
	break;
      case 'u':
	onPiece = NIGHTRIDER;
	break;
      case 'v':
	onPiece = HOPLITE;
	break;
      case 'w':
	onPiece = WOODY;
	break;
      case 'x':
	onPiece = LIEUTENANT;
	break;
      case 'y':
	onPiece = CAPTAIN;
	break;
      case 'z':
	onPiece = MAN2;
	break;
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
      case 'g':
      case 'h':
      case 'i':
      case 'j':
      case 'k':
      case 'l':
	onFile = c - 'a';
        if(onFile >= gs->files) { onFile = -1; break; }
	onRank = -1;
	break;
      case '@':
	if (onColor >= 0 && onPiece >= 0) // allow placement in holdings
	  gs->holding[onColor == BLACK][onPiece-1]++;
	break;
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '0':
	onRank = c - '1' + (gs->ranks > 9);
        if(onRank < 0 || onRank >= gs->ranks) { onRank = -1; break; }
	if (onFile >= 0 && onColor >= 0 && onPiece >= 0)
	  gs->board[onFile][onRank] = onPiece | onColor;
	break;
      case '#':
	while (!feof(fp) && c != '\n')
	  c = fgetc(fp);	/* Comment line */
      case '\n':
	onNewLine = 1;
	onColor = -1;
	onPiece = -1;
	onFile = -1;
	onRank = -1;
	break;
      default:
	break;
      }
    }
  }
  fclose(fp);
  return 0;
}

#define WHITE_SQUARE 1
#define BLACK_SQUARE 0
#define ANY_SQUARE -1
#define SquareColor(f, r) ((f ^ r) & 1)

static void place_piece(board_t b, int piece, int squareColor, int width)
{ //[HGM] board: make width a variable
  int r, f;
  int placed = 0;

  if (iscolor(piece, BLACK))
    r = 7;
  else
    r = 0;

  while (!placed) {
    if (squareColor == ANY_SQUARE) {
      f = random() % width;
    } else {
      f = (random() % ((width+1)/2)) * 2; // to not overflow odd-width boards
      if (SquareColor(f, r) != squareColor)
	f++;
    }
    if ((b)[f][r] == NOPIECE) {
      (b)[f][r] = piece;
      placed = 1;
    }
  }
}

static void wild_update(board_t b, int style)
{
  int f, r, i, j;

  for (f = 0; f < BW; f++) // [HGM] board: make sure also works with wider boards
    for (r = 0; r < 8; r++)
      b[f][r] = NOPIECE;
  for (f = 0; f < 8; f++) {
    b[f][1] = W_PAWN;
    b[f][6] = B_PAWN;
  }
  switch (style) {
  case 1:
    if (random() & 0x01) {
      b[4][0] = W_KING;
      b[3][0] = W_QUEEN;
    } else {
      b[3][0] = W_KING;
      b[4][0] = W_QUEEN;
    }
    if (random() & 0x01) {
      b[4][7] = B_KING;
      b[3][7] = B_QUEEN;
    } else {
      b[3][7] = B_KING;
      b[4][7] = B_QUEEN;
    }
    b[0][0] = b[7][0] = W_ROOK;
    b[0][7] = b[7][7] = B_ROOK;
    /* Must do bishops before knights to be sure opposite colored squares are
       available. */
    place_piece(b, W_BISHOP, WHITE_SQUARE, 8);
    place_piece(b, W_BISHOP, BLACK_SQUARE, 8);
    place_piece(b, W_KNIGHT, ANY_SQUARE, 8);
    place_piece(b, W_KNIGHT, ANY_SQUARE, 8);
    place_piece(b, B_BISHOP, WHITE_SQUARE, 8);
    place_piece(b, B_BISHOP, BLACK_SQUARE, 8);
    place_piece(b, B_KNIGHT, ANY_SQUARE, 8);
    place_piece(b, B_KNIGHT, ANY_SQUARE, 8);
    break;
  case 2:
    place_piece(b, W_KING, ANY_SQUARE, 8);
    place_piece(b, W_QUEEN, ANY_SQUARE, 8);
    place_piece(b, W_ROOK, ANY_SQUARE, 8);
    place_piece(b, W_ROOK, ANY_SQUARE, 8);
    place_piece(b, W_BISHOP, ANY_SQUARE, 8);
    place_piece(b, W_BISHOP, ANY_SQUARE, 8);
    place_piece(b, W_KNIGHT, ANY_SQUARE, 8);
    place_piece(b, W_KNIGHT, ANY_SQUARE, 8);
    /* Black mirrors White */
    for (i = 0; i < 8; i++) {
      b[i][7] = b[i][0] | BLACK;
    }
    break;
  case 3:
    /* Generate White king on random square plus random set of pieces */
    place_piece(b, W_KING, ANY_SQUARE, 8);
    for (i = 0; i < 8; i++) {
      if (b[i][0] != W_KING) {
	b[i][0] = (random() % 4) + 2; if(b[i][0] == 5) b[i][0] += 3;
      }
    }
    /* Black mirrors White */
    for (i = 0; i < 8; i++) {
      b[i][7] = b[i][0] | BLACK;
    }
    break;
  case 4:
    /* Generate White king on random square plus random set of pieces */
    place_piece(b, W_KING, ANY_SQUARE, 8);
    for (i = 0; i < 8; i++) {
      if (b[i][0] != W_KING) {
	b[i][0] = (random() % 4) + 2; if(b[i][0] == 5) b[i][0] += 3;
      }
    }
    /* Black has same set of pieces, but randomly permuted, except that Black
       must have the same number of bishops on white squares as White has on
       black squares, and vice versa.  So we must place Black's bishops first
       to be sure there are enough squares left of the correct color. */
    for (i = 0; i < 8; i++) {
      if (b[i][0] == W_BISHOP) {
	place_piece(b, B_BISHOP, !SquareColor(i, 0), 8);
      }
    }
    for (i = 0; i < 8; i++) {
      if (b[i][0] != W_BISHOP) {
	place_piece(b, b[i][0] | BLACK, ANY_SQUARE, 8);
      }
    }
    break;
  case 22:
    /* Chess960 placement: King between R */
    place_piece(b, W_BISHOP, WHITE_SQUARE, 8);
    place_piece(b, W_BISHOP, BLACK_SQUARE, 8);
    place_piece(b, W_QUEEN,  ANY_SQUARE, 8);
    place_piece(b, W_KNIGHT, ANY_SQUARE, 8);
    place_piece(b, W_KNIGHT, ANY_SQUARE, 8);
    for (i = j = 0; i < 8; i++) {
      if(b[i][0] == NOPIECE) b[i][0] = (j++ == 1 ? W_KING : W_ROOK);
    }
    /* Black mirrors White */
    for (i = 0; i < 8; i++) {
      b[i][7] = b[i][0] | BLACK;
    }
    break;
  case 46:
    /* Chess960 placement: King between R */
    place_piece(b, W_BISHOP, WHITE_SQUARE, 10);
    place_piece(b, W_BISHOP, BLACK_SQUARE, 10);
    place_piece(b, W_QUEEN,  ANY_SQUARE, 10);
    place_piece(b, W_MARSHALL, ANY_SQUARE, 10);
    place_piece(b, W_CARDINAL, ANY_SQUARE, 10);
    place_piece(b, W_KNIGHT, ANY_SQUARE, 10);
    place_piece(b, W_KNIGHT, ANY_SQUARE, 10);
    for (i = j = 0; i < 10; i++) {
      if(b[i][0] == NOPIECE) j++ == 1 ? W_KING : W_ROOK;
    }
    /* Black mirrors White */
    for (i = 0; i < 10; i++) {
      b[i][7] = b[i][0] | BLACK;
    }
    break;
  default:
    return;
    break;
  }
  {
    FILE *fp;
    int onPiece;

    fp = fopen_p("%s/wild/%d", "w", BOARD_DIR, style);
    if (!fp) {
      d_printf( "CHESSD: Can't write wild style %d\n", style);
      return;
    }
    fprintf(fp, "W:");
    onPiece = -1;
    for (r = 1; r >= 0; r--) {
      for (f = 0; f < 8; f++) {
	if (onPiece < 0 || b[f][r] != onPiece) {
	  onPiece = b[f][r];
	  fprintf(fp, " %s", wpstring[piecetype(b[f][r])]);
	}
	fprintf(fp, " %c%c", f + 'a', r + '1');
      }
    }
    fprintf(fp, "\nB:");
    onPiece = -1;
    for (r = 6; r < 8; r++) {
      for (f = 0; f < 8; f++) {
	if (onPiece < 0 || b[f][r] != onPiece) {
	  onPiece = b[f][r];
	  fprintf(fp, " %s", wpstring[piecetype(b[f][r])]);
	}
	fprintf(fp, " %c%c", f + 'a', r + '1');
      }
    }
    fprintf(fp, "\n");
    fclose(fp);
  }
}

void wild_init(void)
{
	board_t b;
	wild_update(b, 1);
	wild_update(b, 2);
	wild_update(b, 3);
	wild_update(b, 4);
}

