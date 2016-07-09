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

/* Revision history:
   name		email		yy/mm/dd	Change
   Richard Nash	                93/10/22	Created
*/

#ifndef VARIABLE_H
#define VARIABLE_H

typedef struct var_list {
  char *name;
  int (*var_func)();
} var_list;

#define VAR_OK 0
#define VAR_NOSUCH 1
#define VAR_BADVAL 2
#define VAR_AMBIGUOUS 3

#define LANG_ENGLISH 0
#define LANG_SPANISH 1
#define LANG_FRENCH 2
#define LANG_DANISH 3
#define NUM_LANGS 4

extern var_list variables[];

#endif /* VARIABLE_H */
