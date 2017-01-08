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

#if 0
/* One line has everything on it */
static int WriteMoves(FILE * fp, struct move_t *m)
{
  unsigned long MoveInfo = (m->color == BLACK);
  piece_t piece;
  int castle;
  int useFile = 0, useRank = 0, check = 0;
  int i;

  castle = (m->moveString[0] == 'o');
  if (castle)
    piece = KING;
  else
    piece = piecetype(CharToPiece(m->moveString[0]));

  MoveInfo = (MoveInfo <<= 3) | piece;
  MoveInfo = (MoveInfo <<= 3) | m->fromFile;
  MoveInfo = (MoveInfo <<= 3) | m->fromRank;
  MoveInfo = (MoveInfo <<= 3) | m->toFile;
  MoveInfo = (MoveInfo <<= 3) | m->toRank;
  MoveInfo = (MoveInfo <<= 3) | (m->pieceCaptured & 7);
  MoveInfo = (MoveInfo <<= 3) | (m->piecePromotionTo & 7);
  MoveInfo = (MoveInfo <<= 1) | (m->enPassant != 0);

  /* Are we using from-file or from-rank in algString? */
  i = strlen(m->algString) - 1;
  if (m->algString[i] == '+') {
    check = 1;
    i--;
  }
  if (piece != PAWN && !castle) {
    i -= 2;
    if (i < 0)
      return -1;
    if (m->algString[i] == 'x')
      i--;
    if (i < 0)
      return -1;
    if (isdigit(m->algString[i])) {
      useRank = 2;
      i--;
    }
    if (i < 0)
      return -1;
    useFile = (islower(m->algString[i]) ? 4 : 0);
  }
  MoveInfo = (MoveInfo << 3) | useFile | useRank | check;
  fprintf(fp, "%lx %x %x\n", MoveInfo, m->tookTime, m->atTime);

  return 0;
}
#endif

static int ReadMove(FILE * fp, struct move_t *m)
{
  char line[MAX_GLINE_SIZE];
  fgets(line, MAX_GLINE_SIZE - 1, fp);
  if (sscanf(line, "%d %d %d %d %d %d %d %d %d \"%[^\"]\" \"%[^\"]\" %u %u\n",
	     &m->color, &m->fromFile, &m->fromRank, &m->toFile, &m->toRank,
     &m->pieceCaptured, &m->piecePromotionTo, &m->enPassant, &m->doublePawn,
	     m->moveString, m->algString, &m->atTime, &m->tookTime) != 13)
    return -1;
  return 0;
}

#if 0
static void WriteGameState(FILE * fp, struct game_state_t *gs)
{
  int i, j;

  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++) {
      fprintf(fp, "%c", PieceToChar(gs->board[i][j]));
    }
  fprintf(fp, "%d %d %d %d %d %d",
	  gs->wkmoved, gs->wqrmoved, gs->wkrmoved,
	  gs->bkmoved, gs->bqrmoved, gs->bkrmoved);
  for (i = 0; i < 8; i++)
    fprintf(fp, " %d %d", gs->ep_possible[0][i], gs->ep_possible[1][i]);
  fprintf(fp, " %d %d %d\n", gs->lastIrreversable, gs->onMove, gs->moveNum);
}
#endif

#if 0
static void WriteGameFile(FILE * fp, int g)
{
  int i;
  struct game *gg = &game_globals.garray[g];
  struct player *wp = &player_globals.parray[gg->white], *bp = &player_globals.parray[gg->black];

  fprintf(fp, "v %d\n", GAMEFILE_VERSION);
  fprintf(fp, "%s %s\n", wp->name, bp->name);
  fprintf(fp, "%d %d\n", gg->white_rating, gg->black_rating);
  fprintf(fp, "%d %d %d %d\n", gg->wInitTime, gg->wIncrement,
	  gg->bInitTime, gg->bIncrement);
  fprintf(fp, "%lx\n", gg->timeOfStart);
  fprintf(fp, "%d %d\n",
    (net_globals.con[wp->socket]->timeseal ? gg->wRealTime/100 : gg->wTime),
    (net_globals.con[bp->socket]->timeseal ? gg->bRealTime/100 : gg->bTime));
  fprintf(fp, "%d %d\n", gg->result, gg->winner);
  fprintf(fp, "%d %d %d %d\n", gg->private, gg->type,
	  gg->rated, gg->clockStopped);
  fprintf(fp, "%d\n", gg->numHalfMoves);
  for (i = 0; i < game_globals.garray[g].numHalfMoves; i++) {
    WriteMoves(fp, &game_globals.garray[g].moveList[i]);
  }
/* took out the next 3 lines to see if it helps with the crash bug we are
   having on examine...  fb 2.25.96 */
/* The next three lines stop wild games crashing the system - they are vital.
   I fixed the problem with examine - don't remove these again - DAV */
  if (strcmp(gg->FENstartPos, INITIAL_FEN) != 0)
    fprintf (fp, "%s\n",gg->FENstartPos);
  else
    fprintf (fp, "d w\n");
  WriteGameState(fp, &game_globals.garray[g].game_state);
}
#endif



static int ReadGameState(FILE * fp, struct game_state_t *gs, int version)
{
  int i, j;
  char pieceChar;
  int wkmoved, wqrmoved, wkrmoved, bkmoved, bqrmoved, bkrmoved;

  if (version == 0) {
    for (i = 0; i < 8; i++)
      for (j = 0; j < 8; j++)
	if (fscanf(fp, "%d ", &gs->board[i][j]) != 1)
	  return -1;
  } else {
    for (i = 0; i < 8; i++)
      for (j = 0; j < 8; j++) {
	pieceChar = getc(fp);
	gs->board[i][j] = CharToPiece(pieceChar, NULL);
      }
  }
  if (fscanf(fp, "%d %d %d %d %d %d",
	     &wkmoved, &wqrmoved, &wkrmoved,
	     &bkmoved, &bqrmoved, &bkrmoved) != 6)
    return -1;
  gs->wkmoved = wkmoved;
  gs->wqrmoved = wqrmoved;
  gs->wkrmoved = wkrmoved;
  gs->bkmoved = bkmoved;
  gs->bqrmoved = bqrmoved;
  gs->bkrmoved = bkrmoved;
  for (i = 0; i < 8; i++)
    if (fscanf(fp, " %d %d", &gs->ep_possible[0][i], &gs->ep_possible[1][i]) != 2)
      return -1;
  if (fscanf(fp, " %d %d %d\n", &gs->lastIrreversable, &gs->onMove, &gs->moveNum) != 3)
    return -1;
  return 0;
}


static int got_attr_value(int g, char *attr, char *value, FILE * fp)
{
  int i;

  if (!strcmp(attr, "w_init:")) {
    game_globals.garray[g].wInitTime = atoi(value);
  } else if (!strcmp(attr, "w_inc:")) {
    game_globals.garray[g].wIncrement = atoi(value);
  } else if (!strcmp(attr, "b_init:")) {
    game_globals.garray[g].bInitTime = atoi(value);
  } else if (!strcmp(attr, "b_inc:")) {
    game_globals.garray[g].bIncrement = atoi(value);
  } else if (!strcmp(attr, "white_name:")) {
    strcpy(game_globals.garray[g].white_name, value);
  } else if (!strcmp(attr, "black_name:")) {
    strcpy(game_globals.garray[g].black_name, value);
  } else if (!strcmp(attr, "white_rating:")) {
    game_globals.garray[g].white_rating = atoi(value);
  } else if (!strcmp(attr, "black_rating:")) {
    game_globals.garray[g].black_rating = atoi(value);
  } else if (!strcmp(attr, "result:")) {
    game_globals.garray[g].result = atoi(value);
  } else if (!strcmp(attr, "timestart:")) {
    game_globals.garray[g].timeOfStart = atoi(value);
  } else if (!strcmp(attr, "w_time:")) {
    game_globals.garray[g].wTime = atoi(value);
  } else if (!strcmp(attr, "b_time:")) {
    game_globals.garray[g].bTime = atoi(value);
  } else if (!strcmp(attr, "clockstopped:")) {
    game_globals.garray[g].clockStopped = atoi(value);
  } else if (!strcmp(attr, "rated:")) {
    game_globals.garray[g].rated = atoi(value);
  } else if (!strcmp(attr, "private:")) {
    game_globals.garray[g].private = atoi(value);
  } else if (!strcmp(attr, "type:")) {
    game_globals.garray[g].type = atoi(value);
  } else if (!strcmp(attr, "halfmoves:")) {
    game_globals.garray[g].numHalfMoves = atoi(value);
    if (game_globals.garray[g].numHalfMoves == 0)
      return 0;
    game_globals.garray[g].moveListSize = game_globals.garray[g].numHalfMoves;
    game_globals.garray[g].moveList = (struct move_t *) malloc(sizeof(struct move_t) * game_globals.garray[g].moveListSize);
    for (i = 0; i < game_globals.garray[g].numHalfMoves; i++) {
      if (ReadMove(fp, &game_globals.garray[g].moveList[i])) {
	d_printf( "CHESSD: Trouble reading moves\n");
	return -1;
      }
    }
  } else if (!strcmp(attr, "gamestate:")) {	/* Value meaningless */
    if (game_globals.garray[g].status != GAME_EXAMINE && game_globals.garray[g].status != GAME_SETUP &&
	ReadGameState(fp, &game_globals.garray[g].game_state, 0)) {
      d_printf( "CHESSD: Trouble reading game state\n");
      return -1;
    }
  } else {
    d_printf( "CHESSD: Error bad attribute >%s<\n", attr);
  }
  return 0;
}

static void ReadOneV1Move(FILE * fp, struct move_t *m)
{
  int i;
  char PieceChar;
  int useFile, useRank, check, piece;
  unsigned long MoveInfo;

  fscanf(fp, "%lx %x %x", &MoveInfo, &m->tookTime, &m->atTime);
  check = MoveInfo & 1;
  useRank = MoveInfo & 2;
  useFile = MoveInfo & 4;
  MoveInfo >>= 3;
  m->enPassant = MoveInfo & 1;	/* may have to negate later. */
  MoveInfo >>= 1;
  m->piecePromotionTo = MoveInfo & 7;	/* may have to change color. */
  MoveInfo >>= 3;
  m->pieceCaptured = MoveInfo & 7;	/* may have to change color. */
  MoveInfo >>= 3;
  m->toRank = MoveInfo & 7;
  MoveInfo >>= 3;
  m->toFile = MoveInfo & 7;
  MoveInfo >>= 3;
  m->fromRank = MoveInfo & 7;
  MoveInfo >>= 3;
  m->fromFile = MoveInfo & 7;
  MoveInfo >>= 3;
  piece = MoveInfo & 7;

  m->color = (MoveInfo & 8) ? BLACK : WHITE;
  if (m->pieceCaptured != NOPIECE) {
    if (m->color == BLACK)
      m->pieceCaptured |= WHITE;
    else
      m->pieceCaptured |= BLACK;
  }
  if (piece == PAWN) {
    PieceChar = 'P';
    if ((m->toRank == 3 && m->fromRank == 1)
	|| (m->toRank == 4 && m->fromRank == 6))
      m->doublePawn = m->toFile;
    else
      m->doublePawn = -1;
    if (m->pieceCaptured)
      sprintf(m->algString, "%cx%c%d", 'a' + m->fromFile,
	      'a' + m->toFile, m->toRank + 1);
    else
      sprintf(m->algString, "%c%d", 'a' + m->toFile, m->toRank + 1);
    if (m->piecePromotionTo != 0) {
      if (m->piecePromotionTo == KNIGHT)
	strcat(m->algString, "=N");
      else if (m->piecePromotionTo == BISHOP)
	strcat(m->algString, "=B");
      else if (m->piecePromotionTo == ROOK)
	strcat(m->algString, "=R");
      else if (m->piecePromotionTo == QUEEN)
	strcat(m->algString, "=Q");
      m->piecePromotionTo |= m->color;
    }
    if (m->enPassant)
      m->enPassant = m->toFile - m->fromFile;
  } else {
    m->doublePawn = -1;
    PieceChar = PieceToChar(piecetype(piece) | WHITE);
    if (PieceChar == 'K' && m->fromFile == 4 && m->toFile == 6) {
      strcpy(m->algString, "O-O");
      strcpy(m->moveString, "o-o");
    } else if (PieceChar == 'K' && m->fromFile == 4 && m->toFile == 2) {
      strcpy(m->algString, "O-O-O");
      strcpy(m->moveString, "o-o-o");
    } else {
      i = 0;
      m->algString[i++] = PieceChar;
      if (useFile)
	m->algString[i++] = 'a' + m->fromFile;
      if (useRank)
	m->algString[i++] = '1' + m->fromRank;
      if (m->pieceCaptured != 0)
	m->algString[i++] = 'x';
      m->algString[i++] = 'a' + m->toFile;
      m->algString[i++] = '1' + m->toRank;
      m->algString[i] = '\0';
    }
    if (m->piecePromotionTo != 0) { // must be Shogi promotion
	strcat(m->algString, "=+");
      m->piecePromotionTo |= m->color;
    }
  }
  if (m->algString[0] != 'O')
    sprintf(m->moveString, "%c/%c%d-%c%d", PieceChar, 'a' + m->fromFile,
	    m->fromRank + 1, 'a' + m->toFile, m->toRank + 1);
  if (check)
    strcat(m->algString, "+");
}

static int ReadV1Moves(struct game *g, FILE * fp)
{
  int i;

  g->moveListSize = g->numHalfMoves;
  g->moveList = (struct move_t *) malloc(sizeof(struct move_t) * g->moveListSize);
  for (i = 0; i < g->numHalfMoves; i++) {
    ReadOneV1Move(fp, &g->moveList[i]);
  }
  return 0;
}

static int ReadV1GameFmt(struct game *g, FILE * fp, int version)
{
  char* FEN;
  char tmp[MAX_STRING_LENGTH];
  unsigned result;

  fscanf(fp, "%s %s", g->white_name, g->black_name);
  fscanf(fp, "%d %d", &g->white_rating, &g->black_rating);
  fscanf(fp, "%d %d %d %d", &g->wInitTime, &g->wIncrement,
	 &g->bInitTime, &g->bIncrement);
  if ((version < 3) && (!(g->bInitTime)))
    g->bInitTime = g->wInitTime;
                       /*PRE-V3 assumed bInitTime was 0 if balanced clocks*/
  fscanf(fp, "%lx", &g->timeOfStart);
  fscanf(fp, "%d %d", &g->wTime, &g->bTime);

/* fixing an (apparently) old bug: winner not saved */
  if (version > 1)
    fscanf(fp, "%d %d", &result, &g->winner);
  else
    fscanf(fp, "%d", &result);

  g->result = (enum gameend)result;

  fscanf(fp, "%d %d %d %d", &g->private, (int *) &g->type,
	 &g->rated, &g->clockStopped);
  fscanf(fp, "%d", &g->numHalfMoves);
  ReadV1Moves(g, fp);

  if (version >= 4) {
    getc(fp);                   /* Skip past a newline. */ 

    fgets(tmp, MAX_LINE_SIZE, fp);
    tmp [strlen(tmp)-1] = '\0'; /* kill the newline char */

    if ((tmp[0] == '\0') || (!strcmp(tmp,"d w"))) {
	    /* default position */
	    strcpy (g->FENstartPos,INITIAL_FEN);
    } else {
      if (!strcmp(tmp,"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"))
                 /* missing colour for default pos 1.7.1 bug */
	      strcpy(g->FENstartPos,INITIAL_FEN);
      else {
        strcpy (g->FENstartPos,tmp);
        FEN = g->FENstartPos;
        while (*(FEN) != '/') { /* check for missing fen pos (1.7.1 bug) */
          if (*(FEN++) == '\0') {
            d_printf("Corrupt game %s vs %s!\n",g->white_name,g->black_name);
            return -1;
          }
        }
      }
    }
  } else
    getc(fp);                   /* Skip past a newline. */

  if (g->status != GAME_EXAMINE && g->status != GAME_SETUP) { 
    if (ReadGameState(fp, &g->game_state, version)) {
      d_printf( "CHESSD: Trouble reading game state\n");
      return -1;
    }
  } else if (g->status == GAME_EXAMINE)
    FEN_to_board(g->FENstartPos, &g->game_state);
  return 0;
}


int ReadGameAttrs_old(FILE * fp, int g,int version)
{
  int len;
  char *attr, *value;
  char line[MAX_GLINE_SIZE];

  if (version > 0) {
    if ((ReadV1GameFmt(&game_globals.garray[g], fp, version)) < 0)
      return -1;
  }
  /* Read the game file here */
  else
    do {
      if ((len = strlen(line)) <= 1) {
	fgets(line, MAX_GLINE_SIZE - 1, fp);
	continue;
      }
      line[len - 1] = '\0';
      attr = eatwhite(line);
      if (attr[0] == '#')
	continue;		/* Comment */
      value = eatword(attr);
      if (!*value) {
	d_printf( "CHESSD: Error reading file\n");
	fgets(line, MAX_GLINE_SIZE - 1, fp);
	continue;
      }
      *value = '\0';
      value++;
      value = eatwhite(value);
      if (!*value) {
	d_printf( "CHESSD: Error reading file\n");
	fgets(line, MAX_GLINE_SIZE - 1, fp);
	continue;
      }
      stolower(attr);
      if (got_attr_value(g, attr, value, fp)) {
	return -1;
      }
      fgets(line, MAX_GLINE_SIZE - 1, fp);
    } while (!feof(fp)); 
  if (!(game_globals.garray[g].bInitTime))
     game_globals.garray[g].bInitTime = game_globals.garray[g].wInitTime;

  return 0;
}
