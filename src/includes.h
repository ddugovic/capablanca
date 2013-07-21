/*
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


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stddef.h>
#include <tdb.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <netdb.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include "autoconfig.h"
#include "malloc.h"
#include "parsers/genparser.h"
#include "common.h"
#include "vers.h"
#include "variable.h"
#include "command.h"
#include "gics.h"
#include "ficsmain.h"
#include "config.h"
#include "network.h"
#include "board.h"
#include "gamedb.h"
#include "lists.h"
#include "iset.h"
#include "playerdb.h"
#include "ratings.h"
#include "utils.h"
#include "talkproc.h"
#include "comproc.h"
#include "pending.h"
#include "multicol.h"
#include "movecheck.h"
#include "obsproc.h"
#include "formula.h"
#include "gameproc.h"
#include "matchproc.h"
#include "md5.h"
#include "news.h"
#include "globals.h"

#include "proto.h"
