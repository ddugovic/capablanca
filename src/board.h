/*
   Copyright (c) 1993 Richard V. Nash.
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


#ifndef _BOARD_H
#define _BOARD_H

#define WHITE 0x00
#define BLACK 0x80
#define CString( c ) (  ((c) == WHITE) ? "White" : "Black" )
#define CToggle( c ) (  ((c) == BLACK) ? WHITE : BLACK )

/* These will go into an array */

#define NOPIECE 0
#define PAWN 1
#define KNIGHT 2
#define BISHOP 3
#define ROOK 4
#define CARDINAL 5
#define MARSHALL 6
#define MAN 7
#define QUEEN 8
#define ELEPHANT 9
#define ALFIL 10
#define FERZ 11
#define WAZIR 12
#define HORSE 13
#define HONORABLEHORSE 14
#define DRAGONKING 15
#define DRAGONHORSE 16
#define LANCE 17
#define CANNON 18
#define SILVER 19
#define GOLD 20
#define NIGHTRIDER 21
#define MANDARIN 22
#define FERZ2 23
#define ALFIL2 24
#define PRIESTESS 25
#define MINISTER 26
#define MAN2 27
#define MODERNELEPHANT 28
#define WOODY 29
#define SQUIRREL 30
#define MASTODON 31
#define CENTAUR 32
#define PRINCESS 33
#define EMPRESS 34
#define AMAZON 35
#define KING 36
#define HAWK 37
#define SELEPHANT 38
#define WARLORD 39
#define GENERAL 40
#define LIEUTENANT 41
#define CAPTAIN 42
#define HOPLITE 43
#define PIECES 44

#define MAX_BOARD_STRING_LENGTH 1280	/* Abitrarily 80 * 16 */
#define MAX_MOVE_LENGTH 20
#define MAX_STYLES 13

#define W_PAWN (PAWN | WHITE)
#define W_KNIGHT (KNIGHT | WHITE)
#define W_BISHOP (BISHOP | WHITE)
#define W_ROOK (ROOK | WHITE)
#define W_CARDINAL (CARDINAL | WHITE)
#define W_MARSHALL (MARSHALL | WHITE)
#define W_MAN (MAN | WHITE)
#define W_QUEEN (QUEEN | WHITE)
#define W_ELEPHANT (ELEPHANT | WHITE)
#define W_ALFIL (ALFIL | WHITE)
#define W_FERZ (FERZ | WHITE)
#define W_WAZIR (WAZIR | WHITE)
#define W_ALFIL2 (ALFIL2 | WHITE)
#define W_FERZ2 (FERZ2 | WHITE)
#define W_AMAZON (AMAZON | WHITE)
#define W_CENTAUR (CENTAUR | WHITE)
#define W_HORSE (HORSE | WHITE)
#define W_HONORABLEHORSE (HONORABLEHORSE | WHITE)
#define W_DRAGONKING (DRAGONKING | WHITE)
#define W_DRAGONHORSE (DRAGONHORSE | WHITE)
#define W_LANCE (LANCE | WHITE)
#define W_CANNON (CANNON | WHITE)
#define W_SILVER (SILVER | WHITE)
#define W_GOLD (GOLD | WHITE)
#define W_KING (KING | WHITE)
#define W_MANDARIN (MANDARIN | WHITE)
#define W_EMPRESS (EMPRESS | WHITE)
#define W_PRINCESS (PRINCESS | WHITE)
#define W_WOODY (WOODY | WHITE)
#define W_MINISTER (MINISTER | WHITE)
#define W_PRIESTESS (PRIESTESS | WHITE)
#define W_MASTODON (MASTODON | WHITE)
#define W_MAN2 (MAN2 | WHITE)
#define W_NIGHTRIDER (NIGHTRIDER | WHITE)
#define W_HAWK (HAWK | WHITE)
#define W_SELEPHANT (SELEPHANT | WHITE)

#define B_PAWN (PAWN | BLACK)
#define B_KNIGHT (KNIGHT | BLACK)
#define B_BISHOP (BISHOP | BLACK)
#define B_ROOK (ROOK | BLACK)
#define B_CARDINAL (CARDINAL | BLACK)
#define B_MARSHALL (MARSHALL | BLACK)
#define B_MAN (MAN | BLACK)
#define B_QUEEN (QUEEN | BLACK)
#define B_ELEPHANT (ELEPHANT | BLACK)
#define B_ALFIL (ALFIL | BLACK)
#define B_FERZ (FERZ | BLACK)
#define B_WAZIR (WAZIR | BLACK)
#define B_ALFIL2 (ALFIL2 | BLACK)
#define B_FERZ2 (FERZ2 | BLACK)
#define B_AMAZON (AMAZON | BLACK)
#define B_CENTAUR (CENTAUR | BLACK)
#define B_HORSE (HORSE | BLACK)
#define B_HONORABLEHORSE (HONORABLEHORSE | BLACK)
#define B_DRAGONKING (DRAGONKING | BLACK)
#define B_DRAGONHORSE (DRAGONHORSE | BLACK)
#define B_LANCE (LANCE | BLACK)
#define B_CANNON (CANNON | BLACK)
#define B_SILVER (SILVER | BLACK)
#define B_GOLD (GOLD | BLACK)
#define B_KING (KING | BLACK)
#define B_MANDARIN (MANDARIN | BLACK)
#define B_EMPRESS (EMPRESS | BLACK)
#define B_PRINCESS (PRINCESS | BLACK)
#define B_WOODY (WOODY | BLACK)
#define B_MINISTER (MINISTER | BLACK)
#define B_PRIESTESS (PRIESTESS | BLACK)
#define B_MASTODON (MASTODON | BLACK)
#define B_MAN2 (MAN2 | BLACK)
#define B_NIGHTRIDER (NIGHTRIDER | BLACK)
#define B_HAWK (HAWK | BLACK)
#define B_SELEPHANT (SELEPHANT | BLACK)
#define B_WARLORD (WARLORD | BLACK)
#define B_GENERAL (GENERAL | BLACK)
#define B_LIEUTENANT (LIEUTENANT | BLACK)
#define B_CAPTAIN (CAPTAIN | BLACK)
#define B_HOPLITE (HOPLITE | BLACK)

#define isblack(p) ((p) & BLACK)
#define iswhite(p) (!isblack(p))
#define iscolor(p,color) (((p) & BLACK) == (color))
#define piecechar(p) (isblack(p) ? bpchar[p] : wpchar[p])
#define piecetype(p) ((p) & 0x7f)
#define colorval(p) ((p) & 0x80)
#define square_color(r,f) ((((r)+(f)) & 0x01) ? BLACK : WHITE)

extern int pieceValues[PIECES];

#define BW 12
#define BH 10
/* Treated as [file][rank] */
typedef int piece_t;
typedef piece_t board_t[BW][BH];

GENSTRUCT struct game_state_t {
	piece_t board[BW][BH];
	/* for bughouse */
	piece_t holding[2][PIECES];
	/* For castling */
	char wkmoved, wqrmoved, wkrmoved;
	char bkmoved, bqrmoved, bkrmoved;
	/* for ep */
	int ep_possible[2][BW];
	/* For draws */
	int lastIrreversable;
	int onMove;
	int moveNum;
	/* Game num not saved, must be restored when read */
	int gameNum;
	char royalKnight;
	char capablancaPieces;
	char pawnDblStep;
	char ranks;
	char files;
	char holdings;
	char drops;
	char castlingStyle;
	char palace;
	char setup;
	char bareKingLoses;
	char stalemate;
	char promoType;
	char promoZone;
	char variant[20];
	char name[2][8];
};

#define ALG_DROP -2
#define ALG_CASTLE -3

/* bughouse: if a drop move, then fromFile is ALG_DROP and fromRank is piece */

GENSTRUCT struct move_t {
	int color;
	int fromFile, fromRank;
	int toFile, toRank;
	piece_t pieceCaptured;
#define BUGHOUSE_PAWN_REVERT 1
#if BUGHOUSE_PAWN_REVERT
	piece_t piecePromotionFrom;
#endif
	piece_t piecePromotionTo;
	int enPassant; /* 0 = no, 1=higher -1= lower */
	int doublePawn; /* Only used for board display */
	char moveString[8]; _NULLTERM
	char algString[8]; _NULLTERM
	char FENpos[74]; _NULLTERM
	unsigned atTime;
	unsigned tookTime;

	/* these are used when examining a game */
	int wTime;
	int bTime;

	/* [HGM] these are used for computer games */
	float score;
	int depth;
};

#define MoveToHalfMove( gs ) ((((gs)->moveNum - 1) * 2) + (((gs)->onMove == WHITE) ? 0 : 1))

extern const char wpchar[];
extern const char bpchar[];

extern int kludgeFlag; // [HGM] setup: forces move nr to zero in board printing

/* the FEN for the default initial position */
#define INITIAL_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w"

#endif
