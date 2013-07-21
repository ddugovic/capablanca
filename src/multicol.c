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

struct multicol *multicol_start(int maxArray)
{
  int i;
  multicol *m;

  m = (multicol *) malloc(sizeof(multicol));
  m->arraySize = maxArray;
  m->num = 0;
  m->strArray = (char **) malloc(sizeof(char *) * m->arraySize);
  for (i = 0; i < m->arraySize; i++)
    m->strArray[i] = NULL;
  return m;
}

int multicol_store(multicol * m, char *str)
{
  if (m->num >= m->arraySize)
    return -1;
  if (!str)
    return -1;
  m->strArray[m->num] = strdup(str);
  m->num++;
  return 0;
}

int multicol_store_sorted(multicol * m, char *str)
/* use this instead of multicol_store to print a list sorted */
{
  int i;
  int found = 0;
  if (m->num >= m->arraySize)
    return -1;
  if (!str)
    return -1;
  for (i = m->num; (i > 0) && (!found); i--) {
    if (strcasecmp(str, m->strArray[i - 1]) >= 0) {
      found = 1;
      m->strArray[i] = strdup(str);
    } else {
      m->strArray[i] = m->strArray[i - 1];
    }
  }
  if (!found)
    m->strArray[0] = strdup(str);
  m->num++;
  return 0;
}

int multicol_pprint(multicol * m, int player, int cols, int space)
{
  int i;
  int maxWidth = 0;
  int numPerLine;
  int numLines;
  int on, theone, len;
  int done;
  int temp;
  char *tempptr;

  pprintf(player, "\n");
  for (i = 0; i < m->num; i++) {
    tempptr = m->strArray[i];
    temp = strlen(tempptr);	/* loon: yes, this is pathetic */
    for (; *tempptr; tempptr++) {
      if (*tempptr == '\033')
	temp -= 4;
    }
    if (temp > maxWidth)
      maxWidth = temp;
  }
  maxWidth += space;
  numPerLine = cols / maxWidth;
  if (!numPerLine)
    numPerLine = 1; /* stop a division by 0 on large members */
  numLines = m->num / numPerLine;
  if (numLines * numPerLine < m->num)
    numLines++;
  on = 0;
  done = 0;
  while (!done) {
    for (i = 0; i < numPerLine; i++) {
      theone = on + numLines * i;
      if (theone >= m->num) {
	break;
      }
      tempptr = m->strArray[theone];
      temp = strlen(tempptr);	/* loon: yes, still pathetic */
      for (; *tempptr; tempptr++) {
	if (*tempptr == '\033')
	  temp -= 4;
      }
      len = maxWidth - temp;
      if (i == numPerLine - 1)
	len -= space;
      pprintf(player, "%s", m->strArray[theone]);
      while (len) {
	pprintf(player, " ");
	len--;
      }
    }
    pprintf(player, "\n");
    on += 1;
    if (on >= numLines)
      break;
  }
  return 0;
}

int multicol_end(multicol * m)
{
	int i;
	for (i = 0; i < m->num; i++)
		FREE(m->strArray[i]);
	FREE(m->strArray);
	FREE(m);
	return 0;
}
