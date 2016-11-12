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
  ficsmain.c is a minimal wrapper around chessd.so. By keeping this module to a
  minimum size we can reload nearly all the chess server functionality on a 'areload'
  command. 
*/


#include "includes.h"

/* handle used to talk to chessd.so */
static void *chessd_so_handle;

static void usage();
static void main_event_loop(void);

unsigned chessd_reload_flag;

/* 
   load a function from chessd.so 
   this is the core of the runtime reload functionality
*/
static void *chessd_function(const char *name)
{
	void *sym;

	if (!chessd_so_handle) {
		chessd_so_handle = dlopen(LIB_DIR "/chessd.so", RTLD_LAZY);
		if (!chessd_so_handle) {
			fprintf(stderr, "CHESSD: Failed to load chessd.so !! (%s)\n",
				dlerror());
			exit(1);
		}
	}

	sym = dlsym(chessd_so_handle, name);
	if (!sym) {
		fprintf(stderr, "CHESSD: Failed to find symbol %s in chessd.so !!\n",
			name);
		exit(1);
	}

	return sym;
}


static void usage()
{
	fprintf(stderr,  "Usage: chessd -p port [-f] [-T timeseal_decoder] [-R rootdir]\n");
	fprintf(stderr,  "PORT_NUMBER is 5000 by default\n");
	exit(1);
}

static int daemonize(void)
{
	pid_t pid;
	if ((pid = fork()) == -1) 
		return -1;
	else if (pid != 0)
		exit(0);
	if (setsid() == -1)
		return -1;
	return 0;
}

static void main_event_loop(void) 
{
	void (*select_loop)(void ) = chessd_function("select_loop");

	while (1) {
		select_loop();

		/* check the reload flag */
		if (chessd_reload_flag) {
			void (*reload_close)(void ) = chessd_function("reload_close");
			void (*reload_open)(void );

			chessd_reload_flag = 0;

			fprintf(stderr, "CHESSD: Reloading server code!\n");

			/* free up local vars */
			reload_close();

			/* close the handle to the shared lib */
			dlclose(chessd_so_handle);
			chessd_so_handle = NULL;

			/* re-initialise local variables */
			reload_open = chessd_function("reload_open");
			reload_open();
			select_loop = chessd_function("select_loop");
		}
	}
}

static void TerminateServer(int sig)
{
	void (*output_shut_mess)(void ) = chessd_function("output_shut_mess");
	void (*TerminateCleanup)(void ) = chessd_function("TerminateCleanup");
	void (*net_close)(void ) = chessd_function("net_close");

	fprintf(stderr,  "CHESSD: Received signal %d\n", sig);
	output_shut_mess();
	TerminateCleanup();
	net_close();
	exit(1);
}

static void BrokenPipe(int sig)
{
	signal(SIGPIPE, BrokenPipe);
	fprintf(stderr,  "CHESSD: Pipe signal\n");
}


/* this assumes we are setuid root to start */
static void do_chroot(const char *dir)
{
	int i;
	uid_t uid = getuid();
	uid_t euid = geteuid();
	struct rlimit rlp;

	if (euid != 0 || setuid(0) != 0) {
		fprintf(stderr, "Must be setuid root to use -R\n");
		exit(1);
	}
	if (chroot(dir) != 0 || chdir("/") != 0) {
		perror("chroot");
		exit(1);
	}
	if (setuid(uid) != 0) {
		perror("setuid");
		exit(1);
	}

	/* close all extraneous file descriptors */
	for (i=0;i<3;i++) close(i);
	open("stdin.log", O_RDWR|O_CREAT|O_TRUNC, 0644);
	open("stdout.log", O_WRONLY|O_CREAT|O_TRUNC, 0644);
	open("stderr.log", O_WRONLY|O_CREAT|O_TRUNC, 0644);

	/* reestabish core limits */
	getrlimit(RLIMIT_CORE, &rlp );
	rlp.rlim_cur = MAX(64*1024*1024, rlp.rlim_max);
	setrlimit( RLIMIT_CORE, &rlp );
}

int main(int argc, char *argv[])
{
	int i, foreground, port;
	void (*timeseal_init)(const char * ) = chessd_function("timeseal_init");
	int (*net_init)(int ) = chessd_function("net_init");
	void (*initial_load)(void ) = chessd_function("initial_load");

	port = DEFAULT_PORT;
	foreground = 0;

	/* enable malloc checking in libc */
	setenv("MALLOC_CHECK_", "2", 1);
	
	while((i = getopt(argc, argv, "p:fR:T:")) != -1) {
		switch(i) {
		case 'p':
			port = atoi(optarg);
			break;
		case 'f':
			foreground = 1;
			break;
		case 'T':
			timeseal_init(optarg);
			break;
		case 'R':
			do_chroot(optarg);
			break;
		default:
			usage();
		}
	}
   
	if (!foreground && daemonize()){
		printf("Problem with Daemonization - Aborting\n");
		exit(1);
	}  

	signal(SIGTERM, TerminateServer);
	signal(SIGINT, TerminateServer);
	signal(SIGPIPE, BrokenPipe);

	if (net_init(port)) {
		fprintf(stderr, "CHESSD: Network initialize failed on port %d.\n", port);
		exit(1);
	}
	fprintf(stderr,  "CHESSD: Initialized on port %d\n", port);

	initial_load();
	
	main_event_loop();
	/* will never get here - uses TerminateServer */
	
	return 0;
}
