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


#ifndef _COMMON_H
#define _COMMON_H

#ifndef MAX 
#define MAX(x,y)  (((x)>(y))? (x):(y))
#endif
#ifndef MIN
#define MIN(x,y)  (((x)>(y))? (y):(x))
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#define SWAP(a,b,type) {\
  type tmp; \
  tmp = (a);\
  (a) = (b);\
  (b) = tmp;\
}

#define ZERO_STRUCT(s) memset(&(s), 0, sizeof(s))
#define ZERO_STRUCTP(s) memset(s, 0, sizeof(*(s)))


#ifdef __GNUC__
/** Use gcc attribute to check printf fns.  a1 is the 1-based index of
 * the parameter containing the format, and a2 the index of the first
 * argument.  **/
#define PRINTF_ATTRIBUTE(a1, a2) __attribute__ ((format (__printf__, a1, a2)))
#else
#define PRINTF_ATTRIBUTE(a1, a2)
#endif


/* declare some printf style functions so the compiler checks the args */
FILE *fopen_p(const char *fmt, const char *mode, ...) PRINTF_ATTRIBUTE(1,3);
int asprintf(char **strp, const char *fmt, ...) PRINTF_ATTRIBUTE(2,3);
void d_printf(const char *fmt, ...) PRINTF_ATTRIBUTE(1,2);
int pprintf(int p, const char *format,...) PRINTF_ATTRIBUTE(2,3);
int pprintf_highlight(int p, const char *format,...) PRINTF_ATTRIBUTE(2,3);
int psprintf_highlight(int p, char *s, const char *format,...) PRINTF_ATTRIBUTE(3,4);
int pprintf_prompt(int p, const char *format,...) PRINTF_ATTRIBUTE(2,3);
int pprintf_noformat(int p, const char *format,...) PRINTF_ATTRIBUTE(2,3);
int pcommand(int p, char *comstr,...) PRINTF_ATTRIBUTE(2,3);



/* prototypes for replacement functions */
#ifndef HAVE_DPRINTF
int dprintf(int fd, const char *format, ...);
#endif
#ifndef HAVE_STRNLEN
size_t strnlen(const char *s, size_t n);
#endif

#ifndef HAVE_STRLCPY
size_t strlcpy(char *d, const char *s, size_t bufsize);
#endif
#ifndef HAVE_STRLCAT
size_t strlcat(char *d, const char *s, size_t bufsize);
#endif

#if HAVE_COMPAR_FN_T
#define COMPAR_FN_T __compar_fn_t
#else
typedef int (*COMPAR_FN_T)(const void *, const void *);
#endif

/* tell tdb that it can use mmap */
#define HAVE_MMAP 1

#endif /* _COMMON_H */
