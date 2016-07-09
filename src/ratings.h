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

#ifndef _RATINGS_H
#define _RATINGS_H

#define STATS_VERSION 2

#define RESULT_WIN 0
#define RESULT_DRAW 1
#define RESULT_LOSS 2
#define RESULT_ABORT 3

#define PROVISIONAL 20

#define MAX_RANK_LINE 50
#define MAX_BEST 20

#define SHOW_BLITZ 0x1
#define SHOW_STANDARD 0x2
#define SHOW_WILD 0x4

typedef struct rateStruct {
  char name[MAX_LOGIN_NAME];
  int rating;
} rateStruct;

#endif /* _RATINGS_H */

