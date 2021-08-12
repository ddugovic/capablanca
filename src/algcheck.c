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

/* Well, lets see if I can list the possibilities
 * Piece moves
 * Ne4
 * Nxe4
 * Nce4
 * Ncxe4
 * R2f3
 * R2xf3
 * Special pawn moves
 * e4
 * ed
 * exd
 * exd5
 * ed5
 * Drop moves (bughouse, board edit)
 * P@f7 P*f7
 * #f7 #Nf7
 * (o-o, o-o-o) Castling is handled earlier, so don't worry about that
 * Of course any of these can have a + or ++ or = string on the end, just
 * cut that off.
 */

/* f - file
 * r - rank
 * p - piece
 * x - x
 * @ - drop character (bughouse)
 */
static char *alg_list[] = {
  "fxfr", "pxfr",		/* These two get confused in case of bishop */
  "ffr", "pfr",			/* These two get confused in case of bishop */
  "pffr",
  "pfxfr",
  "prfr",
  "prxfr",
  "fr",
  "ff",
  "fxf",
  "p@fr",
  "#fr",
  "#pfr",
  NULL
};

#define ALG_UNKNOWN -1

static int get_move_info(const char *str, piece_t *piece, int *ff, int *fr, int *tf, int *tr, int *bishconfusion)
{
  char tmp[1024];
  char *s;
  int i, j, len;
  char c;
  int matchVal = -1;
  int lpiece, lff, lfr, ltf, ltr;

  *bishconfusion = 0;
  strlcpy(tmp, str, sizeof(tmp));
  if ((s = strchr(tmp, '+'))) {	/* Cut off any check marks */
    *s = '\0';
  }
  if ((s = strchr(tmp, '='))) {	/* Cut off any promotion marks */
    *s = '\0';
  }
  if ((s = strchr(tmp, '#'))) {	/* Cut off any 'mates' marks */
    *s = '\0';
  }
  *piece = *ff = *fr = *tf = *tr = ALG_UNKNOWN;
  len = strlen(tmp);
  for (i = 0; alg_list[i]; i++) {
    lpiece = lff = lfr = ltf = ltr = ALG_UNKNOWN;
    if (strlen(alg_list[i]) != len)
      continue;
    for (j = len - 1; j >= 0; j--) {
      switch (alg_list[i][j]) {
      case 'f':
	if ((tmp[j] < 'a') || (tmp[j] > 'l')) // [HGM] upto l-file
	  goto nomatch;
	if (ltf == ALG_UNKNOWN)
	  ltf = tmp[j] - 'a';
	else
	  lff = tmp[j] - 'a';
	break;
      case 'r':
	if ((tmp[j] < '0') || (tmp[j] > '9')) // [HGM] also match 0- and 9-rank
	  goto nomatch;
	if (ltr == ALG_UNKNOWN)
	  ltr = tmp[j] - '0'; // [HGM] allow 0-rank for Xiangqi, correct later
	else
	  lfr = tmp[j] - '0';
	break;
      case 'p':
	if (isupper(tmp[j]))
	  c = tolower(tmp[j]);
	else
	  c = tmp[j];
	if (c == 'k')
	  lpiece = KING;
	else if (c == 'e')   // [HGM] note that som piece indicators are ambiguous,
	  lpiece = ELEPHANT; //       and their true meaning depends on the variant,
	else if (c == 'v')   //       which we do not know at this point.
	  lpiece = CENTAUR;
	else if (c == 's')
	  lpiece = SILVER;
	else if (c == 'g')
	  lpiece = GOLD;
	else if (c == 'l')
	  lpiece = LANCE;
	else if (c == 'f')
	  lpiece = FERZ;
	else if (c == 'h')
	  lpiece = HORSE;
	else if (c == 'w')
	  lpiece = WAZIR;
	else if (c == 'o')
	  lpiece = SQUIRREL;
	else if (c == 'q')
	  lpiece = QUEEN;
	else if (c == 'c')
	  lpiece = MARSHALL;
	else if (c == 'a')
	  lpiece = CARDINAL;
	else if (c == 'm')
	  lpiece = MAN;
	else if (c == 'r')
	  lpiece = ROOK;
	else if (c == 'b')
	  lpiece = BISHOP;
	else if (c == 'n')
	  lpiece = KNIGHT;
	else if (c == 'p')
	  lpiece = PAWN;
	else if (c == 'd')
	  lpiece = DRAGONKING;
	else
	  goto nomatch;
	break;
      case 'x':
	if ((tmp[j] != 'x') && (tmp[j] != 'X'))
	  goto nomatch;
	break;
      case '@':
	if (tmp[j] != '@' && tmp[j] != '*')
	  goto nomatch;
	lff = lfr = ALG_DROP;
	break;
      case '#':
	if (tmp[j] != '#')
	  goto nomatch;
	lff = lfr = ALG_DROP;
	break;
      default:
	d_printf( "Unknown character in algebraic parsing\n");
	break;
      }
    }
    if (lpiece == ALG_UNKNOWN)
      lpiece = PAWN;
    if (lpiece == PAWN && (lfr == ALG_UNKNOWN)) {	/* ffr or ff */
      if (lff != ALG_UNKNOWN) {
	if (lff == ltf)
	  goto nomatch;
	if ((lff - ltf != 1) && (ltf - lff != 1))
	  goto nomatch;
      }
    }
    *piece = lpiece;		/* We have a match */
    *tf = ltf;
    *tr = ltr;
    *ff = lff;
    *fr = lfr;
    if (matchVal != -1) {
      /* We have two matches, it must be that Bxc4 vs. bxc4 problem */
      /* Or it could be the Bc4 vs bc4 problem */
      *bishconfusion = 1;
    }
    matchVal = i;
nomatch:;
  }

  if (matchVal != -1)
    return MS_ALG;
  else
    return MS_NOTMOVE;
}

int alg_is_move(const char *mstr)
{
	piece_t piece;
	int ff=0, fr=0, tf=0, tr=0, bc=0;

	return get_move_info(mstr, &piece, &ff, &fr, &tf, &tr, &bc);
}

/* add any promotion qualifier from a move string */
static void add_promotion(struct game_state_t *gs, const char *mstr, struct move_t * mt)
{
	char *s;
	piece_t piece;
	int i;
	s = strchr(mstr, '=');
	if (s == NULL) {
		return;
	}
	if(gs->promoType == 3) { // handle Shogi promotions
		piece = gs->board[mt->fromFile][mt->fromRank];
#if BUGHOUSE_PAWN_REVERT
		mt->piecePromotionFrom = piece;
#endif
 		if(colorval(piece) == WHITE && mt->fromRank < gs->ranks - gs->ranks/3
					    && mt->toRank   < gs->ranks - gs->ranks/3 ) return;
 		if(colorval(piece) == BLACK && mt->fromRank >= gs->ranks/3
					    && mt->toRank   >= gs->ranks/3 ) return;
                switch(piecetype(piece)) {
		    case PAWN:
		    case LANCE:
		    case HONORABLEHORSE:
		    case SILVER:
			if(s[1] != '+' && s[1] != '^' && s[1] != 'G' && s[1] != 'g') return;
			piece = GOLD; break;
		    case BISHOP:
			if(s[1] != '+' && s[1] != '^' && s[1] != 'H' && s[1] != 'h') return;
			piece = DRAGONHORSE; break;
		    case ROOK:
			if(s[1] != '+' && s[1] != '^' && s[1] != 'D' && s[1] != 'd') return;
			piece = DRAGONKING; break;
		    default: return; // others do not promote, so ignore
		}
		mt->piecePromotionTo = piece | colorval(gs->board[mt->fromFile][mt->fromRank]);
		return;
 	}

      if(gs->drops != 2 || (gs->onMove == WHITE ? 0 : gs->ranks-1) != mt->fromRank) { // [HGM] always accept if backrank mover in Seirawan
	if (piecetype(gs->board[mt->fromFile][mt->fromRank]) != PAWN &&
	    piecetype(gs->board[mt->fromFile][mt->fromRank]) != HOPLITE) {
		return;
	}
	if (mt->toRank < gs->ranks - gs->promoZone && mt->toRank >= gs->promoZone) {
		return;
	}
      }

	switch (tolower(s[1])) {
	case 'f':
		piece = FERZ2;
		break;
	case 'q':
		piece = QUEEN;
		break;
	case 'c':
		if(piecetype(gs->board[mt->fromFile][mt->fromRank]) == HOPLITE) piece = CAPTAIN; else
		if(!gs->capablancaPieces) return; // [HGM] should make variant-dependent piece mask
		piece = MARSHALL;
		break;
	case 'a':
		if(!gs->capablancaPieces) return;
		piece = CARDINAL;
		break;
	case 'm':
		if(!gs->royalKnight) return; // [HGM] only in knightmate
		piece = MAN;
		break;
	case 'r':
		piece = ROOK;
		break;
	case 'b':
		piece = BISHOP;
		break;
	case 'n':
		if(gs->royalKnight) return; // [HGM] not in knightmate
		piece = KNIGHT;
		break;
	// Superchess promotons: filtered out later by promoType
	case 'g':
		if(piecetype(gs->board[mt->fromFile][mt->fromRank]) == HOPLITE) piece = GENERAL; else
		piece = MASTODON;
		break;
	case 'o':
		piece = SQUIRREL;
		break;
	case 'w':
		if(piecetype(gs->board[mt->fromFile][mt->fromRank]) == HOPLITE) piece = WARLORD; else
		piece = WOODY;
		break;
	case 'k':
		if(piecetype(gs->board[mt->fromFile][mt->fromRank]) != HOPLITE) return;
		piece = KING;
		break;
	case 'l':
		if(piecetype(gs->board[mt->fromFile][mt->fromRank]) != HOPLITE) return;
		piece = LIEUTENANT;
		break;
	case 'v':
		piece = CENTAUR;
		break;
	case 'e':
		piece = gs->drops == 2 ? SELEPHANT : EMPRESS; // for Seirawan
		break;
	case 's':
		piece = PRINCESS;
		break;
	case 'h':
		if(gs->drops != 2) return;
		piece = HAWK;
		break;
	default:
		return;
	}
#if BUGHOUSE_PAWN_REVERT
	mt->piecePromotionFrom = piece;
#endif
	i = colorval(gs->board[mt->fromFile][mt->fromRank]) == WHITE ? 0 : 1;
	if(gs->promoType == 2 && gs->holding[i][piece-PAWN] == 0) return; // only if piece was captured
	if(piece >= WOODY && piece < KING && (gs->promoType != 2 || gs->promoZone == 3)) return; // reserved for Superchess

	mt->piecePromotionTo = piece | colorval(gs->board[mt->fromFile][mt->fromRank]);
}

/* We already know it is algebraic, get the move squares */
int alg_parse_move(char *mstr, struct game_state_t * gs, struct move_t * mt)
{
	int f=0, r=0, tmpr=0, posf=0, posr=0, posr2=0;
	piece_t piece;
	int ff=0, fr=0, tf=0, tr=0, bc=0;

  if (get_move_info(mstr, &piece, &ff, &fr, &tf, &tr, &bc) != MS_ALG) {
    d_printf( "CHESSD: Shouldn't try to algebraicly parse non-algabraic move string.\n");
    return MOVE_ILLEGAL;
  }
  // [HGM] check if move does not stray off board
  if(gs->ranks < 10) { 
    if(tr == 0 || fr == 0) return MOVE_ILLEGAL; // used nonexistent 0-rank
    if(tr != ALG_UNKNOWN) tr--;
    if(fr != ALG_UNKNOWN) fr--; // shift to lowest rank = 1
  }
  if(tr >= gs->ranks || fr >= gs->ranks || tf >= gs->files || ff >= gs->files)
    return MOVE_ILLEGAL;

  // [HGM] resolve ambiguity in piece, type based on variant
  switch(piece) {
    case ELEPHANT:
      if(strstr(gs->variant, "super"))   piece = EMPRESS; else
      if(strstr(gs->variant, "seirawan"))piece = SELEPHANT; else
      if(strstr(gs->variant, "great"))   piece = MODERNELEPHANT; else
      if(strstr(gs->variant, "courier")) piece = ALFIL2;
      break;
    case CARDINAL:
      if(strstr(gs->variant, "super")) piece = AMAZON; else
      if(strstr(gs->variant, "xiangqi")) piece = MANDARIN;
      break;
    case MARSHALL:
      if(strstr(gs->variant, "xiangqi")) piece = CANNON;
      break;
    case SILVER:
      if(strstr(gs->variant, "super")) piece = PRINCESS;
      if(strstr(gs->variant, "great")) piece = MAN2;
      break;
    case BISHOP:
      if(strstr(gs->variant, "shatranj")) piece = ALFIL;
      break;
    case QUEEN:
      if(strstr(gs->variant, "shatranj")) piece = FERZ;
      break;
    case WAZIR:
      if(strstr(gs->variant, "super")) piece = WOODY;
      break;
    case KNIGHT:
      if(strstr(gs->variant, "shogi")) piece = HONORABLEHORSE;
      break;
    case MAN:
      if(strstr(gs->variant, "great")) piece = MINISTER;
      break;
    case HORSE:
      if(strstr(gs->variant, "shogi")) piece = DRAGONHORSE; else
      if(strstr(gs->variant, "seirawan")) piece = HAWK; else
      if(strstr(gs->variant, "great")) piece = PRIESTESS;
      break;
    case GOLD:
      if(strstr(gs->variant, "great")) piece = MASTODON;
      break;
  }

  /* Resolve ambiguities in to-ness */
  if (tf == ALG_UNKNOWN) {
	  d_printf("Ambiguous %s(%d)\n", __FUNCTION__, __LINE__);
	  return MOVE_AMBIGUOUS;	/* Must always know to file */
  }
  if (tr == ALG_UNKNOWN) {
    posr = posr2 = ALG_UNKNOWN;
    if (piece != PAWN) {
	    d_printf("Ambiguous %s(%d)\n", __FUNCTION__, __LINE__);
	    return MOVE_AMBIGUOUS;
    }
    if (ff == ALG_UNKNOWN) {
	    d_printf("Ambiguous %s(%d)\n", __FUNCTION__, __LINE__);
	    return MOVE_AMBIGUOUS;
    }
    /* Need to find pawn on ff that can take to tf and fill in ranks */
    for (InitPieceLoop(gs->board, &f, &r, gs->onMove);
	 NextPieceLoop(gs->board, &f, &r, gs->onMove, gs->files, gs->ranks);) {
      if ((ff != ALG_UNKNOWN) && (ff != f))
	continue;
      if (piecetype(gs->board[f][r]) != piece)
	continue;
      if (gs->onMove == WHITE) {
	tmpr = r + 1;
      } else {
	tmpr = r - 1;
      }
/*      if ((gs->board[tf][tmpr] == NOPIECE) ||
          (iscolor(gs->board[tf][tmpr], gs->onMove))) continue;*/
/* patch from Soso, added by Sparky 3/16/95                    */
      if (gs->board[tf][tmpr] == NOPIECE) {
	if ((gs->ep_possible[((gs->onMove == WHITE) ? 0 : 1)][ff]) != (tf - ff))
	  continue;
      } else {
	if (iscolor(gs->board[tf][tmpr], gs->onMove))
	  continue;
      }

      if (legal_andcheck_move(gs, f, r, tf, tmpr)) {
	      if ((posr != ALG_UNKNOWN) && (posr2 != ALG_UNKNOWN)) {
		      d_printf("Ambiguous %s(%d)\n", __FUNCTION__, __LINE__);
		      return MOVE_AMBIGUOUS;
	      }
	posr = tmpr;
	posr2 = r;
      }
    }
    tr = posr;
    fr = posr2;
  } else if (bc) {		/* Could be bxc4 or Bxc4, tr is known */
    ff = ALG_UNKNOWN;
    fr = ALG_UNKNOWN;
    for (InitPieceLoop(gs->board, &f, &r, gs->onMove);
	 NextPieceLoop(gs->board, &f, &r, gs->onMove, gs->files, gs->ranks);) {
	    if ((piecetype(gs->board[f][r]) != PAWN) && (piecetype(gs->board[f][r]) != piece)) {
		    // note that the interpretation Bxc4 is matched last, and has set piece to BISHOP
		    continue;
	    }
	    if (legal_andcheck_move(gs, f, r, tf, tr)) {
		    if ((piecetype(gs->board[f][r]) == PAWN) && (f != tolower(mstr[0]) - 'a')) {
			    continue;
		    }

		    /* if its a lowercase 'b' then prefer the pawn move if there is one */
		    if ((ff != ALG_UNKNOWN) && (fr != ALG_UNKNOWN) &&
			piecetype(gs->board[f][r]) == PAWN && mstr[0] >= 'a') {
			    ff = f;
			    fr = r;
			    continue;
		    }

		    if ((ff != ALG_UNKNOWN) && (fr != ALG_UNKNOWN) &&
			piecetype(gs->board[ff][fr]) == PAWN && mstr[0] >= 'a') {
			    continue;
		    }

		    if ((ff != ALG_UNKNOWN) && (fr != ALG_UNKNOWN)) {
			    d_printf("Ambiguous %s(%d) mstr=%s\n", __FUNCTION__, __LINE__, mstr);
			    return (MOVE_AMBIGUOUS);
		    }
		    ff = f;
		    fr = r;
	    }
    }
  } else {			/* The from position is unknown */
    posf = ALG_UNKNOWN;
    posr = ALG_UNKNOWN;
    if ((ff == ALG_UNKNOWN) || (fr == ALG_UNKNOWN)) {
      /* Need to find a piece that can go to tf, tr */
      for (InitPieceLoop(gs->board, &f, &r, gs->onMove);
	   NextPieceLoop(gs->board, &f, &r, gs->onMove, gs->files, gs->ranks);) {
	if ((ff != ALG_UNKNOWN) && (ff != f))
	  continue;
	if ((fr != ALG_UNKNOWN) && (fr != r))
	  continue;
	if (piecetype(gs->board[f][r]) != piece)
	  continue;
	if (legal_andcheck_move(gs, f, r, tf, tr)) {
		if ((posf != ALG_UNKNOWN) && (posr != ALG_UNKNOWN)) {
			d_printf("Ambiguous %s(%d)\n", __FUNCTION__, __LINE__);
			return MOVE_AMBIGUOUS;
		}
	  posf = f;
	  posr = r;
	}
      }
    } else if (ff == ALG_DROP) {
      if (legal_andcheck_move(gs, ALG_DROP, piece, tf, tr)) {
	posf = ALG_DROP;
	posr = piece;
      }
    }
    ff = posf;
    fr = posr;
  }
  if ((tf == ALG_UNKNOWN) || (tr == ALG_UNKNOWN) ||
      (ff == ALG_UNKNOWN) || (fr == ALG_UNKNOWN))
    return MOVE_ILLEGAL;
  mt->fromFile = ff;
  mt->fromRank = fr;
  mt->toFile = tf;
  mt->toRank = tr;

  add_promotion(gs, mstr, mt);

  return MOVE_OK;
}

/* A assumes the move has yet to be made on the board */

/* Soso: rewrote alg_unparse function.
 * Algebraic deparser - sets the mStr variable with move description
 * in short notation. Used in last move report and in 'moves' command.
 */

char *alg_unparse(struct game_state_t * gs, struct move_t * mt)
{
  static char mStr[MAX_MOVE_LENGTH];
  char tmp[MAX_MOVE_LENGTH];
  piece_t piece;
  int f, r;
  int ambig, r_ambig, f_ambig;
  struct game_state_t fakeMove;

  if (mt->fromFile == ALG_DROP) {
    piece = mt->fromRank;
  } else {
    piece = piecetype(gs->board[mt->fromFile][mt->fromRank]);
  }

  if (mt->fromFile == ALG_CASTLE) {
    int r = gs->onMove == WHITE ? 1 : gs->ranks;
    if(mt->toFile > mt->toRank) { // [HGM] castle: K ends right of R
      strcpy(mStr, "O-O");
    }
    if (mt->toFile < mt->toRank) { // [HGM] castle: K ends left of R
      strcpy(mStr, "O-O-O");
    }
    if(gs->drops == 2) {
	if(mt->piecePromotionTo < 0) snprintf(mStr, MAX_MOVE_LENGTH, "%c%de%d", mt->fromRank + 'a', r, r);
	goto suffix; // [HGM] in Seirawan castling can have gating suffix
    }
    goto check;
  }
  strcpy(mStr, "");
  switch (piece) {
  case PAWN:
    if (mt->fromFile == ALG_DROP) {
      strcpy(mStr,"P");
    } else if (mt->fromFile != mt->toFile 
	    || gs->board[mt->toFile][mt->toRank] != NOPIECE) { // [HGM] XQ: forward captures as "axa6"
      sprintf(tmp, "%c", mt->fromFile + 'a');
      strcpy(mStr, tmp);
    }
    break;
  case KNIGHT:
    strcpy(mStr, "N");
    break;
  case BISHOP:
    strcpy(mStr, "B");
    break;
  case ROOK:
    strcpy(mStr, "R");
    break;
  case ALFIL2:
  case AMAZON:
  case CARDINAL:
    strcpy(mStr, "A");
    break;
  case CAPTAIN:
  case CANNON:
  case MARSHALL:
    strcpy(mStr, "C");
    break;
  case MAN:
    strcpy(mStr, "M");
    break;
  case FERZ:
  case QUEEN:
    strcpy(mStr, "Q");
    break;
  case EMPRESS:
  case ELEPHANT:
  case SELEPHANT:
    strcpy(mStr, "E");
    break;
  case ALFIL:
    strcpy(mStr, "B");
    break;
  case FERZ2:
    strcpy(mStr, "F");
    break;
  case WARLORD:
  case WOODY:
  case WAZIR:
    strcpy(mStr, "W");
    break;
  case SQUIRREL:
    strcpy(mStr, "O");
    break;
  case CENTAUR:
    strcpy(mStr, "V");
    break;
  case HORSE:
  case DRAGONHORSE:
  case HAWK:
  case HOPLITE:
    strcpy(mStr, "H");
    break;
  case HONORABLEHORSE:
    strcpy(mStr, "N");
    break;
  case DRAGONKING:
    strcpy(mStr, "D");
    break;
  case LIEUTENANT:
  case LANCE:
    strcpy(mStr, "L");
    break;
  case PRINCESS:
  case SILVER:
    strcpy(mStr, "S");
    break;
  case MASTODON:
  case GENERAL:
  case GOLD:
    strcpy(mStr, "G");
    break;
  case MANDARIN:
    strcpy(mStr, "A");
    break;
  case KING:
    strcpy(mStr, "K");
    break;
  default:
    strcpy(mStr, "");
    break;
  }

  if (mt->fromFile == ALG_DROP) {
    strcat(mStr, "@");
  } else {
  /* Checks for ambiguity in short notation ( Ncb3, R8e8 or so) */
  if (piece != PAWN) {
    ambig = r_ambig = f_ambig = 0;
    for (r = 0; r < gs->ranks; r++)
      for (f = 0; f < gs->files; f++) {
	if ((gs->board[f][r] != NOPIECE) && iscolor(gs->board[f][r], gs->onMove)
	    && (piecetype(gs->board[f][r]) == piece) &&
	    ((f != mt->fromFile) || (r != mt->fromRank))) {
	  if (legal_move(gs, f, r, mt->toFile, mt->toRank)) {
	    fakeMove = *gs;
	    fakeMove.board[f][r] = NOPIECE;
	    fakeMove.board[mt->toFile][mt->toRank] = piece | gs->onMove;
	    fakeMove.onMove = CToggle(fakeMove.onMove);
	    gs->onMove = CToggle(gs->onMove);

#if 0
	    d_printf("possible move %c%d%c%d against %c%d%c%d\n",
		     'a' + f, r+1,
		     'a' + mt->toFile, mt->toRank+1,
		     'a' + mt->fromFile, mt->fromRank+1,
		     'a' + mt->toFile, mt->toRank+1);
#endif

	    if (!in_check(&fakeMove)) {
		    ambig++;
		    if (f == mt->fromFile) {
			    ambig++;
			    f_ambig++;
		    }
		    if (r == mt->fromRank) {
			    ambig++;
			    r_ambig++;
		    }
	    }
	    gs->onMove = CToggle(gs->onMove);
	  }
	}
      }
    if (ambig > 0) {
      /* Ambiguity in short notation, need to add file,rank or _both_ in
         notation */
      if (f_ambig == 0) {
	sprintf(tmp, "%c", mt->fromFile + 'a');
	strcat(mStr, tmp);
      } else if (r_ambig == 0) {
	sprintf(tmp, "%d", mt->fromRank + 1 - (gs->ranks > 9));
	strcat(mStr, tmp);
      } else {
	sprintf(tmp, "%c%d", mt->fromFile + 'a', mt->fromRank + 1 - (gs->ranks > 9));
	strcat(mStr, tmp);
      }
    }
  }
  if ((gs->board[mt->toFile][mt->toRank] != NOPIECE) ||
      ((piece == PAWN) && (mt->fromFile != mt->toFile))) {
    strcat(mStr, "x");
  }
  }
  sprintf(tmp, "%c%d", mt->toFile + 'a', mt->toRank + 1 - (gs->ranks > 9));
  strcat(mStr, tmp);
suffix:
  if ((piece == PAWN || piece == HOPLITE || gs->promoType == 3 || gs->drops == 2) && (mt->piecePromotionTo != NOPIECE)) {
    strcat(mStr, "=");		/* = before promoting piece */
    switch (piecetype(abs(mt->piecePromotionTo))) {
    case KING:
      strcat(mStr, "K");
      break;
    case KNIGHT:
      strcat(mStr, "N");
      break;
    case BISHOP:
      strcat(mStr, "B");
      break;
    case ROOK:
      strcat(mStr, "R");
      break;
    case CARDINAL:
      strcat(mStr, "A");
      break;
    case MARSHALL:
    case CAPTAIN:
      strcat(mStr, "C");
      break;
    case MAN:
      strcat(mStr, "M");
      break;
    case QUEEN:
      strcat(mStr, "Q");
      break;
    case FERZ2:
      strcat(mStr, "F");
      break;
    case WARLORD:
    case WOODY:
      strcat(mStr, "W");
      break;
    case SELEPHANT:
    case EMPRESS:
      strcat(mStr, "E");
      break;
    case CENTAUR:
      strcat(mStr, "V");
      break;
    case PRINCESS:
      strcat(mStr, "S");
      break;
    case SQUIRREL:
      strcat(mStr, "O");
      break;
    case MASTODON:
    case GENERAL:
    case GOLD: // [HGM] Shogi promotions: avoid use of '+'
      strcat(mStr, "G");
      break;
    case HAWK:
    case DRAGONHORSE:
      strcat(mStr, "H");
      break;
    case DRAGONKING:
      strcat(mStr, "D");
      break;
    case LIEUTENANT:
      strcat(mStr, "L");
      break;
    default:
      break;
    }
  }
check:;
  fakeMove = *gs;
  execute_move(&fakeMove, mt, 0);
  fakeMove.onMove = CToggle(fakeMove.onMove);
  if (in_check(&fakeMove)) {
    strcat(mStr, "+");
  }
  return mStr;
}
