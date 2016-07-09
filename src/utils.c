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

int safestring(char *str);

char *eatword(char *str)
{
  while (*str && !isspace(*str))
    str++;
  return str;
}

char *eatwhite(char *str)
{
  while (*str && isspace(*str))
    str++;
  return str;
}

char *eattailwhite(char *str)
{
  int len;
  if (str == NULL)
    return NULL;

  len = strlen(str);
  while (len > 0 && isspace(str[len - 1]))
    len--;
  str[len] = '\0';
  return (str);
}

char *nextword(char *str)
{
  return eatwhite(eatword(str));
}

int mail_string_to_address(char *addr, char *subj, char *str)
{
  int fd;
  char template[] = SPOOL_DIR;

  fd = mkstemp(template);
  if (fd == -1) {
    d_printf("Failed to create spool file\n");
    return -1;
  }

  dprintf(fd, "To: %s\nSubject: %s\n\n%s\n", addr, subj, str);
  close(fd);

  return 0;
}

int mail_string_to_user(int p, char *subj, char *str)
{
	struct player *pp = &player_globals.parray[p];
	if (pp->emailAddress &&
	    pp->emailAddress[0] &&
	    safestring(pp->emailAddress)) {
		return mail_string_to_address(pp->emailAddress, subj, str);
	}
	return -1;
}


/* Process a command for a user */
int pcommand(int p, char *comstr,...)
{
	static int recurse = 0;
	struct player *pp = &player_globals.parray[p];
	char *tmp;
	int retval;
	int current_socket = pp->socket;
	va_list ap;
	
	if(recurse > 10) return COM_BADPARAMETERS; /* [HGM] guard against infinite recursion through bad aliasing */

	va_start(ap, comstr);
	vasprintf(&tmp, comstr, ap);
	va_end(ap);
	recurse++;
	retval = process_input(current_socket, tmp);
	recurse--;
	free(tmp);
	if (retval == COM_LOGOUT) {
		process_disconnection(current_socket);
		net_close_connection(current_socket);
	}
	return retval;
}

static int vpprintf(int p, int do_formatting, char *format,va_list ap)
{
	struct player *pp = &player_globals.parray[p];
	char *tmp = NULL;
	int retval;

	retval = vasprintf(&tmp, format, ap);
	if (retval != 0) {
		net_send_string(pp->socket, 
				tmp, 
				do_formatting, 
				pp->d_width + 1);
	}
	if (tmp) {
		free(tmp);
	}

	return retval;
}

int pprintf(int p, char *format,...)
{
	int retval;
	va_list ap;
	va_start(ap, format);
	retval = vpprintf(p, 1, format, ap);
	va_end(ap);
	return retval;
}

static void pprintf_dohightlight(int p)
{
	struct player *pp = &player_globals.parray[p];
	if (pp->highlight & 0x01)
		pprintf(p, "\033[7m");
	if (pp->highlight & 0x02)
		pprintf(p, "\033[1m");
	if (pp->highlight & 0x04)
		pprintf(p, "\033[4m");
	if (pp->highlight & 0x08)
		pprintf(p, "\033[2m");
}

int pprintf_highlight(int p, char *format,...)
{
	struct player *pp = &player_globals.parray[p];
	int retval;
	va_list ap;

	pprintf_dohightlight(p);
	va_start(ap, format);
	retval = vpprintf(p, 1, format, ap);
	va_end(ap);
	if (pp->highlight)
		pprintf(p, "\033[0m");
	return retval;
}

static void sprintf_dohightlight(int p, char *s)
{
	struct player *pp = &player_globals.parray[p];
	if (pp->highlight & 0x01)
		strcat(s, "\033[7m");
	if (pp->highlight & 0x02)
		strcat(s, "\033[1m");
	if (pp->highlight & 0x04)
		strcat(s, "\033[4m");
	if (pp->highlight & 0x08)
		strcat(s, "\033[2m");
}

/*
  like pprintf() but with paging for long messages
*/
int pprintf_more(int p, char *format,...)
{
	struct player *pp = &player_globals.parray[p];
	char *s = NULL;
	va_list ap;
	va_start(ap, format);
	vasprintf(&s, format, ap);
	va_end(ap);

	FREE(pp->more_text);
	pp->more_text = s;

	return pmore_text(p);
}

int psprintf_highlight(int p, char *s, char *format,...)
{
	struct player *pp = &player_globals.parray[p];
	int retval;
	va_list ap;
	
	va_start(ap, format);
	if (pp->highlight) {
		sprintf_dohightlight(p, s);
		retval = vsprintf(s + strlen(s), format, ap);
		strcat(s, "\033[0m");
	} else {
		retval = vsprintf(s, format, ap);
	}
	va_end(ap);
	return retval;
}

int pprintf_prompt(int p, char *format,...)
{
	int retval;
	va_list ap;
	va_start(ap, format);
	retval = vpprintf(p, 1, format, ap);
	va_end(ap);
	send_prompt(p);
	return retval;
}

/* send a prompt to p */
void send_prompt(int p) 
{
	struct player *pp = &player_globals.parray[p];
	const char *prompt = pp->prompt;
	pprintf(p, "%s%s", 
		prompt,
		isspace(prompt[strlen(prompt)-1])?"":" ");
}

int pprintf_noformat(int p, char *format,...)
{
	int retval;
	va_list ap;
	va_start(ap, format);
	retval = vpprintf(p, 0, format, ap);
	va_end(ap);
	return retval;
}

void Bell (int p)
{
	if (CheckPFlag(p, PFLAG_BELL))
		pprintf (p, "\a");
	return;
}

int psend_raw_file(int p, char *dir, char *file)
{
	struct player *pp = &player_globals.parray[p];
	FILE *fp;
	char tmp[MAX_LINE_SIZE];
	int num;
	
	fp = fopen_p("%s/%s", "r", dir, file);
	
	if (!fp)
		return -1;
	while ((num = fread(tmp, sizeof(char), MAX_LINE_SIZE - 1, fp)) > 0) {
		tmp[num] = '\0';
		net_send_string(pp->socket, tmp, 1, pp->d_width + 1);
	}
	fclose(fp);
	return 0;
}

/*
  send a file a page at a time
*/
int psend_file(int p, const char *dir, const char *file)
{
	struct player *pp = &player_globals.parray[p];
	char *fname;

	if (strstr(file, "..")) {
		pprintf(p,"Trying to be tricky, are we?\n");
		return 0;
	}

	if (dir) {
		asprintf(&fname,"%s/%s",dir,file);
	} else {
		fname = strdup(file);
	}

	FREE(pp->more_text);
	pp->more_text = file_load(fname, NULL);
	if (!pp->more_text) {
		return -1;
	}
	
	free(fname);

	return pmore_text(p);
}

/* 
 * Marsalis added on 8/27/95 so that [next] does not
 * appear in the logout process for those that have
 * a short screen height.  (Fixed due to complaint 
 * in Bug's messages).
 */
int psend_logoutfile(int p, char *dir, char *file)
{
	return psend_file(p, dir, file);
}

/* 
   continue with text from a previous command
*/
int pmore_text(int p)
{
	struct player *pp = &player_globals.parray[p];
	int lcount = pp->d_height - 1;

	if (!pp->more_text) {
		pprintf(p, "There is no more.\n");
		return -1;
	}

	while (pp->more_text && lcount--) {
		char *s = strndup(pp->more_text, strcspn(pp->more_text, "\n")+1);
		int len = strlen(s);
		net_send_string(pp->socket, s, 1, pp->d_width + 1);
		s = strdup(pp->more_text+len);
		FREE(pp->more_text);
		if (s[0]) {
			pp->more_text = s;
		} else {
			free(s);
		}
	}
	
	if (pp->more_text) {
		pprintf(p, "Type [next] to see next page.\n");
	}

	return 0;
}

char *stolower(char *str)
{
	int i;
	
	if (!str)
		return NULL;
	for (i = 0; str[i]; i++) {
		if (isupper(str[i])) {
			str[i] = tolower(str[i]);
		}
	}
	return str;
}

static int safechar(int c)
{
	return (isprint(c) && !strchr(">!&*?/<|`$;", c));
}

int safestring(char *str)
{
	int i;

	if (!str)
		return 1;
	for (i = 0; str[i]; i++) {
		if (!safechar(str[i]))
			return 0;
	}
	return 1;
}

int alphastring(char *str)
{
	int i;

	if (!str)
		return 1;
	for (i = 0; str[i]; i++) {
		if (!isalpha(str[i])) {
			return 0;
		}
	}
	return 1;
}

int printablestring(const char *str)
{
	int i;
	
	if (!str)
		return 1;
	for (i = 0; str[i]; i++) {
		if ((!isprint(str[i])) && (str[i] != '\t') && (str[i] != '\n'))
			return 0;
	}
	return 1;
}

char *hms_desc(int t)
{
static char tstr[80];
int days, hours, mins, secs;

    days  = (t / (60*60*24));
    hours = ((t % (60*60*24)) / (60*60));
    mins  = (((t % (60*60*24)) % (60*60)) / 60);
    secs  = (((t % (60*60*24)) % (60*60)) % 60);
    if ((days==0) && (hours==0) && (mins==0)) {
      sprintf(tstr, "%d sec%s",
                 secs, (secs==1) ? "" : "s");
    } else if ((days==0) && (hours==0)) {
/*      sprintf(tstr, "%d min%s, %d sec%s",
                 mins, (mins==1) ? "" : "s",
                 secs, (secs==1) ? "" : "s");  */
      sprintf(tstr, "%d min%s",
                 mins, (mins==1) ? "" : "s");
    } else if (days==0) {
      sprintf(tstr, "%d hr%s, %d min%s, %d "
                 "sec%s",
                 hours, (hours==1) ? "" : "s",
                 mins, (mins==1) ? "" : "s",
                 secs, (secs==1) ? "" : "s");
    } else {
      sprintf(tstr, "%d day%s, %d hour%s, %d minute%s "
                 "and %d second%s",
                 days, (days==1) ? "" : "s",
                 hours, (hours==1) ? "" : "s",
                 mins, (mins==1) ? "" : "s",
                 secs, (secs==1) ? "" : "s");
    }
    return tstr;
}

char *hms(int t, int showhour, int showseconds, int spaces)
{
  static char tstr[20];
  char tmp[10];
  int h, m, s;

  h = t / 3600;
  t = t % 3600;
  m = t / 60;
  s = t % 60;
  if (h || showhour) {
    if (spaces)
      sprintf(tstr, "%d : %02d", h, m);
    else
      sprintf(tstr, "%d:%02d", h, m);
  } else {
    sprintf(tstr, "%d", m);
  }
  if (showseconds) {
    if (spaces)
      sprintf(tmp, " : %02d", s);
    else
      sprintf(tmp, ":%02d", s);
    strcat(tstr, tmp);
  }
  return tstr;
}

/* This is used only for relative timeing since it reports seconds since
 * about 5:00pm on Feb 16, 1994
 */
unsigned tenth_secs(void)
{
  struct timeval tp;
  struct timezone tzp;

  gettimeofday(&tp, &tzp);
/* .1 seconds since 1970 almost fills a 32 bit int! So lets subtract off
 * the time right now */
  return ((tp.tv_sec - 331939277) * 10L) + (tp.tv_usec / 100000);
}

/* This is to translate tenths-secs time back into 1/1/70 time in full
 * seconds, because vek didn't read utils.c when he programmed new ratings.
   1 sec since 1970 fits into a 32 bit int OK.
*/
int untenths(unsigned tenths)
{
  return (tenths / 10 + 331939277 + 4*((1<<30) / 5) + 1);
}

char *tenth_str(unsigned t, int spaces)
{
  return hms((t + 5) / 10, 0, 1, spaces);	/* Round it */
}

#define MAX_TRUNC_SIZE 100

/* Warning, if lines in the file are greater than 1024 bytes in length, this
   won't work! */
/* nasty bug fixed by mann, 3-12-95 */
int truncate_file(char *file, int lines)
{
  FILE *fp;
  int bptr = 0, trunc = 0, i;
  char tBuf[MAX_TRUNC_SIZE][MAX_LINE_SIZE];

  if (lines > MAX_TRUNC_SIZE)
    lines = MAX_TRUNC_SIZE;
  fp = fopen_s(file, "r");
  if (!fp)
    return 1;
  while (!feof(fp)) {
    fgets(tBuf[bptr], MAX_LINE_SIZE, fp);
    if (feof(fp))
      break;
    if (tBuf[bptr][strlen(tBuf[bptr]) - 1] != '\n') {	/* Line too long */
      fclose(fp);
      return -1;
    }
    bptr++;
    if (bptr == lines) {
      trunc = 1;
      bptr = 0;
    }
  }
  fclose(fp);
  if (trunc) {
    fp = fopen_s(file, "w");
    for (i = 0; i < lines; i++) {
      fputs(tBuf[bptr], fp);
      bptr++;
      if (bptr == lines) {
	bptr = 0;
      }
    }
    fclose(fp);
  }
  return 0;
}

/* Warning, if lines in the file are greater than 1024 bytes in length, this
   won't work! */
int lines_file(char *file)
{
  FILE *fp;
  int lcount = 0;
  char tmp[MAX_LINE_SIZE];

  fp = fopen_s(file, "r");
  if (!fp)
    return 0;
  while (!feof(fp)) {
    if (fgets(tmp, MAX_LINE_SIZE, fp))
      lcount++;
  }
  fclose(fp);
  return lcount;
}

int file_has_pname(char *fname, char *plogin)
{
  if (!strcmp(file_wplayer(fname), plogin))
    return 1;
  if (!strcmp(file_bplayer(fname), plogin))
    return 1;
  return 0;
}

char *file_wplayer(char *fname)
{
  static char tmp[MAX_FILENAME_SIZE];
  char *ptr;
  strcpy(tmp, fname);
  ptr = rindex(tmp, '-');
  if (!ptr)
    return "";
  *ptr = '\0';
  return tmp;
}

char *file_bplayer(char *fname)
{
  char *ptr;

  ptr = rindex(fname, '-');
  if (!ptr)
    return "";
  return ptr + 1;
}

/*
  make a human readable IP
*/
char *dotQuad(struct in_addr a)
{
	return inet_ntoa(a);
}

int file_exists(char *fname)
{
  FILE *fp;

  fp = fopen_s(fname, "r");
  if (!fp)
    return 0;
  fclose(fp);
  return 1;
}

char *ratstr(int rat)
{
  static int on = 0;
  static char tmp[20][10];

  if (on == 20)
    on = 0;
  if (rat) {
    sprintf(tmp[on], "%4d", rat);
  } else {
    sprintf(tmp[on], "----");

  }
  on++;
  return tmp[on - 1];
}

char *ratstrii(int rat, int p)
{
  static int on = 0;
  static char tmp[20][10];

  if (on == 20)
    on = 0;
  if (rat) {
    sprintf(tmp[on], "%4d", rat);
  } else if (CheckPFlag(p, PFLAG_REG)) {
    sprintf(tmp[on], "----");
  } else {
    sprintf(tmp[on], "++++");
  }
  on++;
  return tmp[on - 1];
}

#define OVERRUN 99

struct t_tree {
  struct t_tree *left, *right;
  char name[OVERRUN+1];         // [HGM] hack below caused crashing on some compilers
//  char name;			/* Not just 1 char - space for whole name */
};				/* is allocated.  Maybe a little cheesy? */

struct t_dirs {
  struct t_dirs *left, *right;
  time_t mtime;			/* dir's modification time */
  struct t_tree *files;
  char name[OVERRUN+1];         // [HGM] ditto
//  char name;			/* ditto */
};

static char **t_buffer = NULL; /* pointer to next space in return buffer */
static int t_buffersize = 0;	/* size of return buffer */

/* fill t_buffer with anything matching "want*" in file tree t_tree */
static void t_sft(const char *want, struct t_tree *t)
{
  if (t) {
    int cmp = strncmp(want, &t->name, strlen(want));
    if (cmp <= 0)		/* if want <= this one, look left */
      t_sft(want, t->left);
    if (t_buffersize && (cmp == 0)) {	/* if a match, add it to buffer */
      t_buffersize--;
      *t_buffer++ = &(t->name);
    }
    if (cmp >= 0)		/* if want >= this one, look right */
      t_sft(want, t->right);
  }
}

/* delete file tree t_tree */
static void t_cft(struct t_tree **t)
{
  if (*t) {
    t_cft(&(*t)->left);
    t_cft(&(*t)->right);
    free(*t);
    *t = NULL;
  }
}

/* make file tree for dir d */
static void t_mft(struct t_dirs *d)
{
  DIR *dirp;
  struct dirent *dp;
  struct t_tree **t;

  if ((dirp = opendir(&(d->name))) == NULL) {
    d_printf( "CHESSD:mft() couldn't opendir.\n");
  } else {
    while ((dp = readdir(dirp))) {
      t = &d->files;
      if (dp->d_name[0] != '.') {	/* skip anything starting with . */
	while (*t) {
	  if (strcmp(dp->d_name, &(*t)->name) < 0) {
	    t = &(*t)->left;
	  } else {
	    t = &(*t)->right;
	  }
	}
	*t = malloc(sizeof(struct t_tree) - OVERRUN + strlen(dp->d_name));
	(*t)->right = (*t)->left = NULL;
	strcpy(&(*t)->name, dp->d_name);
      }
    }
    closedir(dirp);
  }
}

int search_directory(const char *dir, const char *filter, char **buffer, int buffersize)
{
/* dir = directory to search
   filter = what to search for
   buffer = where to store pointers to matches
   buffersize = how many pointers will fit inside buffer */

  static struct t_dirs *ramdirs = NULL;
  struct t_dirs **i;
  int cmp;
  static char nullify = '\0';
  struct stat statbuf;

  t_buffer = buffer;
  t_buffersize = buffersize;

  if (!stat(dir, &statbuf)) {
    if (filter == NULL)		/* NULL becomes pointer to null string */
      filter = &nullify;
    i = &ramdirs;
    while (*i) {			/* find dir in dir tree */
      cmp = strcmp(dir, &(*i)->name);
      if (cmp == 0)
	break;
      else if (cmp < 0)
	i = &(*i)->left;
      else
	i = &(*i)->right;
    }
    if (!*i) {				/* if dir isn't in dir tree, add him */
      *i = malloc(sizeof(struct t_dirs) - OVERRUN + strlen(dir)); // [HGM] allocate just enough, even though struct promoised more
      (*i)->left = (*i)->right = NULL;
      (*i)->files = NULL;
      strcpy(&(*i)->name, dir);
    }
    if ((*i)->files) {			/* delete any obsolete file tree */
      if ((*i)->mtime != statbuf.st_mtime) {
	t_cft(&(*i)->files);
      }
    }
    if ((*i)->files == NULL) {		/* if no file tree for him, make one */
      (*i)->mtime = statbuf.st_mtime;
      t_mft(*i);
    }
    t_sft(filter, (*i)->files);		/* finally, search for matches */
  }
  return (buffersize - t_buffersize);
}

int display_directory(int p, char **buffer, int count)
/* buffer contains 'count' string pointers */
{
	struct player *pp = &player_globals.parray[p];
	int i;
	multicol *m = multicol_start(count);

	for (i = 0; (i < count); i++)
		multicol_store(m, *buffer++);
	multicol_pprint(m, p, pp->d_width, 1);
	multicol_end(m);
	return (i);
}

void CenterText (char *target, const char *text, int width, int pad)
{
  int left, len;
  char *format;

  len = strlen(text);
  left = (width + 1 - len) / 2;    /* leading blank space. */

  if (pad)
    asprintf (&format, "%%%ds%%-%ds", left, width-left);  /* e.g. "%5s%-10s" */
  else
    asprintf (&format, "%%%ds%%s", left);    /* e.g. "%5s%s" */
  sprintf (target, format, "", text);

  free(format);

  return;
}

/* block a signal */
void block_signal(int signum)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set,signum);
	sigprocmask(SIG_BLOCK,&set,NULL);
}

/* unblock a signal */
void unblock_signal(int signum)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set,signum);
	sigprocmask(SIG_UNBLOCK,&set,NULL);
}


int file_copy(const char *src, const char *dest)
{
  int fd1, fd2, n;
  char buf[1024];

  fd1 = open(src, O_RDONLY);
  if (fd1 == -1) return -1;

  unlink(dest);
  fd2 = open(dest, O_WRONLY|O_CREAT|O_EXCL, 0644);
  if (fd2 == -1) {
    close(fd1);
    return -1;
  }

  while ((n = read(fd1, buf, sizeof(buf))) > 0) {
    if (write(fd2, buf, n) != n) {
      close(fd2);
      close(fd1);
      unlink(dest);
      return -1;
    }
  }

  close(fd1);
  close(fd2);
  return 0;
}


#ifndef HAVE_DPRINTF
 int dprintf(int fd, const char *format, ...)
{
	va_list ap;
	char *ptr = NULL;
	int ret = 0;

	va_start(ap, format);
	vasprintf(&ptr, format, ap);
	va_end(ap);

	if (ptr) {
		ret = write(fd, ptr, strlen(ptr));
		free(ptr);
	}

	return ret;
}
#endif


#ifndef HAVE_STRNLEN_X
/*
  some platforms don't have strnlen
*/
size_t strnlen(const char *s, size_t n)
{
	int i;
	for (i=0; s[i] && i<n; i++) /* noop */ ;
	return i;
}
#endif

/* day as a string */
const char *strday(time_t *t)
{
	struct tm *stm = localtime(t);
	static char tstr[100];

	strftime(tstr, sizeof(tstr), "%a %b %e", stm);
	return tstr;
}


static const char *strtime(struct tm * stm, short gmt)
{
	static char tstr[100];

	if (gmt)
		strftime(tstr, sizeof(tstr), "%a %b %e, %H:%M GMT %Y", stm);
	else
		strftime(tstr, sizeof(tstr), "%a %b %e, %H:%M %Z %Y", stm);
	return (tstr);
}

const char *strltime(time_t *clock)
{
	struct tm *stm = localtime(clock);
	return strtime(stm, 0);
}

const char *strgtime(time_t *clock)
{
	struct tm *stm = gmtime(clock);
	return strtime(stm, 1);
}

/* useful debug utility */
void d_printf(const char *fmt, ...)
{
	va_list ap;
	time_t t = time(NULL);
	fprintf(stderr,"%s ", strltime(&t));
	va_start(ap, fmt);
	vfprintf(stderr,fmt,ap);
	va_end(ap);
}


/* append something to admin.log */
static void admin_printf(const char *fmt, ...)
{
	int fd;
	va_list ap;
	time_t t = time(NULL);

	fd = open("admin.log", O_APPEND | O_CREAT | O_RDWR, 0600);
	if (fd == -1) {
		d_printf("Failed to open admin.log - %s\n", strerror(errno));
		return;
	}

	dprintf(fd,"%s ", strltime(&t));
	va_start(ap, fmt);
	vdprintf(fd,fmt,ap);
	va_end(ap);

	close(fd);
}

/*
  log an admin command 
*/
void admin_log(struct player *pp, const char *command, param_list params)
{
	char *s;
	char *s2;
	int i;

	asprintf(&s, "%s: %s", pp->login, command);

	if (!s) return;
	for (i=0; params[i].type != TYPE_NULL; i++) {
		switch (params[i].type) {
		case TYPE_INT:
			asprintf(&s2, "%s %d", s, params[i].val.integer);
			break;
		case TYPE_STRING:
			asprintf(&s2, "%s %s", s, params[i].val.string);
			break;
		case TYPE_WORD:
			asprintf(&s2, "%s %s", s, params[i].val.word);
			break;
		}

		free(s);
		s = s2;
		if (!s) return;
	}

	admin_printf("%s\n", s);
	free(s);
}

/*
  save a lump of data into a file. 
  return 0 on success
*/
int file_save(const char *fname, void *data, size_t length)
{
	int fd;
	unlink(fname);
	fd = open(fname, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd == -1) {
		return -1;
	}
	if (write(fd, data, length) != length) {
		close(fd);
		unlink(fname);
		return -1;
	}
	close(fd);
	return 0;
}

/*
  load a file into memory from a fd.
*/
char *fd_load(int fd, size_t *size)
{
	struct stat sbuf;
	char *p;

	if (fstat(fd, &sbuf) != 0) return NULL;

	p = (char *)malloc(sbuf.st_size+1);
	if (!p) return NULL;

	if (pread(fd, p, sbuf.st_size, 0) != sbuf.st_size) {
		free(p);
		return NULL;
	}
	p[sbuf.st_size] = 0;

	if (size) *size = sbuf.st_size;

	return p;
}

/*
  load a file into memory
*/
char *file_load(const char *fname, size_t *size)
{
	int fd;
	char *p;

	if (!fname || !*fname) return NULL;
	
	fd = open(fname,O_RDONLY);
	if (fd == -1) return NULL;

	p = fd_load(fd, size);
	close(fd);

	return p;
}

/*
  this is like fgets() but operates on a file descriptor
*/
char *fd_gets(char *line, size_t maxsize, int fd)
{
	char c;
	int n = 0;
	while (n < (maxsize-1) && read(fd, &c, 1) == 1) {
		line[n++] = c;
		if (c == '\n') break;
	}
	line[n] = 0;
	return n?line:NULL;
}


/*
  like fopen() but uses printf style arguments for the file name
*/
FILE *fopen_p(const char *fmt, const char *mode, ...)
{
	char *s = NULL;
	FILE *f;
	va_list ap;
	va_start(ap, mode);
	vasprintf(&s, fmt, ap);
	va_end(ap);
	if (!s) return NULL;
	if (strstr(s, "..")) {
		d_printf("Invalid filename [%s]\n", s);
		free(s);
		return NULL;
	}
	f = fopen(s, mode);
	if (f == NULL) {
		d_printf("Could not open file \"%s\" [%s]\n", s, strerror(errno));
	}
	free(s);
	return f;
}

/*
  like fopen() but doesn't allow opening of filenames containing '..'
*/
FILE *fopen_s(const char *fname, const char *mode)
{
	return fopen_p("%s", mode, fname);
}


#ifndef HAVE_STRLCPY
/**
 * Like strncpy but does not 0 fill the buffer and always null 
 * terminates.
 *
 * @param bufsize is the size of the destination buffer.
 *
 * @return index of the terminating byte.
 **/
 size_t strlcpy(char *d, const char *s, size_t bufsize)
{
	size_t len = strlen(s);
	size_t ret = len;
	if (bufsize <= 0) return 0;
	if (len >= bufsize) len = bufsize-1;
	memcpy(d, s, len);
	d[len] = 0;
	return ret;
}
#endif


/*
  set an integer value in a TDB
*/
int tdb_set_int(TDB_CONTEXT *tdb, const char *name, int value)
{
	TDB_DATA key, data;
	int ret;

	key.dptr = strdup(name);
	key.dsize = strlen(name)+1;

	data.dptr = (char *)&value;
	data.dsize = sizeof(value);

	ret = tdb_store(tdb, key, data, TDB_REPLACE);
	free(key.dptr);
	return ret;
}

/*
  get an integer value from a TDB. Return def_value if its not there
*/
int tdb_get_int(TDB_CONTEXT *tdb, const char *name, int def_value)
{
	TDB_DATA key, data;
	int ret;

	key.dptr = strdup(name);
	key.dsize = strlen(name)+1;

	data = tdb_fetch(tdb, key);
	free(key.dptr);

	if (!data.dptr) {
		return def_value;
	}

	ret = *(int *)data.dptr;
	free(data.dptr);

	return ret;
}


/*
  set a string value in a TDB
*/
int tdb_set_string(TDB_CONTEXT *tdb, const char *name, const char *value)
{
	TDB_DATA key, data;
	int ret;

	key.dptr = strdup(name);
	key.dsize = strlen(name)+1;

	data.dptr = strdup(value);
	data.dsize = strlen(value)+1;

	ret = tdb_store(tdb, key, data, TDB_REPLACE);
	free(key.dptr);	
	free(data.dptr);
	return ret;
}

/*
  get a string value from a TDB. Caller frees.
*/
const char *tdb_get_string(TDB_CONTEXT *tdb, const char *name)
{
	TDB_DATA key, data;

	key.dptr = strdup(name);
	key.dsize = strlen(name)+1;

	data = tdb_fetch(tdb, key);
	free(key.dptr);	
	if (!data.dptr) {
		return NULL;
	}
	return data.dptr;
}

/*
  delete a value from a TDB by string
*/
int tdb_delete_string(TDB_CONTEXT *tdb, const char *name)
{
	TDB_DATA key;
	int ret;

	key.dptr = strdup(name);
	key.dsize = strlen(name)+1;

	ret = tdb_delete(tdb, key);
	free(key.dptr);
	return ret;
}
