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

#ifndef _UTILS_H
#define _UTILS_H

#define MAX_WORD_SIZE 1024

/* Maximum length of an output line */
#define MAX_LINE_SIZE 1024

/* Maximum size of a filename */
#ifdef FILENAME_MAX
#  define MAX_FILENAME_SIZE FILENAME_MAX
#else
#  define MAX_FILENAME_SIZE 1024
#endif

#define FlagON(VAR, FLAG)  (VAR |= (FLAG))
#define FlagOFF(VAR, FLAG)  (VAR &= ~(FLAG))   
#define CheckFlag(VAR, FLAG)  ((VAR) & (FLAG))
#define BoolCheckFlag(VAR, FLAG)  (CheckFlag(VAR, FLAG)  ?  1  :  0)
#define ToggleFlag(VAR, FLAG)  (VAR ^= (FLAG))
#define SetFlag(VAR, FLAG, VALUE)  ((VALUE) ? FlagON(VAR, FLAG) : FlagOFF(VAR, FLAG))

#define PFlagON(WHO, FLAG)  FlagON(player_globals.parray[WHO].Flags, FLAG)
#define PFlagOFF(WHO, FLAG)  FlagOFF(player_globals.parray[WHO].Flags, FLAG)
#define CheckPFlag(WHO, FLAG)  CheckFlag(player_globals.parray[WHO].Flags, FLAG)
#define BoolCheckPFlag(WHO, FLAG)  BoolCheckFlag(player_globals.parray[WHO].Flags, FLAG)
#define TogglePFlag(WHO, FLAG)  ToggleFlag(player_globals.parray[WHO].Flags, FLAG)
#define SetPFlag(WHO, FLAG, VALUE)  ((VALUE) ? PFlagON(WHO, FLAG) : PFlagOFF(WHO, FLAG))


#endif /* _UTILS_H */
