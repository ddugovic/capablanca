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

/*    
 * This file contains descriptive text for the C preprocessor macros
 * that are missing in the original acconfig.h of the autoconf
 * distribution, but used in the configure.in of the FICS distribution.
 *
 * It is used *only* by the autoheader program contained in the
 * autoconf distribution.
 *
 *
 * Leave the following blank line there!!  Autoheader needs it.
 */

/*  Define this to be the return value of the time() function.  */
#undef time_t

/*  Undefine, if your compiler doesn't support prototypes  */
#undef HAVE_PROTOTYPES

/*  Define, if you have a statfs() function which fills a struct
    statfs, declared in sys/vfs.h. (NeXTStep)  */
#undef HAVE_STATFS_FILLING_STRUCT_STATFS

/*  Define, if you have a statfs() function which fills a struct
    fs_data, declared in sys/mount.h. (Ultrix)  */
#undef HAVE_STATFS_FILLING_STRUCT_FS_DATA

/*  Define, if you have crypt() and it is declared in either
    crypt.h or unistd.h.  */
#undef HAVE_CRYPT_DECLARED

/*  Define this to be tv_usec, if your struct rusage has
    a member ru_utime.tv_usec.
    Define this to be tv_nsec, if your struct rusage has
    a member ru_utime.tv_nsec.  */
#undef TV_USEC

#undef HAVE_DLOPEN

