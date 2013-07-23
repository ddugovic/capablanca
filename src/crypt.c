/*
   Copyright (C) Daniel Dugovic 2013.
   
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
  crypt.c is loosely based on an implementation (C) Mark Murray 1999.
  It wrappers the standard crypt() function and does some sanity-checking
  on the salt value.
 */

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *gen_salt(char *salt, const int len)
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	int i;
	for (i = 0; i < len; ++i) {
		salt[i] = alphanum[random() % (sizeof(alphanum) - 1)];
	}
	salt[len] = '\0';
	return salt;
}

char *chessd_crypt(const char *passwd, const char *salt)
{
	if (salt != NULL) {
		return crypt(passwd, salt);
	}
	// Creates salt for SHA-based encryption.
	char sha_salt[20] = "$6$";
	gen_salt(sha_salt+3, 16);
	return crypt(passwd, sha_salt);
}
