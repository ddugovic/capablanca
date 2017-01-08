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

/* setup.c 

 Contains stuff for setting up boards in examine mode

*/

#include "includes.h"

static int check_valid_square(char* square)
{
 if (strlen (square) == 2) {
   if ( (square[0] <= 'a') && (square[0] >= 'h')
     && (square[1] <= '1') && (square [1] >= '8') )
     return 0;
   return 1; 
 } else
   return 0;
}

/* Check the position is valid
  ie two kings - one white one black
  ie no pawns on 1st or 8th ranks
  only the king that is to move may be in check,
  checkmate and stalemate */

static int validate_position (int p,struct game_state_t *b)
{
 int r,f,check;
 int white_k = 0;
 int black_k = 0;

  for (f = 0; (f < b->files); f++) {
    for (r = 0; (r < b->ranks); r++) {
      if (b->board[f][r] == W_KING) {
         white_k += 1;
         if (white_k == 2) {
           pprintf (p,"You can only have one white king.\n");
           return 0;
         }
      }
      if (b->board[f][r] == B_KING) {
         black_k += 1;
         if (black_k == 2) {
           pprintf (p,"You can only have one black king.\n");
           return 0;
         }
      }
      if (((b->board[f][r] == W_PAWN) || (b->board[f][r] == B_PAWN)) &&
         ((r == 0) || (r == b->ranks-1))) {
         pprintf (p,"Pawns cannot be placed on the first or eighth rank.\n");
         return 0;
      }
    }
  }
  if (!white_k) {
    pprintf (p,"There is no white king.\n");
    return 0;
  }
  if (!black_k) {
    pprintf (p,"There is no black king.\n");
    return 0;
  }
  if (b->onMove == WHITE) { pprintf(p, "WHITE to move\n"); }
  else if (b->onMove == BLACK) { pprintf(p, "BLACK to move\n"); }
  else pprintf(p, "ERROR!!\n");
  check = in_check(b);
  if (check) {
    pprintf (p,"Only the player to move may be in check.\n");
    return 0;
  }
  if (!has_legal_move(b)) {
    b->onMove = CToggle(b->onMove);
    if (in_check(b)) {
      pprintf (p, "%s is checkmated.\n",
                   b->onMove == WHITE  ?  "BLACK"  :  "WHITE");
    } else {
      pprintf (p, "%s is stalemated.\n",
                   b->onMove == WHITE  ?  "BLACK"  :  "WHITE");
    }
    b->onMove = CToggle(b->onMove);
  }
  return 1; /* valid position */
}

int com_setup (int p,param_list param)
{
 struct player *pp = &player_globals.parray[p];
 int gamenum;

 if ((pp->game <0) || (game_globals.garray[pp->game].status != GAME_SETUP)) {
   if (param[0].type == TYPE_NULL) {
       if (pp->game >= 0)
         if  (game_globals.garray[pp->game].status == GAME_EXAMINE) {
           game_free (pp->game);
           game_globals.garray[pp->game].status = GAME_SETUP;
           game_globals.garray[pp->game].numHalfMoves = 0;
	   game_globals.garray[pp->game].totalHalfMoves = 0;
	   game_globals.garray[pp->game].revertHalfMove = 0;
           pprintf (p,"Entering examine(setup) mode.\n");
           return COM_OK;
         }
     pcommand (p,"examine setup");
     return COM_OK_NOPROMPT;
   } else {
   pprintf(p, "You are not setting up a position.\n");
   return COM_OK;
   }
 }
 gamenum = game_globals.garray[pp->game].game_state.gameNum;
 if (param[0].type != TYPE_NULL) {
   if (!strcmp("clear",param[0].val.word)) {
     board_clear(&(game_globals.garray[pp->game].game_state));
     game_globals.garray[pp->game].game_state.gameNum = gamenum; 
     send_board_to(pp->game, p);
     pprintf (p,"Board cleared.\n"); 
     return COM_OK;
   } else if (!strcmp("start",param[0].val.word)) {
     board_standard(&(game_globals.garray[pp->game].game_state));
     game_globals.garray[pp->game].game_state.gameNum = gamenum;
     send_board_to(pp->game, p);
     pprintf (p,"Board set up as starting position.\n");
     return COM_OK;
   } else if (!strcmp("fen",param[0].val.word) && param[1].type != TYPE_NULL) {
     FEN_to_board(param[1].val.string, &(game_globals.garray[pp->game].game_state));
     game_globals.garray[pp->game].game_state.gameNum = gamenum;
     send_board_to(pp->game, p);
     pprintf (p,"Board set up by FEN.\n");
     return COM_OK;
   } else if (!strcmp("done",param[0].val.word)) {
     if (validate_position (p,&(game_globals.garray[pp->game].game_state))) {
       game_globals.garray[pp->game].status = GAME_EXAMINE;
       pprintf(p,"Game is validated - entering examine mode.\n");
       MakeFENpos(pp->game, game_globals.garray[pp->game].FENstartPos);
     } else
       pprintf(p,"The position is not valid - staying in setup mode.\n");
     return COM_OK;
   } else { /* try to load a category of board */
     char *board = param[1].val.word;
     if (param[1].type == TYPE_NULL) board = "0";
     game_globals.garray[pp->game].FENstartPos[0] = 0; // [HGM] new shuffle
       if (!board_init (pp->game,&(game_globals.garray[pp->game].game_state),param[0].val.word,board)) {
         game_globals.garray[pp->game].game_state.gameNum = gamenum;
	 if(!strcmp(board,"0"))
           sprintf(game_globals.garray[pp->game].variant, "%s",param[0].val.word);
         else
           sprintf(game_globals.garray[pp->game].variant, "%s/%s",param[0].val.word,board);
         send_board_to(pp->game, p);
         pprintf (p,"Board set up as %s %s.\n",param[0].val.word,board);
       } else {
         pprintf (p,"Board %s %s is unavailable.\n",param[0].val.word,board);
         game_globals.garray[pp->game].game_state.gameNum = gamenum;
       }
       return COM_OK;
     
   }
 }
 pprintf (p, "You have supplied an incorrect parameter to setup.\n");
 return COM_OK;
}

int com_tomove (int p,param_list param)
{
 struct player *pp = &player_globals.parray[p];
 if ((pp->game <0) || (game_globals.garray[pp->game].status != GAME_SETUP)) {
   if (pp->game >= 0)
     if (game_globals.garray[pp->game].status == GAME_EXAMINE) {
       pprintf (p,"This game is active - type 'setup' to allow editing.\n");
       return COM_OK;
     }
   pprintf(p, "You are not setting up a position.\n");
   return COM_OK;
 }
 if (!strcmp("white",param[0].val.word))
    game_globals.garray[pp->game].game_state.onMove = WHITE;
 else if (!strcmp("black",param[0].val.word))
    game_globals.garray[pp->game].game_state.onMove = BLACK;
 else {
   pprintf (p,"Please type: tomove white    or   tomove black\n");
   return COM_OK;
 }
 pcommand (p,"refresh");
 return COM_OK_NOPROMPT;
}

int com_clrsquare (int p,param_list param)
{
 struct player *pp = &player_globals.parray[p];
 if ((pp->game <0) || (game_globals.garray[pp->game].status != GAME_SETUP)) {
   if (pp->game >= 0)
     if (game_globals.garray[pp->game].status == GAME_EXAMINE) {
       pprintf (p,"This game is active - type 'setup' to allow editing.\n");
       return COM_OK;
     }
   pprintf(p, "You are not setting up a position.\n");
   return COM_OK;
 }

 if (!check_valid_square(param[0].val.word)) {    
   pprintf (p,"You must specify a square.");
   return COM_OK;
 }

 pcommand (p,"x@%s",param[0].val.word);
 return COM_OK_NOPROMPT;
}

/* allows the following

 x@rf or X@rf - clear a square
 P@rf - drop a white pawn
 p@rf - drop a black pawn
 wp@rf - drop a white pawn
 bp@rf - drop a black pawn
  can replace the @ with a * - some people do not have a @ key
*/

/* having to check before then after is lame - will change this later */
int is_drop(char* dropstr)
{

 int len = strlen (dropstr);

 if ((len < 4) || (len > 5))
   return 0;

/*check x@e3 */

 if ((dropstr[0] == 'x') || (dropstr[0] == 'X')) {
   if (!((dropstr[1] == '@') || (dropstr[1] == '*')))
     return 0;
   if (!check_valid_square(dropstr+2))
     return 0;

/*check wp@e3 */

 } else if (dropstr[0] == 'w') {
   if (!((dropstr[1] == 'p') || (dropstr[1] == 'r') || (dropstr[1] == 'n') || (dropstr[1] == 'b') || (dropstr[1] == 'q') || (dropstr[1] == 'k')))
     return 0;
   if (!((dropstr[2] == '@') || (dropstr[2] == '*')))
     return 0;
   if (!check_valid_square(dropstr+3))
     return 0;

/* check bp@e3 */

 } else if (len == 5) { /* so b@e2 and bb@e2 aren't confused */
   if (dropstr[0] == 'b') {
     if (!((dropstr[1] == 'p') || (dropstr[1] == 'r') || (dropstr[1] == 'n') || (dropstr[1] == 'b') || (dropstr[1] == 'q') || (dropstr[1] == 'k')))
       return 0;
     if (!((dropstr[2] == '@') || (dropstr[2] == '*')))
       return 0;
     if (!check_valid_square(dropstr+3))
       return 0;
     } else return 0; /* Exhausted 5 char possibilities */ 

/* check p@e3 and P@e3 */

 } else if ((dropstr[0] == 'p') || (dropstr[0] == 'r') || (dropstr[0] == 'n') || (dropstr[0] == 'b') || (dropstr[0] == 'q') || (dropstr[0] == 'k') || 
       (dropstr[0] == 'P') || (dropstr[0] == 'R') || (dropstr[0] == 'N') || (dropstr[0] == 'B') || (dropstr[0] == 'Q') || (dropstr[0] == 'K')) {

     if (!((dropstr[1] == '@') || (dropstr[1] == '*')))
      return 0;
     if (!check_valid_square(dropstr+2))
       return 0;
 } else return 0;
 return 1; /* valid drop */
}

static void getsquare(char* square,int *f, int *r)
{
 *f = square[0] - 'a';
 *r = square[1] - '1';
}

int attempt_drop(int p,int g,char* dropstr)
{
 int len = strlen (dropstr);
 int color;
 piece_t piece = 0;
 int f;
 int r;

 if ((len < 4) || (len > 5))
   return 0;

/*check x@e3 */

 if ((dropstr[0] == 'x') || (dropstr[0] == 'X')) { /* as x must be clear */
   getsquare(dropstr+2,&f,&r);
   piece = NOPIECE;

/*check wp@e3 */

 } else if (dropstr[0] == 'w') {
   piece = CharToPiece(dropstr[1], game_globals.garray[g].game_state.variant) & 0x7F;
   color = WHITE;
   getsquare(dropstr+3,&f,&r);

/* check bp@e3 */

 } else  if (len == 5) { /* check length to avoid b@e2 and bb@e2 being confused */
   if (dropstr[0] == 'b') {
     piece = CharToPiece(dropstr[1], game_globals.garray[g].game_state.variant) | BLACK;
     getsquare(dropstr+3,&f,&r);
   }

/* check p@e3 and P@e3 */

 } else {
   if (!((dropstr[1] == '@') || (dropstr[1] == '*')))
     return 0;
   else {
     piece = CharToPiece(dropstr[0], game_globals.garray[g].game_state.variant);
     getsquare(dropstr+2,&f,&r);
     }
 }

 game_globals.garray[g].game_state.board[f][r] = piece;
 return 1; /* valid drop */
}
