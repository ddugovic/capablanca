#!make
#
# $Revision: 1.8 $
# $Author: mlong $
#
# $Date: 1998/09/11 15:47:50 $
# 
# $Locker:  $
#

ifeq (${USE_INSURE}, 1)
        CFLAGS = -Wall -ggdb
        C++FLAGS = -Wall -ggdb
        CC = insure
        C++ = insure
else
        ifeq (${USE_CHECKER}, 1)
             CFLAGS = -Wall ##-ggdb
             CC = checker -gcc
             C++ = checker -g++
        else
             CFLAGS = -Wall -ggdb
	     C++FLAGS = -Wall -ggdb
             CC = gcc
             C++ = g++
        endif
endif


#DEFINES = -DSUN

#LDFLAGS = -lsocket -lnls -lnsl

MAMER=mamer
MAMER-HEADERS = config.h Mamer.hh User.hh Tourney.hh link.hh linklist.hh\
	Command.hh CommandEntry.hh TourneyParameters.hh TourneyPlayers.hh\
	Game.hh Player.hh Storage.hh
MAMER-SRCS = main.cc Mamer.cc User.cc Tourney.cc link.cc Command.cc\
	CommandEntry.cc TourneyParameters.cc TourneyPlayers.cc\
	Game.cc Player.cc Storage.cc
MAMER-OBJS = main.o Mamer.o User.o Tourney.o link.o Command.o \
	CommandEntry.o TourneyParameters.o TourneyPlayers.o\
	Game.o Player.o Storage.o

HEADERS = $(MAMER-HEADERS)
SRCS = $(MAMER-SRCS)
OBJS = $(MAMER-OBJS)

default:: $(MAMER)

all:: $(MAMER)

# Object file rules
%.o: %.cc 
	$(C++) $(DEFINES) $(C++FLAGS) -c $<

%.o: %.c
	$(CC) $(DEFINES) $(CFLAGS) -c $<

$(MAMER): $(MAMER-OBJS)
	$(C++) $(C++FLAGS) -o bin/$@ $(MAMER-OBJS) $(LDFLAGS)

clean:
	rm -f $(OBJS) bin/mamer *~ #*

count:
	wc -l $(HEADERS) $(SRCS)

tar:
	make clean
	(cd ..; tar -cf mamer.tar mamer; gzip -f mamer.tar; mv mamer.tar.gz mamer)

bak:
	mv bin/mamer bin/mamer_bak

# File Dependencies

main.o: main.cc\
	Mamer.hh\
	config.h

Mamer.o: Mamer.cc\
	Mamer.hh\
	User.hh\
	Tourney.hh\
	config.h\
	link.hh\
	linklist.hh\
	CommandEntry.hh\
	TourneyParameters.hh\
	Player.hh\
	Game.hh\
	Storage.hh

User.o: User.cc\
	User.hh\
	config.h\
	link.hh

Tourney.o: Tourney.cc\
	Tourney.hh\
	config.h\
	link.hh\
	TourneyParameters.hh\
	TourneyPlayers.hh\
	Player.hh\
	Game.hh\
	Storage.hh

Command.o: Command.cc\
	Command.hh\
	config.h\
	link.hh\
	types.h\
	Game.hh

CommandEntry.o: CommandEntry.cc\
	CommandEntry.hh\
	User.hh\
	types.h\
	Game.hh

TourneyParameters.o: TourneyParameters.cc\
	TourneyParameters.hh

TourneyPlayers.o: TourneyPlayers.cc\
	TourneyPlayers.hh

Storage.o: Storage.cc\
	Storage.hh

Player.o: Player.cc\
	Player.hh

Game.o: Game.cc\
	Game.hh
