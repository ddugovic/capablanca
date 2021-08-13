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

// [HGM] some explanation of the parser code:
// The routine parse_move() calls is_move() to recognize some simple cases,
// like OO-castle and long algebraic with or without dash. What does not fall
// in this class is handed to alg_is_move(), whch is really a continuation
// of is_move(), to recognize SAN.
// Depending on the type of move syntax, parse_move can extract from- and to-square
// immediately, or transate the OO-castling to internal from-to representation.
// Only for SAN syntax the routine alg_pars_move() is called to extract the
// given elements, (through get_move_info(), which is the same as alg_is_move(),
// xcept that it does not discard the value of the elements), and disambiguate 
// the move (i.e. determines the from-square) by looking for a piece of the given
// type on the board for which the move is pseudo-legal (using legal_move() ).
// Th

/* Simply tests if the input string is a move or not. */
/* If it matches patterns below */
/* Add to this list as you improve the move parser */
/* MS_COMP e2e4 */
/* MS_COMPDASH e2-e4 */
/* MS_CASTLE o-o, o-o-o */
/* Not done yet */
/* MS_ALG e4, Nd5 Ncd5 */
int is_move(const char *mstr)
{
  int len = strlen(mstr);
  /* remove the 'mates' marker */
  if (mstr[len - 1] == '#')
    len--;

  if ((len > 3) && (mstr[len - 2] == '=' || mstr[len - 2] == '/'))
    len -= 2;

  if (len == 4) {		/* Test for e2e4 */
    if (isfile(mstr[0]) && isrank(mstr[1]) &&
	isfile(mstr[2]) && isrank(mstr[3])) {
      return MS_COMP;
    }
  }
  if (len == 5) {		/* Test for e2-e4 */
    if (isfile(mstr[0]) && isrank(mstr[1]) &&
	(mstr[2] == '-') &&
	isfile(mstr[3]) && isrank(mstr[4])) {
      return MS_COMPDASH;
    }
  }
  if (len == 3) {		/* Test for o-o */
    if ((mstr[0] == 'o') && (mstr[1] == '-') && (mstr[2] == 'o')) {
      return MS_KCASTLE;
    }
    if ((mstr[0] == 'O') && (mstr[1] == '-') && (mstr[2] == 'O')) {
      return MS_KCASTLE;
    }
    if ((mstr[0] == '0') && (mstr[1] == '-') && (mstr[2] == '0')) {
      return MS_KCASTLE;
    }
  }
  if (len == 2) {		/* Test for oo */
    if ((mstr[0] == 'o') && (mstr[1] == 'o')) {
      return MS_KCASTLE;
    }
    if ((mstr[0] == 'O') && (mstr[1] == 'O')) {
      return MS_KCASTLE;
    }
    if ((mstr[0] == '0') && (mstr[1] == '0')) {
      return MS_KCASTLE;
    }
  }
  if (len == 5) {		/* Test for o-o-o */
    if ((mstr[0] == 'o') && (mstr[1] == '-') && (mstr[2] == 'o') && (mstr[3] == '-') && (mstr[4] == 'o')) {
      return MS_QCASTLE;
    }
    if ((mstr[0] == 'O') && (mstr[1] == '-') && (mstr[2] == 'O') && (mstr[3] == '-') && (mstr[4] == 'O')) {
      return MS_QCASTLE;
    }
    if ((mstr[0] == '0') && (mstr[1] == '-') && (mstr[2] == '0') && (mstr[3] == '-') && (mstr[4] == '0')) {
      return MS_QCASTLE;
    }
  }
  if (len == 3) {		/* Test for ooo */
    if ((mstr[0] == 'o') && (mstr[1] == 'o') && (mstr[2] == 'o')) {
      return MS_QCASTLE;
    }
    if ((mstr[0] == 'O') && (mstr[1] == 'O') && (mstr[2] == 'O')) {
      return MS_QCASTLE;
    }
    if ((mstr[0] == '0') && (mstr[1] == '0') && (mstr[2] == '0')) {
      return MS_QCASTLE;
    }
  }
  return alg_is_move(mstr);
}


int NextPieceLoop(board_t b, int *f, int *r, int color, int w, int h)
{
  for (;;) {
    (*r) = (*r) + 1;
    if (*r >= h) {
      *r = 0;
      *f = *f + 1;
      if (*f >= w)
	break;
    }
    if ((b[*f][*r] != NOPIECE) && iscolor(b[*f][*r], color))
      return 1;
  }
  return 0;
}

int InitPieceLoop(board_t b, int *f, int *r, int color)
{
  *f = 0;
  *r = -1;
  return 1;
}

/* All of the routines assume that the obvious problems have been checked */
/* See legal_move() */
static int legal_pawn_move( struct game_state_t *gs, int ff, int fr, int tf, int tr )
{
  if (ff == tf) {
    if (gs->board[tf][tr] != NOPIECE && !gs->palace && gs->promoType != 3) return 0; // [HGM] XQ and Shogi pawns can capture straight ahead
    if (gs->onMove == WHITE) {
      if (tr - fr == 1) return 1;
      if ((fr <= gs->pawnDblStep) && (tr - fr == 2) && gs->board[ff][fr+1]==NOPIECE) return 1;
    } else {
      if (fr - tr == 1) return 1;
      if ((fr >= gs->ranks - 1 - gs->pawnDblStep) && (fr - tr == 2) && gs->board[ff][fr-1]==NOPIECE) return 1;
    }
    return 0;
  }
  if (ff != tf && gs->promoType != 3) { /* Capture ? ([HGM] but not in Shogi) */
    if ((ff - tf != 1) && (tf - ff != 1)) return 0;
    if (gs->onMove == WHITE) {
      if(gs->palace) return (fr >= gs->ranks/2 && fr == tr); // [HGM] XQ promoted pawns
      if (tr != fr+1) return 0;
      if ((gs->board[tf][tr] != NOPIECE) && iscolor(gs->board[tf][tr],BLACK))
        return 1;
      if (gs->ep_possible[0][ff] == 1) {
        if ((tf==ff+1) && (gs->board[ff+1][fr] == B_PAWN)) return 1;
      } else if (gs->ep_possible[0][ff] == -1) {
        if ((tf==ff-1) && (gs->board[ff-1][fr] == B_PAWN)) return 1;
      }
    } else {
      if(gs->palace) return (fr < gs->ranks/2 && fr == tr); // [HGM] XQ promoted pawns
      if (tr != fr-1) return 0;
      if ((gs->board[tf][tr] != NOPIECE) && iscolor(gs->board[tf][tr],WHITE))
        return 1;
      if (gs->ep_possible[1][ff] == 1) {
        if ((tf==ff+1) && (gs->board[ff+1][fr] == W_PAWN)) return 1;
      } else if (gs->ep_possible[1][ff] == -1) {
        if ((tf==ff-1) && (gs->board[ff-1][fr] == W_PAWN)) return 1;
      }
    }
  }
  return 0;
}

static int legal_hoplite_move( struct game_state_t *gs, int ff, int fr, int tf, int tr )
{
  if (ff == tf) { /* Capture ? */
    if (gs->board[tf][tr] == NOPIECE) return 0;
    if (gs->onMove == WHITE) {
      if(iscolor(gs->board[tf][tr],WHITE)) return 0;
      if (tr - fr == 1) return 1;
    } else {
      if(iscolor(gs->board[tf][tr],BLACK)) return 0;
      if (fr - tr == 1) return 1;
    }
    return 0;
  }
  if (ff != tf) {
    if (gs->board[tf][tr] != NOPIECE) return 0;
    if (gs->onMove == WHITE) {
      if (tr == fr+1 && abs(tf-ff) == 1) return 1;
      if (tr != fr+2 || abs(tf-ff) != 2) return 0;
      if (fr > gs->pawnDblStep) return 0;
        return 1;
    } else {
      if (tr == fr-1 && abs(tf-ff) == 1) return 1;
      if (tr != fr-2 || abs(tf-ff) != 2) return 0;
      if (fr < gs->ranks - 1 - gs->pawnDblStep) return 0;
        return 1;
    }
  }
  return 0;
}

static int legal_knight_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  int dx, dy;

  dx = ff - tf;
  dy = fr - tr;
  if (abs(dx) == 2) {
    if (abs(dy) == 1)
      return 1;
  }
  if (abs(dy) == 2) {
    if (abs(dx) == 1)
      return 1;
  }
  return 0;
}

static int legal_horse_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  int dx, dy;

  dx = ff - tf;
  dy = fr - tr;
  if (abs(dx) == 2) {
    if (abs(dy) == 1 && gs->board[(ff+tf)/2][fr] == NOPIECE)
      return 1;
  }
  if (abs(dy) == 2) {
    if (abs(dx) == 1 && gs->board[ff][(fr+tr)/2] == NOPIECE)
      return 1;
  }
  return 0;
}

static int legal_honorablehorse_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  int dx, dy;

  dx = ff - tf;
  dy = fr - tr;
  if (dy == (gs->onMove == WHITE ? -2 : 2)) {
    if (abs(dx) == 1)
      return 1;
  }
  return 0;
}

static int legal_bishop_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  int dx, dy, x, y;
  int startx, starty;
  int count;
  int incx, incy;

  if (ff > tf) {
    dx = ff - tf;
    incx = -1;
  } else {
    dx = tf - ff;
    incx = 1;
  }
  startx = ff + incx;
  if (fr > tr) {
    dy = fr - tr;
    incy = -1;
  } else {
    dy = tr - fr;
    incy = 1;
  }
  starty = fr + incy;
  if (dx != dy)
    return 0;			/* Not diagonal */
  if (dx == 1)
    return 1;			/* One square, ok */
  count = dx - 1;
  for (x = startx, y = starty; count; x += incx, y += incy, count--) {
    if (gs->board[x][y] != NOPIECE)
      return 0;
  }
  return 1;
}

static int legal_rook_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  int i;
  int start, stop;

  if (ff == tf) {
    if (abs(fr - tr) == 1)
      return 1;
    if (fr < tr) {
      start = fr + 1;
      stop = tr - 1;
    } else {
      start = tr + 1;
      stop = fr - 1;
    }
    for (i = start; i <= stop; i++) {
      if (gs->board[ff][i] != NOPIECE)
	return 0;
    }
    return 1;
  } else if (fr == tr) {
    if (abs(ff - tf) == 1)
      return 1;
    if (ff < tf) {
      start = ff + 1;
      stop = tf - 1;
    } else {
      start = tf + 1;
      stop = ff - 1;
    }
    for (i = start; i <= stop; i++) {
      if (gs->board[i][fr] != NOPIECE)
	return 0;
    }
    return 1;
  } else {
    return 0;
  }
}

static int legal_cannon_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  int i, cnt=0;
  int start, stop;

  if (ff == tf) {
    if (fr < tr) {
      start = fr + 1;
      stop = tr - 1;
    } else {
      start = tr + 1;
      stop = fr - 1;
    }
    for (i = start; i <= stop; i++) {
      if (gs->board[ff][i] != NOPIECE) cnt++;
    }
    return (cnt == 0 && gs->board[tf][tr] == NOPIECE) ||
	   (cnt == 1 && gs->board[tf][tr] != NOPIECE);
  } else if (fr == tr) {
    if (ff < tf) {
      start = ff + 1;
      stop = tf - 1;
    } else {
      start = tf + 1;
      stop = ff - 1;
    }
    for (i = start; i <= stop; i++) {
      if (gs->board[i][fr] != NOPIECE) cnt++;
    }
    return (cnt == 0 && gs->board[tf][tr] == NOPIECE) ||
	   (cnt == 1 && gs->board[tf][tr] != NOPIECE);
  } else {
    return 0;
  }
}

static int legal_lance_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  int i;
  int start, stop;

  if (ff != tf)
    return 0;
  if (gs->onMove == WHITE ? (fr >= tr) : (fr <= tr))
    return 0;

  if (abs(fr - tr) == 1)
    return 1;
  if (gs->onMove == WHITE) {
    start = fr + 1;
    stop = tr - 1;
  } else {
    start = tr - 1;
    stop = fr + 1;
  }
  for (i = start; i <= stop; i++) {
    if (gs->board[ff][i] != NOPIECE)
      return 0;
  }
  return 1;
}

static int legal_queen_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_rook_move(gs, ff, fr, tf, tr) || legal_bishop_move(gs, ff, fr, tf, tr);
}

static int legal_cardinal_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_knight_move(gs, ff, fr, tf, tr) || legal_bishop_move(gs, ff, fr, tf, tr);
}

static int legal_marshall_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_rook_move(gs, ff, fr, tf, tr) || legal_knight_move(gs, ff, fr, tf, tr);
}

/* Ckeck, if square (kf,kr) is attacked by enemy piece.
 * Used in castling from/through check testing.
 */

/* new one from soso: */
static int is_square_attacked (struct game_state_t *gs, int kf, int kr)
{
  struct game_state_t fakeMove;
  int oldk = gs->onMove == WHITE ? gs->wkmoved : gs->bkmoved;

  fakeMove = *gs;
  fakeMove.board[oldk][kr] = NOPIECE; // [HGM] castle: this routine is called only when King has not moved
  fakeMove.board[kf][kr] = KING | fakeMove.onMove;
  fakeMove.onMove = CToggle (fakeMove.onMove);
  if (in_check(&fakeMove)) return 1;
    else return 0;
}

/* old one:
static int is_square_attacked(struct game_state_t * gs, int kf, int kr)
{
  int f, r;
  gs->onMove = CToggle(gs->onMove);

  for (InitPieceLoop(gs->board, &f, &r, gs->onMove);
       NextPieceLoop(gs->board, &f, &r, gs->onMove, gs->files, gs->ranks);) {
    if (legal_move(gs, f, r, kf, kr)) {
      gs->onMove = CToggle(gs->onMove);
      return 1;
    }
  }
  gs->onMove = CToggle(gs->onMove);
  return 0;
}
*/

static int legal_man_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  if (abs(ff - tf) > 1)
    return 0;
  if (abs(fr - tr) > 1)
    return 0;
  return 1;
}

static int legal_wazir_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  if (gs->palace && ((tr > gs->palace && tr < gs->ranks - gs->palace) ||
      tf < (gs->files - gs->palace)/2 || tf >= (gs->files + gs->palace)/2))
    return 0;
  if (abs(ff - tf) == 1 && fr == tr)
    return 1;
  if (abs(fr - tr) == 1 && ff == tf)
    return 1;
  return 0;
}

static int legal_dababba_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  if (abs(ff - tf) == 2 && fr == tr)
    return 1;
  if (abs(fr - tr) == 2 && ff == tf)
    return 1;
  return 0;
}

static int legal_ferz_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  if (abs(ff - tf) != 1)
    return 0;
  if (abs(fr - tr) != 1)
    return 0;
  return 1;
}

static int legal_mandarin_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  if (gs->palace && ((tr > gs->palace && tr < gs->ranks - gs->palace) ||
      tf < (gs->files - gs->palace)/2 || tf >= (gs->files + gs->palace)/2))
    return 0;
  if (abs(ff - tf) != 1)
    return 0;
  if (abs(fr - tr) != 1)
    return 0;
  return 1;
}

static int legal_alfil_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  if (abs(ff - tf) != 2)
    return 0;
  if (abs(fr - tr) != 2)
    return 0;
  return 1;
}

static int legal_elephant_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  if (abs(ff - tf) != 2)
    return 0;
  if (abs(fr - tr) != 2)
    return 0;
  if(gs->board[(ff+tf)/2][(fr+tr)/2] != NOPIECE) return 0; // blocked
  if((tr >= gs->ranks/2) != (fr >= gs->ranks/2)) return 0; // do not cross river
  return 1;
}

static int legal_gold_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_wazir_move(gs, ff, fr, tf, tr) || (abs(ff-tf) == 1 && tr == fr + (gs->onMove==WHITE ? 1 : -1));
}

static int legal_silver_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_ferz_move(gs, ff, fr, tf, tr) || (tf == ff && tr == fr + (gs->onMove==WHITE ? 1 : -1) );
}

static int legal_woody_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_wazir_move(gs, ff, fr, tf, tr) || legal_dababba_move(gs, ff, fr, tf, tr);
}

static int legal_squirrel_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_alfil_move(gs, ff, fr, tf, tr) || legal_dababba_move(gs, ff, fr, tf, tr) 
							|| legal_knight_move(gs, ff, fr, tf, tr);
}

static int legal_mastodon_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_man_move(gs, ff, fr, tf, tr) || legal_alfil_move(gs, ff, fr, tf, tr)
							|| legal_dababba_move(gs, ff, fr, tf, tr);
}

static int legal_centaur_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_man_move(gs, ff, fr, tf, tr) || legal_knight_move(gs, ff, fr, tf, tr);
}

static int legal_amazon_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_queen_move(gs, ff, fr, tf, tr) || legal_knight_move(gs, ff, fr, tf, tr);
}

static int legal_dragonking_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_rook_move(gs, ff, fr, tf, tr) || legal_ferz_move(gs, ff, fr, tf, tr);
}

static int legal_dragonhorse_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_bishop_move(gs, ff, fr, tf, tr) || legal_wazir_move(gs, ff, fr, tf, tr);
}

static int legal_modernelephant_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_ferz_move(gs, ff, fr, tf, tr) || legal_alfil_move(gs, ff, fr, tf, tr);
}

static int legal_lieutenant_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_modernelephant_move(gs, ff, fr, tf, tr) ||
	 (fr == tr && abs(ff - tf) == 1 && gs->board[tf][tr] == NOPIECE);
}

static int legal_priestess_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_knight_move(gs, ff, fr, tf, tr) || legal_ferz_move(gs, ff, fr, tf, tr)
						|| legal_alfil_move(gs, ff, fr, tf, tr);
}

static int legal_minister_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_knight_move(gs, ff, fr, tf, tr) || legal_wazir_move(gs, ff, fr, tf, tr)
						|| legal_dababba_move(gs, ff, fr, tf, tr);
}

static int legal_king_move(struct game_state_t * gs, int ff, int fr, int tf, int tr)
{
  int result;

  // [HGM] castle: test first if valid as regular King move; result = 1 or 0
  if(gs->royalKnight)
    result = legal_knight_move(gs, ff, fr, tf, tr);
  else if(gs->palace) {
    result = legal_wazir_move(gs, ff, fr, tf, tr);
    if(!result && ff == tf && piecetype(gs->board[tf][tr]) == KING) { // XQ regicide
      int i, d = (tr>fr ? 1 : -1);
      for(i=fr+d; i!=tr; i+=d) 
	if(gs->board[ff][i] != NOPIECE) return 0; // line of sight blocked
      return 1;
    }
  } else
    result = legal_man_move(gs, ff, fr, tf, tr);

  if(result) return 1;
  // [HGM] castle: orthodox legal castlings given as King move return 2

  if (gs->onMove == WHITE) {
    /* King side castling */
    if ((fr == 0) && (tr == 0) && ((ff == gs->files/2) && (tf == gs->files-2) ||
		 gs->drops == 2 && (tf == gs->files/2) && (ff == gs->files-1)) // [HGM] reverse Seirawan gating
	&& (gs->wkmoved >= 0) && (gs->wkrmoved >= 0) && (gs->board[gs->files-3][0] == NOPIECE) &&
	(gs->board[gs->files-2][0] == NOPIECE) && (gs->board[gs->files-1][0] == W_ROOK) &&
	(gs->board[gs->files/2+1][0] == NOPIECE) && (!is_square_attacked(gs, gs->files/2+1, 0)) &&
	(!is_square_attacked(gs, gs->files/2, 0)) && (!is_square_attacked(gs, gs->files-3, 0))) {
      return 2;
    }
    /* Queen side castling */
    if ((fr == 0) && (tr == 0) && ((ff == gs->files/2) && (tf == 2) ||
		 gs->drops == 2 && (tf == gs->files/2) && (ff == 0)) // [HGM] reverse Seirawan gating
	&& (gs->wkmoved >= 0) && (gs->wqrmoved >= 0) && (gs->board[3][0] == NOPIECE) &&
	(gs->board[2][0] == NOPIECE) && (gs->board[1][0] == NOPIECE) &&
	(gs->board[0][0] == W_ROOK) &&
	(gs->board[gs->files/2-1][0] == NOPIECE) && (!is_square_attacked(gs, gs->files/2-1, 0)) &&
	(!is_square_attacked(gs, gs->files/2, 0)) && (!is_square_attacked(gs, 3, 0))) {
      return 2;
    }
  } else {			/* Black */
    /* King side castling */
    if ((fr == gs->ranks-1) && (tr == gs->ranks-1) && ((ff == gs->files/2) && (tf == gs->files-2) ||
				     gs->drops == 2 && (tf == gs->files/2) && (ff == gs->files-1)) // [HGM] reverse Seirawan gating
	&& (gs->bkmoved >= 0) && (gs->bkrmoved >= 0) && (gs->board[gs->files-3][7] == NOPIECE) &&
	(gs->board[gs->files-2][gs->ranks-1] == NOPIECE) && (gs->board[gs->files-1][gs->ranks-1] == B_ROOK) &&
	(gs->board[gs->files/2+1][gs->ranks-1] == NOPIECE) && (!is_square_attacked(gs, gs->files/2+1, gs->ranks-1)) &&
	(!is_square_attacked(gs, gs->files/2, gs->ranks-1)) && (!is_square_attacked(gs, gs->files-3, gs->ranks-1))) {
      return 2;
    }
    /* Queen side castling */
    if ((fr == gs->ranks-1) && (tr == gs->ranks-1) && ((ff == gs->files/2) && (tf == 2) ||
				     gs->drops == 2 && (tf == gs->files/2) && (ff == 0)) // [HGM] reverse Seirawan gating
	&& (gs->bkmoved >= 0) && (gs->bqrmoved >= 0) && (gs->board[3][gs->ranks-1] == NOPIECE) &&
	(gs->board[2][gs->ranks-1] == NOPIECE) && (gs->board[1][gs->ranks-1] == NOPIECE) &&
	(gs->board[0][gs->ranks-1] == B_ROOK) &&
	(gs->board[gs->files/2-1][gs->ranks-1] == NOPIECE) && (!is_square_attacked(gs, gs->files/2-1, gs->ranks-1)) &&
	(!is_square_attacked(gs, gs->files/2, gs->ranks-1)) && (!is_square_attacked(gs, 3, gs->ranks-1))) {
      return 2;
    }
  }

  return 0; // neither regular King move nor castling
}

static void add_pos(int tof, int tor, int *posf, int *posr, int *numpos)
{
  posf[*numpos] = tof;
  posr[*numpos] = tor;
  (*numpos)++;
}

static void possible_pawn_moves(struct game_state_t * gs,
				  int onf, int onr,
				  int *posf, int *posr, int *numpos)
{
  if (gs->onMove == WHITE) {
    if (gs->board[onf][onr + 1] == NOPIECE || gs->palace || gs->promoType == 3) {
      add_pos(onf, onr + 1, posf, posr, numpos);
      if ((onr <= gs->pawnDblStep) && (gs->board[onf][onr + 2] == NOPIECE))
	add_pos(onf, onr + 2, posf, posr, numpos);
    }
    if (onf > 0) {
      if (gs->board[onf - 1][onr + 1] != NOPIECE &&
	  iscolor(gs->board[onf - 1][onr + 1], BLACK) &&
          !gs->palace && gs->promoType != 3) // no diagonal capture in XQ and Shogi
        add_pos(onf - 1, onr + 1, posf, posr, numpos);
      if(gs->palace && onr >= gs->ranks/2 && (gs->board[onf-1][onr] || iscolor(gs->board[onf-1][onr], BLACK)))
        add_pos(onf - 1, onr, posf, posr, numpos); // XQ promoted pawn
    }
    if (onf < gs->files-1) {
      if (gs->board[onf + 1][onr + 1] != NOPIECE &&
	  iscolor(gs->board[onf + 1][onr + 1], BLACK) &&
          !gs->palace && gs->promoType != 3) // no diagonal capture in XQ and Shogi
	add_pos(onf + 1, onr + 1, posf, posr, numpos);
      if(gs->palace && onr >= gs->ranks/2 && (gs->board[onf+1][onr] || iscolor(gs->board[onf+1][onr], BLACK)))
        add_pos(onf + 1, onr, posf, posr, numpos); // XQ promoted pawn
    }
    if (gs->ep_possible[0][onf] == -1)
      add_pos(onf - 1, onr + 1, posf, posr, numpos);
    if (gs->ep_possible[0][onf] == 1)
      add_pos(onf + 1, onr + 1, posf, posr, numpos);
  } else {
    if (gs->board[onf][onr - 1] == NOPIECE || gs->palace || gs->promoType == 3) {
      add_pos(onf, onr - 1, posf, posr, numpos);
      if ((onr >= gs->ranks - gs->pawnDblStep - 1) && (gs->board[onf][onr - 2] == NOPIECE))
	add_pos(onf, onr - 2, posf, posr, numpos);
    }
    if (onf > 0) {
      if (gs->board[onf - 1][onr - 1] != NOPIECE &&
	  iscolor(gs->board[onf - 1][onr - 1], WHITE) &&
          !gs->palace && gs->promoType != 3) // no diagonal capture in XQ and Shogi
	add_pos(onf - 1, onr - 1, posf, posr, numpos);
      if(gs->palace && onr < gs->ranks/2 && !iscolor(gs->board[onf-1][onr], BLACK))
        add_pos(onf - 1, onr, posf, posr, numpos); // XQ promoted pawn
    }
    if (onf < gs->files-1) {
      if (gs->board[onf + 1][onr - 1] != NOPIECE &&
	  iscolor(gs->board[onf + 1][onr - 1], WHITE) &&
          !gs->palace && gs->promoType != 3) // no diagonal capture in XQ and Shogi
	add_pos(onf + 1, onr - 1, posf, posr, numpos);
      if(gs->palace && onr < gs->ranks/2 && !iscolor(gs->board[onf+1][onr], BLACK))
        add_pos(onf + 1, onr, posf, posr, numpos); // XQ promoted pawn
    }
    if (gs->ep_possible[1][onf] == -1)
      add_pos(onf - 1, onr - 1, posf, posr, numpos);
    if (gs->ep_possible[1][onf] == 1)
      add_pos(onf + 1, onr - 1, posf, posr, numpos);
  }
}

static void possible_hoplite_moves(struct game_state_t * gs,
				  int onf, int onr,
				  int *posf, int *posr, int *numpos)
{
  if (gs->onMove == WHITE) { // in Spartan Chess there are no white hoplites...
    if (gs->board[onf][onr + 1] != NOPIECE &&
	  iscolor(gs->board[onf][onr + 1], BLACK)) {
      add_pos(onf, onr + 1, posf, posr, numpos);
    }
    if (onf > 0) {
      if (gs->board[onf - 1][onr + 1] == NOPIECE) {
        add_pos(onf - 1, onr + 1, posf, posr, numpos);
        if (onf > 1 && (onr <= gs->pawnDblStep) /*&& (gs->board[onf-2][onr + 2] == NOPIECE)*/)
	  add_pos(onf - 2, onr + 2, posf, posr, numpos);
      }
    }
    if (onf < gs->files-1) {
      if (gs->board[onf + 1][onr + 1] == NOPIECE) {
	add_pos(onf + 1, onr + 1, posf, posr, numpos);
        if (onf < gs->files-2 && (onr <= gs->pawnDblStep) /*&& (gs->board[onf+2][onr + 2] == NOPIECE)*/)
	  add_pos(onf + 2, onr + 2, posf, posr, numpos);
      }
    }
  } else {
    if (gs->board[onf][onr - 1] != NOPIECE &&
	  iscolor(gs->board[onf][onr - 1], WHITE)) {
      add_pos(onf, onr - 1, posf, posr, numpos);
    }
    if (onf > 0) {
      if (gs->board[onf - 1][onr - 1] == NOPIECE) {
	add_pos(onf - 1, onr - 1, posf, posr, numpos);
        if (onf > 1 && (onr >= gs->ranks - gs->pawnDblStep - 1) /*&& (gs->board[onf - 2][onr - 2] == NOPIECE)*/)
	  add_pos(onf - 2, onr - 2, posf, posr, numpos);
      }
    }
    if (onf < gs->files-1) {
      if (gs->board[onf + 1][onr - 1] == NOPIECE) {
	add_pos(onf + 1, onr - 1, posf, posr, numpos);
        if (onf < gs->files-2 && (onr >= gs->ranks - gs->pawnDblStep - 1) /*&& (gs->board[onf + 2][onr - 2] == NOPIECE)*/)
	  add_pos(onf + 2, onr - 2, posf, posr, numpos);
      }
    }
  }
}

static void possible_knight_moves(struct game_state_t * gs,
				    int onf, int onr,
				    int *posf, int *posr, int *numpos)
{
  static int knightJumps[8][2] = {{-1, 2}, {1, 2}, {2, -1}, {2, 1},
  {-1, -2}, {1, -2}, {-2, 1}, {-2, -1}};
  int f, r;
  int j;

  for (j = 0; j < 8; j++) {
    f = knightJumps[j][0] + onf;
    r = knightJumps[j][1] + onr;
    if ((f < 0) || (f >= gs->files))
      continue;
    if ((r < 0) || (r >= gs->ranks))
      continue;
    if ((gs->board[f][r] == NOPIECE) ||
	(iscolor(gs->board[f][r], CToggle(gs->onMove))))
      add_pos(f, r, posf, posr, numpos);
  }
}

static void possible_horse_moves(struct game_state_t * gs,
				    int onf, int onr,
				    int *posf, int *posr, int *numpos)
{
  static int knightJumps[8][4] = {{-1, 2, 0, 1}, {1, 2, 0, 1}, {2, -1, 1, 0}, {2, 1, 1, 0},
  {-1, -2, 0, -1}, {1, -2, 0, -1}, {-2, 1, -1, 0}, {-2, -1, -1, 0}};
  int f, r;
  int j;

  for (j = 0; j < 8; j++) {
    f = knightJumps[j][0] + onf;
    r = knightJumps[j][1] + onr;
    if ((f < 0) || (f >= gs->files))
      continue;
    if ((r < 0) || (r >= gs->ranks))
      continue;
    if ((gs->board[knightJumps[j][2] + onf][knightJumps[j][3] + onr] == NOPIECE) && 
	((gs->board[f][r] == NOPIECE) || (iscolor(gs->board[f][r], CToggle(gs->onMove)))))
      add_pos(f, r, posf, posr, numpos);
  }
}

static void possible_bishop_moves(struct game_state_t * gs,
				    int onf, int onr,
				    int *posf, int *posr, int *numpos)
{
  int f, r;

  /* Up Left */
  f = onf;
  r = onr;
  for (;;) {
    f--;
    r++;
    if ((f < 0) || (f >= gs->files))
      break;
    if ((r < 0) || (r >= gs->ranks))
      break;
    if ((gs->board[f][r] != NOPIECE) && (iscolor(gs->board[f][r], gs->onMove)))
      break;
    add_pos(f, r, posf, posr, numpos);
    if (gs->board[f][r] != NOPIECE)
      break;
  }
  /* Up Right */
  f = onf;
  r = onr;
  for (;;) {
    f++;
    r++;
    if ((f < 0) || (f >= gs->files))
      break;
    if ((r < 0) || (r >= gs->ranks))
      break;
    if ((gs->board[f][r] != NOPIECE) && (iscolor(gs->board[f][r], gs->onMove)))
      break;
    add_pos(f, r, posf, posr, numpos);
    if (gs->board[f][r] != NOPIECE)
      break;
  }
  /* Down Left */
  f = onf;
  r = onr;
  for (;;) {
    f--;
    r--;
    if ((f < 0) || (f >= gs->files))
      break;
    if ((r < 0) || (r >= gs->ranks))
      break;
    if ((gs->board[f][r] != NOPIECE) && (iscolor(gs->board[f][r], gs->onMove)))
      break;
    add_pos(f, r, posf, posr, numpos);
    if (gs->board[f][r] != NOPIECE)
      break;
  }
  /* Down Right */
  f = onf;
  r = onr;
  for (;;) {
    f++;
    r--;
    if ((f < 0) || (f >= gs->files))
      break;
    if ((r < 0) || (r >= gs->ranks))
      break;
    if ((gs->board[f][r] != NOPIECE) && (iscolor(gs->board[f][r], gs->onMove)))
      break;
    add_pos(f, r, posf, posr, numpos);
    if (gs->board[f][r] != NOPIECE)
      break;
  }
}

static void possible_rook_moves(struct game_state_t * gs,
				  int onf, int onr,
				  int *posf, int *posr, int *numpos)
{
  int f, r;

  /* Left */
  f = onf;
  r = onr;
  for (;;) {
    f--;
    if ((f < 0) || (f >= gs->files))
      break;
    if ((r < 0) || (r >= gs->ranks))
      break;
    if ((gs->board[f][r] != NOPIECE) && (iscolor(gs->board[f][r], gs->onMove)))
      break;
    add_pos(f, r, posf, posr, numpos);
    if (gs->board[f][r] != NOPIECE)
      break;
  }
  /* Right */
  f = onf;
  r = onr;
  for (;;) {
    f++;
    if ((f < 0) || (f >= gs->files))
      break;
    if ((r < 0) || (r >= gs->ranks))
      break;
    if ((gs->board[f][r] != NOPIECE) && (iscolor(gs->board[f][r], gs->onMove)))
      break;
    add_pos(f, r, posf, posr, numpos);
    if (gs->board[f][r] != NOPIECE)
      break;
  }
  /* Up */
  f = onf;
  r = onr;
  for (;;) {
    r++;
    if ((f < 0) || (f >= gs->files))
      break;
    if ((r < 0) || (r >= gs->ranks))
      break;
    if ((gs->board[f][r] != NOPIECE) && (iscolor(gs->board[f][r], gs->onMove)))
      break;
    add_pos(f, r, posf, posr, numpos);
    if (gs->board[f][r] != NOPIECE)
      break;
  }
  /* Down */
  f = onf;
  r = onr;
  for (;;) {
    r--;
    if ((f < 0) || (f >= gs->files))
      break;
    if ((r < 0) || (r >= gs->ranks))
      break;
    if ((gs->board[f][r] != NOPIECE) && (iscolor(gs->board[f][r], gs->onMove)))
      break;
    add_pos(f, r, posf, posr, numpos);
    if (gs->board[f][r] != NOPIECE)
      break;
  }
}

static void possible_cannon_moves(struct game_state_t * gs,
				  int onf, int onr,
				  int *posf, int *posr, int *numpos)
{
  int f, r, i;

  /* Left */
  f = onf;
  r = onr;
  for (i=0;;) {
    f--;
    if ((f < 0) || (f >= gs->files))
      break;
    if ((r < 0) || (r >= gs->ranks))
      break;
    if ((gs->board[f][r] != NOPIECE) && i++ == 0) continue;
    if(i == 0)
	add_pos(f, r, posf, posr, numpos); // no hop: non-capt
    else if(i == 2 && !iscolor(gs->board[f][r], gs->onMove)) 
	add_pos(f, r, posf, posr, numpos); // hop: capt
    if (gs->board[f][r] != NOPIECE)
      break;
  }
  /* Right */
  f = onf;
  r = onr;
  for (i=0;;) {
    f++;
    if ((f < 0) || (f >= gs->files))
      break;
    if ((r < 0) || (r >= gs->ranks))
      break;
    if ((gs->board[f][r] != NOPIECE) && i++ == 0) continue;
    if(i == 0)
	add_pos(f, r, posf, posr, numpos); // no hop: non-capt
    else if(i == 2 && !iscolor(gs->board[f][r], gs->onMove)) 
	add_pos(f, r, posf, posr, numpos); // hop: capt
    if (gs->board[f][r] != NOPIECE)
      break;
  }
  /* Up */
  f = onf;
  r = onr;
  for (i=0;;) {
    r++;
    if ((f < 0) || (f >= gs->files))
      break;
    if ((r < 0) || (r >= gs->ranks))
      break;
    if ((gs->board[f][r] != NOPIECE) && i++ == 0) continue;
    if(i == 0)
	add_pos(f, r, posf, posr, numpos); // no hop: non-capt
    else if(i == 2 && !iscolor(gs->board[f][r], gs->onMove)) 
	add_pos(f, r, posf, posr, numpos); // hop: capt
    if (gs->board[f][r] != NOPIECE)
      break;
  }
  /* Down */
  f = onf;
  r = onr;
  for (i=0;;) {
    r--;
    if ((f < 0) || (f >= gs->files))
      break;
    if ((r < 0) || (r >= gs->ranks))
      break;
    if ((gs->board[f][r] != NOPIECE) && i++ == 0) continue;
    if(i == 0)
	add_pos(f, r, posf, posr, numpos); // no hop: non-capt
    else if(i == 2 && !iscolor(gs->board[f][r], gs->onMove)) 
	add_pos(f, r, posf, posr, numpos); // hop: capt
    if (gs->board[f][r] != NOPIECE)
      break;
  }
}

static void possible_lance_moves(struct game_state_t * gs,
				  int onf, int onr,
				  int *posf, int *posr, int *numpos)
{
  int f, r;

  /* Up */
  f = onf;
  r = onr;
  for (;gs->onMove == WHITE;) {
    r++;
    if ((f < 0) || (f >= gs->files))
      break;
    if ((r < 0) || (r >= gs->ranks))
      break;
    if ((gs->board[f][r] != NOPIECE) && (iscolor(gs->board[f][r], gs->onMove)))
      break;
    add_pos(f, r, posf, posr, numpos);
    if (gs->board[f][r] != NOPIECE)
      break;
  }
  /* Down */
  f = onf;
  r = onr;
  for (;gs->onMove == BLACK;) {
    r--;
    if ((f < 0) || (f >= gs->files))
      break;
    if ((r < 0) || (r >= gs->ranks))
      break;
    if ((gs->board[f][r] != NOPIECE) && (iscolor(gs->board[f][r], gs->onMove)))
      break;
    add_pos(f, r, posf, posr, numpos);
    if (gs->board[f][r] != NOPIECE)
      break;
  }
}

static void possible_cardinal_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_knight_moves(gs, onf, onr, posf, posr, numpos);
  possible_bishop_moves(gs, onf, onr, posf, posr, numpos);
}

static void possible_marshall_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_rook_moves(gs, onf, onr, posf, posr, numpos);
  possible_knight_moves(gs, onf, onr, posf, posr, numpos);
}

static void possible_queen_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_rook_moves(gs, onf, onr, posf, posr, numpos);
  possible_bishop_moves(gs, onf, onr, posf, posr, numpos);
}

static void possible_alfil_moves(struct game_state_t * gs,
				  int onf, int onr,
				  int *posf, int *posr, int *numpos)
{
  static int kingJumps[4][2] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
  int f, r;
  int j;

  for (j = 0; j < 4; j++) {
    f = 2*kingJumps[j][0] + onf;
    r = 2*kingJumps[j][1] + onr;
    if ((f < 0) || (f >= gs->files))
      continue;
    if ((r < 0) || (r >= gs->ranks))
      continue;
    if ((gs->board[f][r] == NOPIECE) ||
	(iscolor(gs->board[f][r], CToggle(gs->onMove))))
      add_pos(f, r, posf, posr, numpos);
  }
}

static void possible_ferz_moves(struct game_state_t * gs,
				  int onf, int onr,
				  int *posf, int *posr, int *numpos)
{
  static int kingJumps[4][2] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
  int f, r;
  int j;

  for (j = 0; j < 4; j++) {
    f = kingJumps[j][0] + onf;
    r = kingJumps[j][1] + onr;
    if ((f < 0) || (f >= gs->files))
      continue;
    if ((r < 0) || (r >= gs->ranks))
      continue;
    if ((gs->board[f][r] == NOPIECE) ||
	(iscolor(gs->board[f][r], CToggle(gs->onMove))))
      add_pos(f, r, posf, posr, numpos);
  }
}

static void possible_mandarin_moves(struct game_state_t * gs,
				  int onf, int onr,
				  int *posf, int *posr, int *numpos)
{
  static int kingJumps[4][2] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
  int f, r;
  int j;

  for (j = 0; j < 4; j++) {
    f = kingJumps[j][0] + onf;
    r = kingJumps[j][1] + onr;
    if ((f < 0) || (f >= gs->files))
      continue;
    if ((r < 0) || (r >= gs->ranks))
      continue;
    if (gs->palace && ((r > gs->palace && r < gs->ranks - gs->palace) ||
        f < (gs->files - gs->palace)/2 || f >= (gs->files + gs->palace)/2))
      continue;
    if ((gs->board[f][r] == NOPIECE) ||
	(iscolor(gs->board[f][r], CToggle(gs->onMove))))
      add_pos(f, r, posf, posr, numpos);
  }
}

static void possible_wazir_moves(struct game_state_t * gs,
				  int onf, int onr,
				  int *posf, int *posr, int *numpos)
{
  static int kingJumps[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
  int f, r;
  int j;

  for (j = 0; j < 4; j++) {
    f = kingJumps[j][0] + onf;
    r = kingJumps[j][1] + onr;
    if ((f < 0) || (f >= gs->files))
      continue;
    if ((r < 0) || (r >= gs->ranks))
      continue;
    if (gs->palace && ((r > gs->palace && r < gs->ranks - gs->palace) ||
        f < (gs->files - gs->palace)/2 || f >= (gs->files + gs->palace)/2))
      continue;
    if ((gs->board[f][r] == NOPIECE) ||
	(iscolor(gs->board[f][r], CToggle(gs->onMove))))
      add_pos(f, r, posf, posr, numpos);
  }
}

static void possible_dababba_moves(struct game_state_t * gs,
				  int onf, int onr,
				  int *posf, int *posr, int *numpos)
{
  static int kingJumps[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
  int f, r;
  int j;

  for (j = 0; j < 4; j++) {
    f = 2*kingJumps[j][0] + onf;
    r = 2*kingJumps[j][1] + onr;
    if ((f < 0) || (f >= gs->files))
      continue;
    if ((r < 0) || (r >= gs->ranks))
      continue;
    if ((gs->board[f][r] == NOPIECE) ||
	(iscolor(gs->board[f][r], CToggle(gs->onMove))))
      add_pos(f, r, posf, posr, numpos);
  }
}

static void possible_man_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_wazir_moves(gs, onf, onr, posf, posr, numpos);
  possible_ferz_moves(gs, onf, onr, posf, posr, numpos);
}

static void possible_dragonking_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_rook_moves(gs, onf, onr, posf, posr, numpos);
  possible_ferz_moves(gs, onf, onr, posf, posr, numpos);
}

static void possible_dragonhorse_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_wazir_moves(gs, onf, onr, posf, posr, numpos);
  possible_bishop_moves(gs, onf, onr, posf, posr, numpos);
}

static void possible_centaur_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_man_moves(gs, onf, onr, posf, posr, numpos);
  possible_knight_moves(gs, onf, onr, posf, posr, numpos);
}

static void possible_woody_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_wazir_moves(gs, onf, onr, posf, posr, numpos);
  possible_dababba_moves(gs, onf, onr, posf, posr, numpos);
}

static void possible_squirrel_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_alfil_moves(gs, onf, onr, posf, posr, numpos);
  possible_dababba_moves(gs, onf, onr, posf, posr, numpos);
  possible_knight_moves(gs, onf, onr, posf, posr, numpos);
}

static void possible_mastodon_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_man_moves(gs, onf, onr, posf, posr, numpos);
  possible_alfil_moves(gs, onf, onr, posf, posr, numpos);
  possible_dababba_moves(gs, onf, onr, posf, posr, numpos);
}

static void possible_amazon_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_queen_moves(gs, onf, onr, posf, posr, numpos);
  possible_knight_moves(gs, onf, onr, posf, posr, numpos);
}

static void possible_modernelephant_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_ferz_moves(gs, onf, onr, posf, posr, numpos);
  possible_alfil_moves(gs, onf, onr, posf, posr, numpos);
}

static void possible_lieutenant_moves(struct game_state_t * gs,
				  int onf, int onr,
				  int *posf, int *posr, int *numpos)
{
  possible_modernelephant_moves(gs, onf, onr, posf, posr, numpos);
  if (onf < gs->files-1 && (gs->board[onf+1][onr] == NOPIECE))
    add_pos(onf + 1, onr, posf, posr, numpos);
  if (onf > 0 && (gs->board[onf-1][onr] == NOPIECE))
    add_pos(onf - 1, onr, posf, posr, numpos);
}

static void possible_priestess_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_ferz_moves(gs, onf, onr, posf, posr, numpos);
  possible_alfil_moves(gs, onf, onr, posf, posr, numpos);
  possible_knight_moves(gs, onf, onr, posf, posr, numpos);
}

static void possible_minister_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_wazir_moves(gs, onf, onr, posf, posr, numpos);
  possible_dababba_moves(gs, onf, onr, posf, posr, numpos);
  possible_knight_moves(gs, onf, onr, posf, posr, numpos);
}

static void possible_gold_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_wazir_moves(gs, onf, onr, posf, posr, numpos);
  if (gs->onMove == WHITE && onr < gs->ranks-1) {
    if (onf > 0 && !iscolor(gs->board[onf-1][onr+1], WHITE))
      add_pos(onf-1, onr+1, posf, posr, numpos);
    if (onf < gs->files-1 && !iscolor(gs->board[onf+1][onr+1], WHITE))
      add_pos(onf+1, onr+1, posf, posr, numpos);
  } else if (gs->onMove == BLACK && onr > 0) {
    if (onf > 0 && !iscolor(gs->board[onf-1][onr-1], BLACK))
      add_pos(onf-1, onr-1, posf, posr, numpos);
    if (onf < gs->files-1 && !iscolor(gs->board[onf+1][onr-1], BLACK))
      add_pos(onf+1, onr-1, posf, posr, numpos);
  }
}

static void possible_silver_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  possible_ferz_moves(gs, onf, onr, posf, posr, numpos);
  if(gs->onMove == WHITE) {
    if(onr < gs->ranks-1 && !iscolor(gs->board[onf][onr+1], WHITE))
      add_pos(onf, onr+1, posf, posr, numpos);
  } else {
    if(onr > 0 && !iscolor(gs->board[onf][onr-1], BLACK))
      add_pos(onf, onr-1, posf, posr, numpos);
  }
}

static void possible_honorablehorse_moves(struct game_state_t * gs,
				  int onf, int onr,
				  int *posf, int *posr, int *numpos)
{
  int r = onr + (gs->onMove == WHITE ? 2 : -2);

  if(r < 0 || r >= gs->ranks) return;
  if(onf > 0) {
    if ((gs->board[onf-1][r] == NOPIECE) ||
	(iscolor(gs->board[onf-1][r], CToggle(gs->onMove))))
      add_pos(onf - 1, r, posf, posr, numpos);
  }
  if(onf < gs->files - 1) {
    if ((gs->board[onf+1][r] == NOPIECE) ||
	(iscolor(gs->board[onf+1][r], CToggle(gs->onMove))))
      add_pos(onf + 1, r, posf, posr, numpos);
  }
}

static void possible_king_moves(struct game_state_t * gs,
				  int onf, int onr,
				  int *posf, int *posr, int *numpos)
{
  if(gs->royalKnight)
    possible_knight_moves(gs, onf, onr, posf, posr, numpos);
  else if(gs->palace)
    possible_wazir_moves(gs, onf, onr, posf, posr, numpos);
  else
    possible_man_moves(gs, onf, onr, posf, posr, numpos);
}

static void possible_elephant_moves(struct game_state_t * gs,
				   int onf, int onr,
				   int *posf, int *posr, int *numpos)
{
  static int kingJumps[4][2] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
  int f, r;
  int j;

  for (j = 0; j < 4; j++) {
    f = 2*kingJumps[j][0] + onf;
    r = 2*kingJumps[j][1] + onr;
    if ((f < 0) || (f >= gs->files))
      continue;
    if ((r < 0) || (r >= gs->ranks))
      continue;
    if ((gs->board[(f+onf)/2][(r+onr)/2] == NOPIECE) && ((gs->board[f][r] == NOPIECE) ||
	(iscolor(gs->board[f][r], CToggle(gs->onMove)))))
      add_pos(f, r, posf, posr, numpos);
  }
}

/* Doesn't check for check */
int legal_move(struct game_state_t * gs,
	       int fFile, int fRank,
	       int tFile, int tRank)
{
  int move_piece, victim;
  int legal;

  if (fFile == ALG_DROP) {
    move_piece = fRank;
    if(gs->drops != 1) return 0; // [HGM] variants: no drops in this variant
    if (move_piece == KING)
      return 0;
    if (gs->holding[gs->onMove==WHITE ? 0 : 1][move_piece-PAWN] == 0)
      return 0;
    if (gs->board[tFile][tRank] != NOPIECE)
      return 0;
    if (gs->promoType == 3) { // Shogi
      int r;
      switch(move_piece) {
	case PAWN:  // check for own Pawn in same file
	  for(r=0; r<gs->ranks; r++) if(gs->board[tFile][r] == (gs->onMove|PAWN)) return 0;
	case LANCE: // Pawns and Lances not on last rank
	  if(gs->onMove == WHITE && tRank >= gs->ranks-1) return 0;
	  if(gs->onMove == BLACK && tRank < 1) return 0;
	  break;
	case HONORABLEHORSE: // Knights at least two ranks from edge
	  if(gs->onMove == WHITE && tRank >= gs->ranks-2) return 0;
	  if(gs->onMove == BLACK && tRank < 2) return 0;
	default: ;
      }
    } else
    if (move_piece == PAWN && (tRank == 0 || tRank == gs->ranks-1))
      return 0;
    return 1;
  } else if(fFile == ALG_CASTLE) {
	// [HGM] castle: this code can handle any type of free castling
	// it does not check if Rook and King from squares correspond to the rights, as the
	// user never enters such squares, but they are copied from the rights on entering o-o-o
	int backRank, kRook, qRook, fKing, leftEmpty, rightEmpty, leftCheck, rightCheck, f;
	if(!gs->castlingStyle) return 0;   // no castling in this variant
	if(gs->onMove == WHITE) {
		if(gs->wkmoved < 0) return 0; // King moved
		fKing = gs->wkmoved;
		backRank = 0;
		kRook = gs->wkrmoved;
		qRook = gs->wqrmoved;
	} else {
		if(gs->bkmoved < 0) return 0; // King moved
		fKing = gs->bkmoved;
		backRank = gs->ranks-1;
		kRook = gs->bkrmoved;
		qRook = gs->bqrmoved;
	}
	if((tRank > tFile ? qRook : kRook) < 0) return 0; // Rook moved
	// here we verified rights do exist, so from squares (fRank and fKing) must be valid
	if(gs->board[fRank][backRank] != (ROOK | gs->onMove) ) return 0; // only with own Rook
	if(gs->board[fKing][backRank] != (KING | gs->onMove) ) return 0; // only with own King

	// by now we know that K and R are in correct position, and still have rights
	if(tRank > tFile) { // R ends right of K: q-side
		leftEmpty  = fRank < tFile ? fRank+1 : tFile+1;
		rightEmpty = tRank < fKing ? fKing-1 : tRank-1;
	} else { // k-side
		leftEmpty  = tRank < fKing ? tRank+1 : fKing+1;
		rightEmpty = fRank < tFile ? fRank-1 : tFile-1;
	}
	for(f=leftEmpty; f<=rightEmpty; f++) // check if other pieces block castling
		if(f != fRank && f != fKing && gs->board[f][backRank] != NOPIECE) return 0;

	leftCheck  = fKing < tFile ? fKing : tFile+1;
	rightCheck = fKing < tFile ? tFile-1 : fKing;
	for(f=leftCheck; f<=rightCheck; f++) // check if King passes attacked square or was in check
		if(is_square_attacked(gs, f, backRank)) return 0;

	return 1; // passed all tests
  } else {
    move_piece = piecetype(gs->board[fFile][fRank]);
  }
  if (gs->board[fFile][fRank] == NOPIECE)
    return 0;
  if (!iscolor(gs->board[fFile][fRank], gs->onMove))	/* Wrong color */
    return 0;
  if (((victim = gs->board[tFile][tRank]) != NOPIECE) &&
      iscolor(gs->board[tFile][tRank], gs->onMove)) {
    if(piecetype(move_piece) == KING && piecetype(victim) == ROOK) { // [HGM] could be FRC castling
    }
    if(gs->drops== 2 && piecetype(move_piece) == ROOK && piecetype(victim) == KING) { // [HGM] could be Seirawan reverse gating
	return legal_king_move(gs, fFile, fRank, tFile, tRank);
    }
    return 0;	/* Can't capture own */
  }
  if ((fFile == tFile) && (fRank == tRank))	/* Same square */
    return 0;
  switch (move_piece) {
  case PAWN:
    legal = legal_pawn_move(gs, fFile, fRank, tFile, tRank);
    break;
  case HOPLITE:
    legal = legal_hoplite_move(gs, fFile, fRank, tFile, tRank);
    break;
  case KNIGHT:
    legal = legal_knight_move(gs, fFile, fRank, tFile, tRank);
    break;
  case BISHOP:
    legal = legal_bishop_move(gs, fFile, fRank, tFile, tRank);
    break;
  case ROOK:
    legal = legal_rook_move(gs, fFile, fRank, tFile, tRank);
    break;
  case WARLORD:
  case HAWK:
  case CARDINAL:
  case PRINCESS:
    legal = legal_cardinal_move(gs, fFile, fRank, tFile, tRank);
    break;
  case SELEPHANT:
  case MARSHALL:
  case EMPRESS:
    legal = legal_marshall_move(gs, fFile, fRank, tFile, tRank);
    break;
  case MAN:
  case MAN2:
    legal = legal_man_move(gs, fFile, fRank, tFile, tRank);
    break;
  case QUEEN:
    legal = legal_queen_move(gs, fFile, fRank, tFile, tRank);
    break;
  case ELEPHANT:
    legal = legal_elephant_move(gs, fFile, fRank, tFile, tRank);
    break;
  case AMAZON:
    legal = legal_amazon_move(gs, fFile, fRank, tFile, tRank);
    break;
  case CAPTAIN:
  case WOODY:
    legal = legal_woody_move(gs, fFile, fRank, tFile, tRank);
    break;
  case SQUIRREL:
    legal = legal_squirrel_move(gs, fFile, fRank, tFile, tRank);
    break;
  case MASTODON:
    legal = legal_mastodon_move(gs, fFile, fRank, tFile, tRank);
    break;
  case CENTAUR:
    legal = legal_centaur_move(gs, fFile, fRank, tFile, tRank);
    break;
  case HORSE:
    legal = legal_horse_move(gs, fFile, fRank, tFile, tRank);
    break;
  case FERZ:
  case FERZ2:
    legal = legal_ferz_move(gs, fFile, fRank, tFile, tRank);
    break;
  case MANDARIN:
    legal = legal_mandarin_move(gs, fFile, fRank, tFile, tRank);
    break;
  case WAZIR:
    legal = legal_wazir_move(gs, fFile, fRank, tFile, tRank);
    break;
  case LIEUTENANT:
    legal = legal_lieutenant_move(gs, fFile, fRank, tFile, tRank);
    break;
  case ALFIL:
  case ALFIL2:
    legal = legal_alfil_move(gs, fFile, fRank, tFile, tRank);
    break;
  case MODERNELEPHANT:
    legal = legal_modernelephant_move(gs, fFile, fRank, tFile, tRank);
    break;
  case PRIESTESS:
    legal = legal_priestess_move(gs, fFile, fRank, tFile, tRank);
    break;
  case MINISTER:
    legal = legal_minister_move(gs, fFile, fRank, tFile, tRank);
    break;
  case SILVER:
    legal = legal_silver_move(gs, fFile, fRank, tFile, tRank);
    break;
  case GOLD:
    legal = legal_gold_move(gs, fFile, fRank, tFile, tRank);
    break;
  case LANCE:
    legal = legal_lance_move(gs, fFile, fRank, tFile, tRank);
    break;
  case CANNON:
    legal = legal_cannon_move(gs, fFile, fRank, tFile, tRank);
    break;
  case DRAGONHORSE:
    legal = legal_dragonhorse_move(gs, fFile, fRank, tFile, tRank);
    break;
  case GENERAL:
  case DRAGONKING:
    legal = legal_dragonking_move(gs, fFile, fRank, tFile, tRank);
    break;
  case HONORABLEHORSE:
    legal = legal_honorablehorse_move(gs, fFile, fRank, tFile, tRank);
    break;
  case KING:
    legal = legal_king_move(gs, fFile, fRank, tFile, tRank);
    break;
  default:
    return 0;
    break;
  }
  return legal;
}

#define DROP_CHAR '@'

/* This fills in the rest of the mt structure once it is determined that
 * the move is legal. Returns MOVE_ILLEGAL if move leaves you in check */
static int move_calculate(struct game_state_t * gs, struct move_t * mt, piece_t promote)
{
  struct game_state_t fakeMove;
  int gating = 0, stm;

#if BUGHOUSE_PAWN_REVERT
  mt->piecePromotionFrom = NOPIECE;
#endif
  mt->pieceCaptured = gs->board[mt->toFile][mt->toRank];
  mt->enPassant = 0;		/* Don't know yet, let execute move take care
				   of it */
  if (mt->fromFile == ALG_DROP) {
    mt->piecePromotionTo = NOPIECE;
    sprintf(mt->moveString, "%s/%c%c-%c%d",
	    wpstring[mt->fromRank],
		DROP_CHAR, DROP_CHAR,
	    mt->toFile + 'a', mt->toRank + 1 - (gs->ranks>9));
  } else if(mt->fromFile == ALG_CASTLE) { 
	// [HGM] castle: generalized castling, fr and tr give from and to file of Rook.
	    sprintf(mt->moveString, mt->toRank > mt->toFile ? "o-o-o" : "o-o");
	if(gs->drops == 2 && promote && gs->holding[gs->onMove == BLACK][abs(promote)-PAWN]) { // promote can be flipped (reverse gating kludge)
	    int c = gs->onMove == WHITE ? 0 : gs->ranks-1;
	    mt->piecePromotionTo = promote; gating = 1;
	    if(promote < 0) sprintf(mt->moveString, "R/%c%d-e%d", mt->fromRank + 'a', c, c); // use RxK notation for Rook-square gatings
	}
  } else {
  stm = colorval(gs->board[mt->fromFile][mt->fromRank]);
  if(gs->promoType == 3) { // Shogi-style promotions: not just Pawns, but many pieces can promote
    piece_t piece = gs->board[mt->fromFile][mt->fromRank];
#if BUGHOUSE_PAWN_REVERT
    mt->piecePromotionFrom = piece;
#endif
    mt->piecePromotionTo = NOPIECE;
    if((colorval(piece) == WHITE && mt->fromRank < gs->ranks - gs->ranks/3
                                 && mt->toRank   < gs->ranks - gs->ranks/3) ||
       (colorval(piece) == BLACK && mt->fromRank >= gs->ranks/3
                                 && mt->toRank   >= gs->ranks/3) )
        promote = NOPIECE; // suppress promotion outside zone
    if(promote) { // promotion piece determined by original, no matter what was requested
      switch(piecetype(piece)) {
        case PAWN:
        case LANCE:
        case HONORABLEHORSE:
        case SILVER:
          promote = GOLD; break;
        case BISHOP:
          promote = DRAGONHORSE; break;
        case ROOK:
          promote = DRAGONKING; break;
        default: promote = NOPIECE; // not a promotion
      }
    } else
      switch(piecetype(piece)) { // force mandatory promotions
        case HONORABLEHORSE:
          if(mt->toRank == 1 || mt->toRank == gs->files-2) promote = GOLD;
        case PAWN:
        case LANCE:
          if(mt->toRank == 0 || mt->toRank == gs->files-1) promote = GOLD;
        default: break;
      }
    if(promote) mt->piecePromotionTo = promote | (colorval(gs->board[mt->fromFile][mt->fromRank]));
  } else
  if ((piecetype(gs->board[mt->fromFile][mt->fromRank]) == PAWN) && 
	!gs->palace && // [HGM] XQ: no promotions in xiangqi
      ((mt->toRank < gs->promoZone) || (mt->toRank >= gs->ranks - gs->promoZone))) {
    if(promote == KING) return MOVE_ILLEGAL; // no king in normal chess
    if(!promote && (mt->toRank == 0 || mt->toRank == gs->ranks-1)) { // promotion obligatory, but not specified
	if(gs->promoType != 2) promote = QUEEN; else { // choose a default
	    for(promote=PIECES-1; promote>PAWN; promote--) if(gs->holding[stm == BLACK][promote-PAWN]) break;
	    if(promote == PAWN) return MOVE_ILLEGAL; // nothing available
	}
    } // if not obligatory, we defer unless promotion was explicitly specified!
    if(!gs->pawnDblStep && promote == PRINCESS) promote = MAN2;
    if(!gs->pawnDblStep && promote != FERZ2 && promote != MAN2) promote = FERZ; // [HGM] kludge to recognize shatranj
    // non-promotion can still be an option for deeper promotion zones
#if BUGHOUSE_PAWN_REVERT
    mt->piecePromotionFrom = piecetype(gs->board[mt->fromFile][mt->fromRank]);
#endif
    mt->piecePromotionTo = promote ? (promote | stm) : NOPIECE;
    if(promote && gs->promoType == 2 && !gs->holding[stm == BLACK][promote-PAWN]) return MOVE_ILLEGAL; // unavailable piece specified
    if(promote == KNIGHT && gs->royalKnight) return MOVE_ILLEGAL; // Knight not allowed in Knightmate
    if(gs->promoType != 2 && promote > QUEEN) { // for promoType != 2 we must check explicitly if the requested pieceis compatible with the variant
	switch(promote) {
	  case HAWK:
	  case SELEPHANT:
	    if(gs->drops != 2) return MOVE_ILLEGAL; // allowed only in S-Chess
	    break;
	  case MARSHALL:
	  case CARDINAL:
	    if(!gs->capablancaPieces) return MOVE_ILLEGAL; // allowed when flagged so
	    break;
	  case FERZ:
	  case FERZ2:
	    if(!gs->pawnDblStep) return MOVE_ILLEGAL; // allowed in Shatranj and Courier
	    break;
	  case MAN2:
	    if(!gs->royalKnight) return MOVE_ILLEGAL; // allowed only in Knightmate
	    break;
	  default:
	    return MOVE_ILLEGAL;
	}
    }
  } else
  if ((piecetype(gs->board[mt->fromFile][mt->fromRank]) == HOPLITE) && 
      ((mt->toRank < gs->promoZone) || (mt->toRank >= gs->ranks - gs->promoZone))) {
    if(!promote || promote == KING) {
	int f, r, k=0, king = gs->onMove == WHITE ? W_KING : B_KING;
	for(r=0; r<gs->ranks;r++) for(f=0; f<gs->files; f++) k += (gs->board[f][r] == king);
	if(k > 1) { // we already have two kings
	  if(promote == KING) return MOVE_ILLEGAL; // three kings not allowed
	  promote = WARLORD;   // use strongest piece as default
	} else promote = KING; // if no promo-piece given, this could be mate test, so test if promoting to King evades
    } else
    if(promote == MASTODON) promote = GENERAL; else
    if(promote == WOODY)    promote = WARLORD; else
    if(promote == MARSHALL) promote = CAPTAIN; else
    if(promote != LIEUTENANT) return MOVE_ILLEGAL;
#if BUGHOUSE_PAWN_REVERT
    mt->piecePromotionFrom = piecetype(gs->board[mt->fromFile][mt->fromRank]);
#endif
    mt->piecePromotionTo = (promote | stm);
  } else if(gs->drops == 2 && promote && mt->fromRank == (stm == WHITE ? 0 : gs->ranks-1)) { // [HGM] Seirawan-style gating
    int i, halfMoves; struct game *g = &game_globals.garray[gs->gameNum];
    struct move_t* moveList;
    if(!gs->holding[stm == BLACK][promote-PAWN]) return MOVE_ILLEGAL; // unavailable piece specified
    // now we must test virginity of the moved piece. Yegh!
    halfMoves = g->numHalfMoves;
    moveList  = (g->status == GAME_EXAMINE) ? g->examMoveList  : g->moveList;
    for (i = halfMoves-1; i >= 0; i--) {
      if ((moveList[i].fromFile == mt->fromFile && moveList[i].fromRank == mt->fromRank) ||
          (moveList[i].toFile   == mt->fromFile && moveList[i].toRank   == mt->fromRank) ||
	  (moveList[i].fromFile == ALG_CASTLE && (i&1 ? gs->ranks-1 : 0) == mt->fromRank &&
		 (moveList[i].fromRank == mt->fromFile || gs->files>>1 == mt->fromFile ))) return MOVE_ILLEGAL;
    }
#if BUGHOUSE_PAWN_REVERT
    mt->piecePromotionFrom = piecetype(gs->board[mt->fromFile][mt->fromRank]);
#endif
    mt->piecePromotionTo = promote; // gating OK
    gating = 1; // remember we did it for check test
  } else {
    mt->piecePromotionTo = NOPIECE;
  }
  if ((piecetype(gs->board[mt->fromFile][mt->fromRank]) == PAWN) &&
   ((mt->fromRank - mt->toRank == 2) || (mt->toRank - mt->fromRank == 2))) {
    mt->doublePawn = mt->fromFile;
  } else {
    mt->doublePawn = -1;
  }
#if 0
  if ((piecetype(gs->board[mt->fromFile][mt->fromRank]) == KING) &&
      (mt->fromFile == gs->files/2) && (mt->toFile == 2) &&
       mt->fromRank == mt->toRank) {
    sprintf(mt->moveString, "o-o-o");
  } else if ((piecetype(gs->board[mt->fromFile][mt->fromRank]) == KING) &&
	     (mt->fromFile == gs->files/2) && (mt->toFile == gs->files-2) &&
		mt->fromRank == mt->toRank) {
    sprintf(mt->moveString, "o-o");
  } else {
#else
  {
#endif
    sprintf(mt->moveString, "%s/%c%d-%c%d",
	    wpstring[piecetype(gs->board[mt->fromFile][mt->fromRank])],
	    mt->fromFile + 'a', mt->fromRank + 1 - (gs->ranks>9),
	    mt->toFile + 'a', mt->toRank + 1 - (gs->ranks>9));
  }
  }
  /* Replace this with an algabraic de-parser */

  sprintf(mt->algString, alg_unparse(gs, mt));
  fakeMove = *gs;
  /* Calculates enPassant also */
  execute_move(&fakeMove, mt, 0);
  if(gating) fakeMove.board[mt->fromFile][mt->fromRank] = NOPIECE; // [HGM] gating is only legal if non-gating move was (weird, but true)

  /* Does making this move leave ME in check? */
  if (in_check(&fakeMove))
    return MOVE_ILLEGAL;
  /* IanO: bughouse variants: drop cannot be check/checkmate */

  return MOVE_OK;
}

int legal_andcheck_move(struct game_state_t * gs,
			int fFile, int fRank,
			int tFile, int tRank)
{
  struct move_t mt;

  if (!legal_move(gs, fFile, fRank, tFile, tRank))
    return 0;
  mt.color = gs->onMove;
  mt.fromFile = fFile;
  mt.fromRank = fRank;
  mt.toFile = tFile;
  mt.toRank = tRank;
  /* This should take into account a pawn promoting to another piece */
  if (move_calculate(gs, &mt, NOPIECE) == MOVE_OK) 
    return 1;
  else
    return 0;
}

/* in_check: checks if the side that is NOT about to move is in check 
 */
int in_check(struct game_state_t * gs)
{
  int f, r;
  int kf = -1, kr = -1;

  /* Find the king */
  if (gs->onMove == WHITE) {
    for (f = 0; f < gs->files && kf < 0; f++)
      for (r = 0; r < gs->ranks && kf < 0; r++)
	if (gs->board[f][r] == B_KING) {
	  kf = f;
	  kr = r;
	}
  } else {
    for (f = 0; f < gs->files && kf < 0; f++)
      for (r = 0; r < gs->ranks && kf < 0; r++)
	if (gs->board[f][r] == W_KING) {
	  kf = f;
	  kr = r;
	}
  }
  if (kf < 0) {
//    d_printf( "CHESSD: Error game with no king!\n");
    return -1;
  }
  for (InitPieceLoop(gs->board, &f, &r, gs->onMove);
       NextPieceLoop(gs->board, &f, &r, gs->onMove, gs->files, gs->ranks);) {
    if (legal_move(gs, f, r, kf, kr)) {	/* In Check? */
      if(gs->onMove == WHITE && !strcmp(gs->variant, "spartan")) { // first king is in check, but we might have spare
//printf("spartan K-capt %c%d%c%d\n",f+'a',r+1,kf+'a',kr+1);
	gs->board[kf][kr] = B_MAN; // temporarily cure the check on the first King by replacing the latter;
	r = in_check(gs);
	gs->board[kf][kr] = B_KING; // and put it back
//printf("duple = %d\n",r);
	return r != 0; // if we have no second king (r = -1) or the second is also attacked (r = 1) we are in check.
      }
      return 1;
    }
  }
  return 0;
}

int has_legal_move(struct game_state_t * gs)
{
  int i;
  int f, r;
  int kf = 0, kr = 0;
  int possiblef[500], possibler[500];
  int numpossible = 0;

  for (InitPieceLoop(gs->board, &f, &r, gs->onMove);
       NextPieceLoop(gs->board, &f, &r, gs->onMove, gs->files, gs->ranks);) {
    switch (piecetype(gs->board[f][r])) {
    case PAWN:
      possible_pawn_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case HOPLITE:
      possible_hoplite_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case KNIGHT:
      possible_knight_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case BISHOP:
      possible_bishop_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case ROOK:
      possible_rook_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case WARLORD:
    case HAWK:
    case CARDINAL:
    case PRINCESS:
      possible_cardinal_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case SELEPHANT:
    case MARSHALL:
    case EMPRESS:
      possible_marshall_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case MAN:
    case MAN2:
      possible_man_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case QUEEN:
      possible_queen_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case ELEPHANT:
      possible_elephant_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case AMAZON:
      possible_amazon_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case CAPTAIN:
    case WOODY:
      possible_woody_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case SQUIRREL:
      possible_squirrel_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case MASTODON:
      possible_mastodon_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case CENTAUR:
      possible_centaur_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case HORSE:
      possible_horse_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case FERZ:
    case FERZ2:
      possible_ferz_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case MANDARIN:
      possible_mandarin_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case WAZIR:
      possible_wazir_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case ALFIL:
    case ALFIL2:
      possible_alfil_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case MODERNELEPHANT:
      possible_modernelephant_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case LIEUTENANT:
      possible_lieutenant_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case PRIESTESS:
      possible_priestess_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case MINISTER:
      possible_minister_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case SILVER:
      possible_silver_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case GOLD:
      possible_gold_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case CANNON:
      possible_cannon_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case LANCE:
      possible_lance_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case DRAGONHORSE:
      possible_dragonhorse_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case GENERAL:
    case DRAGONKING:
      possible_dragonking_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case HONORABLEHORSE:
      possible_honorablehorse_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    case KING:
      kf = f;
      kr = r;
      possible_king_moves(gs, f, r, possiblef, possibler, &numpossible);
      break;
    }
    if (numpossible >= 500) {
      d_printf( "CHESSD: Possible move overrun\n");
    }
    for (i = 0; i < numpossible; i++)
      if (legal_andcheck_move(gs, f, r, possiblef[i], possibler[i])) {
	return 1;
      }
  }

  /* IanO:  if we got here, then kf and kr must be set */
  if ((gs->gameNum >=0 && game_globals.garray[gs->gameNum].link >= 0)
	|| gs->holdings) { // [HGM] zh: also in 2-player games with drops
    /* bughouse: potential drops as check interpositions */
    gs->holding[gs->onMove==WHITE ? 0 : 1][QUEEN - PAWN]++;
    for (f=kf-1; f<=kf+1; f++) for (r=kr-1; r<=kr+1; r++) {
      if (f>=0 && f<gs->files && r>=0 && r<gs->ranks && gs->board[f][r] == NOPIECE) {
	/* try a drop next to the king */
	if (legal_andcheck_move(gs, ALG_DROP, QUEEN, f, r)) {
	  gs->holding[gs->onMove==WHITE ? 0 : 1][QUEEN - PAWN]--;
	  // OK, so we have an interposing drop. But do we have something to drop?
	  if(game_globals.garray[gs->gameNum].link < 0) {
		// we have no partner, so we must have something to drop now
		for(i=QUEEN; i>=PAWN; i--)
			if (legal_andcheck_move(gs, ALG_DROP, i, f, r)) return 1;
		return 0;
	  }
	  return 1;
	}
      }
    }
    gs->holding[gs->onMove==WHITE ? 0 : 1][QUEEN - PAWN]--;
  }

  return 0;
}

/* This will end up being a very complicated function */
int parse_move(char *mstr, struct game_state_t * gs, struct move_t * mt, piece_t promote)
{
  int type = is_move(mstr);
  int result, flipflag = 1;

  mt->piecePromotionTo = NOPIECE;
  mt->color = gs->onMove;
  switch (type) {
  case MS_NOTMOVE:
    return MOVE_ILLEGAL;
    break;
  case MS_COMP:
    mt->fromFile = mstr[0] - 'a';
    mt->fromRank = mstr[1] - '1' + (gs->ranks>9);
    mt->toFile = mstr[2] - 'a';
    mt->toRank = mstr[3] - '1' + (gs->ranks>9);
    break;
  case MS_COMPDASH:
    mt->fromFile = mstr[0] - 'a';
    mt->fromRank = mstr[1] - '1' + (gs->ranks>9);
    mt->toFile = mstr[3] - 'a';
    mt->toRank = mstr[4] - '1' + (gs->ranks>9);
    break;
  case MS_KCASTLE:
#if 0
    mt->fromFile = gs->files/2;
    mt->toFile = gs->files-2;
    if (gs->onMove == WHITE) {
      mt->fromRank = 0;
      mt->toRank = 0;
    } else {
      mt->fromRank = gs->ranks-1;
      mt->toRank = gs->ranks-1;
    }
    break;
#endif
    // [HGM] castle: for now always assume Fischer-type castling (of which normal castling is a special case).
    mt->fromFile = ALG_CASTLE;
    mt->toFile = gs->files-2;
    mt->fromRank = gs->onMove == WHITE ? gs->wkrmoved : gs->bkrmoved;
    mt->toRank = mt->toFile-1; // R next to K
    break;    
  case MS_QCASTLE:
#if 0
    mt->fromFile = gs->files/2;
    mt->toFile = 2;
    if (gs->onMove == WHITE) {
      mt->fromRank = 0;
      mt->toRank = 0;
    } else {
      mt->fromRank = gs->ranks-1;
      mt->toRank = gs->ranks-1;
    }
    break;
#endif
    mt->fromFile = ALG_CASTLE;
    mt->toFile = 2;
    mt->fromRank = gs->onMove == WHITE ? gs->wqrmoved : gs->bqrmoved;
    mt->toRank = mt->toFile+1;
    break;
  case MS_ALG:
    /* Fills in the mt structure */
    if ((result = alg_parse_move(mstr, gs, mt)) != MOVE_OK)
      return result;
    break;
  default:
    return MOVE_ILLEGAL;
    break;
  }
  if (((mt->fromRank >= gs->ranks || mt->fromRank < 0 || mt->fromFile >= gs->files) &&
        mt->fromFile != ALG_DROP && mt->fromFile != ALG_CASTLE)
     || (mt->toRank < 0 || mt->toRank >= gs->ranks || mt->toFile >= gs->files))
    return MOVE_ILLEGAL; // [HGM] make sure move stays on board

  if (!(result = legal_move(gs, mt->fromFile, mt->fromRank, mt->toFile, mt->toRank)))
    return MOVE_ILLEGAL;

  if(result == 2) { // [HGM] castle: orthodox castling was given as King move; convert it to new format
	int ff=mt->fromFile, tf=mt->toFile;
	if(piecetype(gs->board[tf][mt->toRank]) == KING) { // [HGM] RxK notation
	    mt->fromFile = tf; mt->toFile = ff > tf ? gs->files-2 : 2; // correct to coventional
	    flipflag = -1; // kludge: flip gated piece
	}
	if(mt->fromFile - mt->toFile > 1) { // Q-side
		mt->fromRank = 0; 
		mt->toRank   = mt->toFile+1;
	} else if(mt->toFile - mt->fromFile > 1) { // K-side
		mt->fromRank = gs->files-1;
		mt->toRank   = mt->toFile-1;
	}
	mt->fromFile = ALG_CASTLE;
    }

  if (mt->piecePromotionTo != NOPIECE) {
	  promote = piecetype(mt->piecePromotionTo);
  } else if (promote != NOPIECE) { // [HGM] promotion on long algebraic move; correct ambiguous types for variant
    if(gs->promoType == 3 && promote == MASTODON) promote = GOLD;
    if(gs->drops == 2 && promote == EMPRESS) promote = SELEPHANT;
    if(gs->drops == 2 && promote == DRAGONHORSE) promote = HAWK;
  }

  return move_calculate(gs, mt, promote*flipflag);
}

/* Returns MOVE_OK, MOVE_NOMATERIAL, MOVE_CHECKMATE, or MOVE_STALEMATE */
/* check_game_status prevents recursion */
int execute_move(struct game_state_t * gs, struct move_t * mt, int check_game_status)
{
  int movedPiece;
  int tookPiece;
  int i, j, foobar, wCnt, bCnt, king, rook;

  if (mt->fromFile == ALG_DROP) {
    movedPiece = mt->fromRank;
    tookPiece = NOPIECE;
    gs->holding[gs->onMove==WHITE ? 0 : 1][movedPiece-PAWN]--;
    gs->board[mt->toFile][mt->toRank] = movedPiece | gs->onMove;
    if (gs->gameNum >= 0)
      gs->lastIrreversable = game_globals.garray[gs->gameNum].numHalfMoves;
  } else if(mt->fromFile == ALG_CASTLE) {
    int backRank, fKing;
    // [HGM] castle: perform castling
    if(gs->onMove == WHITE) {
	backRank = 0;
	fKing = gs->wkmoved;
	gs->wkmoved = -gs->wkmoved-2;
    } else {
	backRank = gs->ranks-1;
	fKing = gs->bkmoved;
	gs->bkmoved = -gs->bkmoved-2;
    }
    // move Rook & King, in a way that is resistant to ending where they started (for FRC!)
    rook = gs->board[mt->fromRank][backRank];    // first remember
    king = gs->board[fKing][backRank];
    gs->board[mt->fromRank][backRank] = NOPIECE; // then erase
    gs->board[fKing][backRank] = NOPIECE;
    gs->board[mt->toRank][backRank] = rook;      // then put back
    gs->board[mt->toFile][backRank] = king;
    if(gs->drops == 2 && mt->piecePromotionTo != NOPIECE) { // [HGM] Seirawan-style gating
      if(mt->piecePromotionTo > 0)
	gs->board[fKing][backRank] = mt->piecePromotionTo | gs->onMove; // gate on King square
      else
	gs->board[mt->fromRank][backRank] = -mt->piecePromotionTo | gs->onMove; // gate on Rook square
      gs->holding[gs->onMove==WHITE ? 0 : 1][abs(mt->piecePromotionTo)-PAWN]--; // remove gated piece from holdings
    }
  } else {
  movedPiece = gs->board[mt->fromFile][mt->fromRank];
  tookPiece = gs->board[mt->toFile][mt->toRank];
  if(gs->drops == 2 && mt->piecePromotionTo != NOPIECE && piecetype(movedPiece) != PAWN) { // [HGM] Seirawan-style gating
    gs->board[mt->toFile][mt->toRank] = gs->board[mt->fromFile][mt->fromRank];
    gs->board[mt->fromFile][mt->fromRank] = mt->piecePromotionTo | gs->onMove;;
    gs->holding[gs->onMove==WHITE ? 0 : 1][mt->piecePromotionTo-PAWN]--; // remove gated piece from holdings
  } else {
    if (mt->piecePromotionTo == NOPIECE) {
      gs->board[mt->toFile][mt->toRank] = gs->board[mt->fromFile][mt->fromRank];
    } else {
      gs->board[mt->toFile][mt->toRank] = mt->piecePromotionTo | gs->onMove;
      if(gs->promoType == 2) gs->holding[gs->onMove][mt->piecePromotionTo-PAWN]--;
    }
    gs->board[mt->fromFile][mt->fromRank] = NOPIECE;
  }
  /* Check if irreversable */
  if ((piecetype(movedPiece) == PAWN && mt->fromRank != mt->toRank) // [HGM] XQ: sideway Pawn move reversible!
			|| (tookPiece != NOPIECE)) {
    if (gs->gameNum >= 0)
      gs->lastIrreversable = game_globals.garray[gs->gameNum].numHalfMoves;
  }
  /* Check if this move is en-passant */
  if ((piecetype(movedPiece) == PAWN) && (mt->fromFile != mt->toFile) &&
      (tookPiece == NOPIECE) && !gs->palace) { // [HGM] XQ: no e.p. in sideway xiangqi moves
    if (gs->onMove == WHITE) {
      mt->pieceCaptured = B_PAWN;
    } else {
      mt->pieceCaptured = W_PAWN;
    }
    if (mt->fromFile > mt->toFile) {
      mt->enPassant = -1;
    } else {
      mt->enPassant = 1;
    }
    gs->board[mt->toFile][mt->fromRank] = NOPIECE;
  }
  /* Check en-passant flags for next moves */
  for (i = 0; i < gs->files; i++) {
    gs->ep_possible[0][i] = 0;
    gs->ep_possible[1][i] = 0;
  }
/* Added by Sparky 3/16/95

   From soso@Viktoria.drp.fmph.uniba.sk Thu Mar 16 13:08:51 1995
   Subject: Re: To DAV: enpassant prob. again
   To: chess@caissa.onenet.net (ICS)
   Date: Thu, 16 Mar 1995 20:06:20 +0100 (MET)

   Yeah !
   There was bug in other part of code:

   movecheck.c , line about 800:

     if (gs->onMove == WHITE) {
        if ((mt->toFile+1 < 7 ) ....  should be : (mt->toFile < 7 ) }
*/

  if ((piecetype(movedPiece) == PAWN) &&
   ((mt->fromRank == mt->toRank + 2) || (mt->fromRank + 2 == mt->toRank))) {
    /* Should turn on enpassent flag if possible */
    if (gs->onMove == WHITE) {
      if ((mt->toFile < gs->files-1) && gs->board[mt->toFile + 1][mt->toRank] == B_PAWN) {
	gs->ep_possible[1][mt->toFile + 1] = -1;
      }
      if ((mt->toFile - 1 >= 0) && gs->board[mt->toFile - 1][mt->toRank] == B_PAWN) {
	gs->ep_possible[1][mt->toFile - 1] = 1;
      }
    } else {
      if ((mt->toFile < gs->files-1) && gs->board[mt->toFile + 1][mt->toRank] == W_PAWN) {
	gs->ep_possible[0][mt->toFile + 1] = -1;
      }
      if ((mt->toFile - 1 >= 0) && gs->board[mt->toFile - 1][mt->toRank] == W_PAWN) {
	gs->ep_possible[0][mt->toFile - 1] = 1;
      }
    }
  }
  if ((piecetype(movedPiece) == ROOK) && (mt->fromRank == 0) && (gs->onMove == WHITE)) {
    if (mt->fromFile == gs->wqrmoved) // [HGM] castle: flip w.r.t. -1 to remember original
      gs->wqrmoved = -gs->wqrmoved-2;
    if (mt->fromFile == gs->wkrmoved)
      gs->wkrmoved = -gs->wkrmoved-2;
  }
  if ((piecetype(movedPiece) == ROOK) && (mt->fromRank == gs->ranks-1) && (gs->onMove == BLACK)) {
    if (mt->fromFile == gs->bqrmoved)
      gs->bqrmoved = -gs->bqrmoved-2;
    if (mt->fromFile == gs->bkrmoved)
      gs->bkrmoved = -gs->bkrmoved-2;
  }
  if (piecetype(movedPiece) == KING) {
    if ((gs->onMove == WHITE) && (mt->fromFile == gs->wkmoved))
      gs->wkmoved = -gs->wkmoved-2;
    if ((gs->onMove == BLACK) && (mt->fromFile == gs->bkmoved))
      gs->bkmoved = -gs->bkmoved-2;
  }
#if 0
  if ((piecetype(movedPiece) == KING) &&
      ((mt->fromFile == gs->files/2) && (mt->toFile == gs->files-2)) &&
	mt->fromRank == mt->toRank) {	/* Check for KS castling */
    gs->board[gs->files-3][mt->toRank] = gs->board[gs->files-1][mt->toRank];
    gs->board[gs->files-1][mt->toRank] = NOPIECE;
  }
  if ((piecetype(movedPiece) == KING) &&
      ((mt->fromFile == gs->files/2) && (mt->toFile == 2)) &&
	mt->fromRank == mt->toRank) {	/* Check for QS castling */
    gs->board[3][mt->toRank] = gs->board[0][mt->toRank];
    gs->board[0][mt->toRank] = NOPIECE;
  }
#endif
  }
  if (gs->onMove == BLACK)
    gs->moveNum++;

  if (check_game_status) {
    /* Does this move result in check? */
    if (in_check(gs)) {
      /* Check for checkmate */
      gs->onMove = CToggle(gs->onMove);
      if (!has_legal_move(gs))
	return MOVE_CHECKMATE;
    } else {
      /* Check for stalemate */
      gs->onMove = CToggle(gs->onMove);
      if (!has_legal_move(gs))
	return gs->stalemate ? MOVE_STALEMATE : MOVE_CHECKMATE; // [HGM] in XQ and shatranj stalemate loses
    }
/* loon: check for insufficient mating material, first try */
      foobar = wCnt = bCnt = 0;
      for (i=0; i<gs->files; i++) {
        for (j=0; j<gs->ranks; j++) {
	  piece_t p = gs->board[i][j];
          switch(piecetype(p)) {
            case KNIGHT:
            case BISHOP:
              foobar++;
              break;
            case KING:
	    case NOPIECE:
	      break;
            default:
              foobar = 2;
              break;
          }
	  if(p != NOPIECE && iscolor(p, WHITE)) wCnt++;
	  if(iscolor(p, BLACK)) bCnt++;
        }
      }
      if(gs->bareKingLoses) { // [HGM] with bare-King-loses rule only KK is insuff. material
	if(gs->onMove == BLACK && wCnt == 1 && bCnt > 1) return MOVE_BARE;
	if(gs->onMove == WHITE && bCnt == 1 && wCnt > 1) return MOVE_BARE;
	if(bCnt == 1 && wCnt == 1) return MOVE_NOMATERIAL;
      } else if (foobar < 2)
        return MOVE_NOMATERIAL;
  } else {
    gs->onMove = CToggle(gs->onMove);
  }

  return MOVE_OK;
}

static int is_promoted(struct game *g, int f, int r)
{
#if BUGHOUSE_PAWN_REVERT
  int i;
  int halfMoves;
  struct move_t* moveList;

  halfMoves = g->numHalfMoves;
  moveList  = (g->status == GAME_EXAMINE) ? g->examMoveList : g->moveList;
  for (i = halfMoves-2; i > 0; i -= 2) {
    if (moveList[i].toFile == f && moveList[i].toRank == r) {
      if (moveList[i].piecePromotionFrom)
	return moveList[i].piecePromotionFrom;
      if (moveList[i].piecePromotionTo) {
	switch (moveList[i].moveString[0]) { // [HGM] return original piece type rather than just TRUE
          case 'P': return PAWN;
          case 'N': return HONORABLEHORSE; // !!! this is Shogi, so no KNIGHT !!!
          case 'B': return BISHOP;
          case 'R': return ROOK;
          case 'L': return LANCE;
          case 'S': return SILVER;
          default:  return GOLD;
        }
      }
      if (moveList[i].fromFile == ALG_DROP)
	return 0;
      f = moveList[i].fromFile;
      r = moveList[i].fromRank;
    }
  }
#endif
  return 0;
}

int backup_move(int g, int mode)
{
  struct game_state_t *gs;
  struct move_t *m, *m1;
  int now, i, piece;

  if (game_globals.garray[g].link >= 0)	/*IanO: not implemented for bughouse yet */
    return MOVE_ILLEGAL;
  if (game_globals.garray[g].numHalfMoves < 1)
    return MOVE_ILLEGAL;
  gs = &game_globals.garray[g].game_state;
  m = (mode==REL_GAME) ? &game_globals.garray[g].moveList[game_globals.garray[g].numHalfMoves - 1] : 
                         &game_globals.garray[g].examMoveList[game_globals.garray[g].numHalfMoves - 1];
  if (m->toFile < 0) {
    return MOVE_ILLEGAL;
  }
  if(m->fromFile == ALG_CASTLE) {
    // [HGM] castling in new format. Derive K and R moves
    int rank, kingFromFile;
    if(m->color == WHITE) {
      rank = 0;
      kingFromFile = -gs->wkmoved-2;
      if(kingFromFile<0) kingFromFile = -kingFromFile-2; // safety catch; should never happen?
      gs->wkmoved = kingFromFile;
      if(m->toRank > m->toFile) gs->wqrmoved = m->fromRank;
      else gs->wkrmoved = m->fromRank;
    } else {
      rank = gs->ranks-1;
      kingFromFile = -gs->bkmoved-2;
      if(kingFromFile<0) kingFromFile = -kingFromFile-2; // safety catch; should never happen?
      gs->bkmoved = kingFromFile;
      if(m->toRank > m->toFile) gs->bqrmoved = m->fromRank;
      else gs->bkrmoved = m->fromRank;
    }
    // remove first, as one might come back to a square the other left
    gs->board[m->toFile  ][rank] = NOPIECE; // King toSqr
    gs->board[m->toRank  ][rank] = NOPIECE; // Rook toSqr
    if(gs->board[m->fromRank][rank] != NOPIECE)
      gs->holding[gs->onMove==WHITE ? 1 : 0][piecetype(gs->board[m->fromRank][rank])-PAWN]++; // put back in holdings (onMove not flipped yet!)
    if(gs->board[kingFromFile][rank] != NOPIECE)
      gs->holding[gs->onMove==WHITE ? 1 : 0][piecetype(gs->board[kingFromFile][rank])-PAWN]++; // put back in holdings (onMove not flipped yet!)
    gs->board[m->fromRank][rank] = ROOK | m->color; // Rook fromSqr
    gs->board[kingFromFile][rank] = KING | m->color; // King fromSquare
    goto cleanupMove;
  }
  piece = gs->board[m->toFile][m->toRank];
  if(gs->board[m->fromFile][m->fromRank] != NOPIECE) { // [HGM] from-square occupied; move must have been Seirawan-style gating
    gs->holding[gs->onMove==WHITE ? 1 : 0][piecetype(gs->board[m->fromFile][m->fromRank])-PAWN]++; // put back in holdings (onMove not flipped yet!)
  } else
  if (m->piecePromotionTo != NOPIECE) { // it is a real promotion
    switch(piecetype(m->piecePromotionTo)) { // Spartan pieces came from Hoplite, Shogi is problematic
      case KING:
      case CAPTAIN:
      case LIEUTENANT:
      case WARLORD:
      case GENERAL: piece = HOPLITE; break;
      case DRAGONHORSE: piece = BISHOP; break;
      case DRAGONKING:  piece = ROOK;   break;
      case GOLD:
#if BUGHOUSE_PAWN_REVERT
      if ((piece = is_promoted(&game_globals.garray[g], m->toFile, m->toRank))) break;
#endif
      default: piece = PAWN;
    }
    piece |= colorval(gs->board[m->toFile][m->toRank]);
  }
  gs->board[m->fromFile][m->fromRank] = piece;
  /******************
     When takeback a _first_ move of rook, the ??rmoved variable
     must be cleared . To check, if the move is first, we should
     scan moveList.
  *******************/
  if (piecetype(gs->board[m->fromFile][m->fromRank]) == ROOK) {
    if (m->color == WHITE) {
      if ((m->fromFile == -gs->wqrmoved-2) && (m->fromRank == 0)) {
	for (i = 2; i < game_globals.garray[g].numHalfMoves - 1; i += 2) {
	  m1 = (mode==REL_GAME) ? &game_globals.garray[g].moveList[i] : &game_globals.garray[g].examMoveList[i];
	  if ((m1->fromFile == -gs->wqrmoved-2) && (m1->fromRank == 0))
	    break;
	}
	if (i == game_globals.garray[g].numHalfMoves - 1)
	  gs->wqrmoved = m->fromFile;
      }
      if ((m->fromFile == -gs->wkrmoved-2) && (m->fromRank == 0)) {
	for (i = 2; i < game_globals.garray[g].numHalfMoves - 1; i += 2) {
	  m1 = (mode==REL_GAME) ? &game_globals.garray[g].moveList[i] : &game_globals.garray[g].examMoveList[i];
	  if ((m1->fromFile == -gs->wkrmoved-2) && (m1->fromRank == 0))
	    break;
	}
	if (i == game_globals.garray[g].numHalfMoves - 1)
	  gs->wkrmoved = m->fromFile;
      }
    } else {
      if ((m->fromFile == -gs->bqrmoved-2) && (m->fromRank == gs->ranks-1)) {
	for (i = 3; i < game_globals.garray[g].numHalfMoves - 1; i += 2) {
	  m1 = (mode==REL_GAME) ? &game_globals.garray[g].moveList[i] : &game_globals.garray[g].examMoveList[i];
	  if ((m1->fromFile == -gs->bkrmoved-2) && (m1->fromRank == gs->ranks-1))
	    break;
	}
	if (i == game_globals.garray[g].numHalfMoves - 1)
	  gs->bqrmoved = m->fromFile;
      }
      if ((m->fromFile == -gs->bkrmoved-2) && (m->fromRank == gs->ranks-1)) {
	for (i = 3; i < game_globals.garray[g].numHalfMoves - 1; i += 2) {
	  m1 = (mode==REL_GAME) ? &game_globals.garray[g].moveList[i] : &game_globals.garray[g].examMoveList[i];
	  if ((m1->fromFile == -gs->wkrmoved-2) && (m1->fromRank == gs->ranks-1))
	    break;
	}
	if (i == game_globals.garray[g].numHalfMoves - 1)
	  gs->bkrmoved = m->fromFile;
      }
    }
  }
  if (piecetype(gs->board[m->fromFile][m->fromRank]) == KING) {
    gs->board[m->toFile][m->toRank] = m->pieceCaptured;
#if 0
    /* [HGM] castlings are already intercepted due to new format; this code wrecks knightmate! */
    if (m->toFile - m->fromFile == 2) {
      gs->board[7][m->fromRank] = ROOK |
	colorval(gs->board[m->fromFile][m->fromRank]);
      gs->board[5][m->fromRank] = NOPIECE;

      /********
         If takeback a castling, the appropriates ??moved variables
         must be cleared
      ********/
      if (m->color == WHITE) {
	gs->wkmoved = 0;
	gs->wkrmoved = 0;
      } else {
	gs->bkmoved = 0;
	gs->bkrmoved = 0;
      }
      goto cleanupMove;
    }
    if (m->fromFile - m->toFile == 2) {
      gs->board[0][m->fromRank] = ROOK |
	colorval(gs->board[m->fromFile][m->fromRank]);
      gs->board[3][m->fromRank] = NOPIECE;

      /**********
         If takeback a castling, the appropriate ??moved variables
         must be cleared
      ***********/
      if (m->color == WHITE) {
	gs->wkmoved = 0;
	gs->wqrmoved = 0;
      } else {
	gs->bkmoved = 0;
	gs->bqrmoved = 0;
      }
      goto cleanupMove;
    }
#endif
    /******************
       When takeback a _first_ move of king (not the castling),
       the ?kmoved variable must be cleared . To check, if the move is first,
       we should scan moveList.
    *******************/

    if (m->color == WHITE) {

      if ((m->fromFile == -gs->wkmoved-2) && (m->fromRank == 0)) {
	for (i = 2; i < game_globals.garray[g].numHalfMoves - 1; i += 2) {
	  m1 = (mode==REL_GAME) ? &game_globals.garray[g].moveList[i] : &game_globals.garray[g].examMoveList[i];
	  if ((m1->fromFile == gs->wkmoved-2) && (m1->fromRank == 0))
	    break;
	}
	if (i == game_globals.garray[g].numHalfMoves - 1)
	  gs->wkmoved = m->fromFile;
      }
    } else {
      if ((m->fromFile == -gs->bkmoved-2) && (m->fromRank == gs->ranks-1)) {
	for (i = 3; i < game_globals.garray[g].numHalfMoves - 1; i += 2) {
	  m1 = (mode==REL_GAME) ? &game_globals.garray[g].moveList[i] : &game_globals.garray[g].examMoveList[i];
	  if ((m1->fromFile == -gs->bkmoved-2) && (m1->fromRank == gs->ranks-1))
	    break;
	}
	if (i == game_globals.garray[g].numHalfMoves - 1)
	  gs->bkmoved = m->fromFile;
      }
    }
  }
  if (m->enPassant) {		/* Do enPassant */
    gs->board[m->toFile][m->fromRank] = PAWN |
      (colorval(gs->board[m->fromFile][m->fromRank]) == WHITE ? BLACK : WHITE);
    gs->board[m->toFile][m->toRank] = NOPIECE;
    /* Should set the enpassant array, but I don't care right now */
    goto cleanupMove;
  }
  gs->board[m->toFile][m->toRank] = m->pieceCaptured;
  if (gs->holdings && m->pieceCaptured) {
      int victim = m->pieceCaptured;
      if (gs->holdings != -2)
        victim = is_promoted(&game_globals.garray[g], m->toFile, m->toRank);
      gs->holding[gs->onMove==WHITE ? 1 : 0][piecetype(victim)-PAWN]--; // remove captured piece from holdings (onMove not flipped yet!)
  }
cleanupMove:
  if (game_globals.garray[g].status != GAME_EXAMINE) {
    game_update_time(g);
  }
  game_globals.garray[g].numHalfMoves--;
  if (game_globals.garray[g].status != GAME_EXAMINE) {
    if (game_globals.garray[g].wInitTime) {	/* Don't update times in untimed games */
      now = tenth_secs();

      if (m->color == WHITE) {
        if (net_globals.con[player_globals.parray[game_globals.garray[g].white].socket]->timeseal) {  /* white uses timeseal? */      
          game_globals.garray[g].wRealTime += (m->tookTime * 100); 
          game_globals.garray[g].wRealTime -= (game_globals.garray[g].wIncrement * 100);
          game_globals.garray[g].wTime = game_globals.garray[g].wRealTime / 100;
          if (net_globals.con[player_globals.parray[game_globals.garray[g].black].socket]->timeseal) { /* opp uses timeseal? */
            game_globals.garray[g].bTime = game_globals.garray[g].bRealTime / 100;
	  } else {    /* opp has no timeseal */
            game_globals.garray[g].bTime += (game_globals.garray[g].lastDecTime - game_globals.garray[g].lastMoveTime);
	  }
	} else {  /* white has no timeseal */
          game_globals.garray[g].wTime += m->tookTime;
          game_globals.garray[g].wTime -= game_globals.garray[g].wIncrement;
          if (net_globals.con[player_globals.parray[game_globals.garray[g].black].socket]->timeseal) { /* opp uses timeseal? */
            game_globals.garray[g].bTime = game_globals.garray[g].bRealTime / 100;
	  } else {    /* opp has no timeseal */
            game_globals.garray[g].bTime += (game_globals.garray[g].lastDecTime - game_globals.garray[g].lastMoveTime);
	  }
	}
      } else {
        if (net_globals.con[player_globals.parray[game_globals.garray[g].black].socket]->timeseal) {  /* black uses timeseal? */
          game_globals.garray[g].bRealTime += (m->tookTime * 100);
          game_globals.garray[g].bRealTime -= (game_globals.garray[g].wIncrement * 100);
          game_globals.garray[g].bTime = game_globals.garray[g].bRealTime / 100;
          if (net_globals.con[player_globals.parray[game_globals.garray[g].white].socket]->timeseal) { /* opp uses timeseal? */
            game_globals.garray[g].wTime = game_globals.garray[g].wRealTime / 100;
	  } else {    /* opp has no timeseal */
            game_globals.garray[g].wTime += (game_globals.garray[g].lastDecTime - game_globals.garray[g].lastMoveTime);
	  }
	} else {  /* black has no timeseal */
          game_globals.garray[g].bTime += m->tookTime;
          if (!game_globals.garray[g].bIncrement)
            game_globals.garray[g].bTime -= game_globals.garray[g].wIncrement;
          else
  	    game_globals.garray[g].bTime -= game_globals.garray[g].bIncrement;
          if (net_globals.con[player_globals.parray[game_globals.garray[g].white].socket]->timeseal) { /* opp uses timeseal? */
            game_globals.garray[g].wTime = game_globals.garray[g].wRealTime / 100;
	  } else {    /* opp has no timeseal */
            game_globals.garray[g].wTime += (game_globals.garray[g].lastDecTime - game_globals.garray[g].lastMoveTime);
	  }
	}
      }

      if (game_globals.garray[g].numHalfMoves == 0)
        game_globals.garray[g].timeOfStart = now;
      game_globals.garray[g].lastMoveTime = now;
      game_globals.garray[g].lastDecTime = now;
    }
  }
  if (gs->onMove == BLACK)
    gs->onMove = WHITE;
  else {
    gs->onMove = BLACK;
    gs->moveNum--;
  }

  /******* Here begins the patch : ********************************
     Takeback of last move is done already, it's time to update enpassant
     array.  (patch from Soso, added by Sparky 3/17/95)
  ********/

  if (game_globals.garray[g].numHalfMoves > 0) {
    m1 = (mode==REL_GAME) ? &game_globals.garray[g].moveList[game_globals.garray[g].numHalfMoves - 1] : 
                            &game_globals.garray[g].examMoveList[game_globals.garray[g].numHalfMoves - 1];
    if (piecetype(gs->board[m1->toFile][m1->toRank]) == PAWN) {
      if ((m1->toRank - m1->fromRank) == 2) {
	if ((m1->toFile < gs->files-1) && gs->board[m1->toFile + 1][m1->toRank] == B_PAWN) {
	  gs->ep_possible[1][m1->toFile + 1] = -1;
	}
	if ((m1->toFile - 1 >= 0) && gs->board[m1->toFile - 1][m1->toRank] == B_PAWN) {
	  gs->ep_possible[1][m1->toFile - 1] = 1;
	}
      }
      if ((m1->toRank - m1->fromRank) == -2) {
	if ((m1->toFile < gs->files-1) && gs->board[m1->toFile + 1][m1->toRank] == W_PAWN) {
	  gs->ep_possible[0][m1->toFile + 1] = -1;
	}
	if ((m1->toFile - 1 >= 0) && gs->board[m1->toFile - 1][m1->toRank] == W_PAWN) {
	  gs->ep_possible[0][m1->toFile - 1] = 1;
	}
      }
    }
  }
  /************** and here's the end **************/
  return MOVE_OK;
}




