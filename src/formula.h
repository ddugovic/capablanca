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


#ifndef FORMULA_H
#define FORMULA_H

#define OP_BAD      -1

#define OP_NONE      0
#define OP_RTPAREN   1
#define OP_OR        2
#define OP_AND       3
#define OP_EQ        4
#define OP_NEQ       5
#define OP_GT        6
#define OP_GE        7
#define OP_LT        8
#define OP_LE        9
#define OP_PLUS     10
#define OP_MINUS    11
#define OP_MULT     12
#define OP_DIV      13
#define OP_NEGATE   14
#define OP_NOT      15

#define OPTYPE_BAD     0
#define OPTYPE_NONE    1
#define OPTYPE_RPAREN  2
#define OPTYPE_PAREN   3
#define OPTYPE_OR      4
#define OPTYPE_AND     5
#define OPTYPE_COMPEQ  6
#define OPTYPE_COMPGL  7
#define OPTYPE_ADD     8
#define OPTYPE_MULT    9
#define OPTYPE_UNARY  10 

#define ERR_NONE 0
#define ERR_EOF 1
#define ERR_UNARY 2
#define ERR_BADCHAR 3
#define ERR_BADVAR 4
#define ERR_BADOP 5
#define ERR_NONESENSE 6
#define ERR_DIVBYZERO 7
#define ERR_PAREN 8

#define FUDGE_FACTOR 1e-3

#endif

