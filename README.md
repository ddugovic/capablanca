[![Build Status](https://travis-ci.org/ddugovic/capablanca.svg?branch=master)](https://travis-ci.org/ddugovic/capablanca)

Introduction
------------

This is a version of the internet chess server which supports a wider
variety of variants, allowing other board sizes than 8x8 and more pieces
than the 6 orthodox piece types. I renamed it 'capablanca', in remembrance
of Jose Capablanca, who rediscovered and popularized Chess on a 10x8 board.
(Which is one of the better known variants now supported, next to Xiangqi
and Shogi.) It was derived from 'lasker', of which the original README is
shown below.

It also fixes some bugs that remained in the original code.

H.G. Muller

---------------------------------------------------------------------

This is an enhanced version of the 'lasker' internet chess server. I
started to enhance Lasker when I needed a working chess server for a
local school. You can download the original 'lasker' release from
http://chessd.sourceforge.net/ and my enhanced version from
http://chess.samba.org/

Here is a list of some of the new features in this version:

     - lots and lots of bugs fixed
     - timeseal support added (see timeseal/README for details)
     - server configuration via 'aconfig' command instead of config.h
     - added multi-part command parsing (commands separated by ';')
     - enhanced aliases
     - build/install process fixed (32-bit binary on 64-bit architecture)
     - fixed help system
     - transparent server reload (upgrade without disturbing connections or games)


Installation
------------

First you need to configure and compile the chessd server. Do
something like the following:

	  cd src
	  ./configure --host=i686-linux-gnu "CFLAGS=-m32" "CXXFLAGS=-m32" "LDFLAGS=-m32" "LTCC=gcc -m32" --prefix=/usr/local
	  make

Then to install the server run "make install". That will install a
basic skeleton installation in /usr/local/chessd. 

Setting up
----------

Next you will want to launch your chess server using the command:
     bin/chessd -p 5000
while in the chessd/ directory. This will launch the chess server
using the skeleton data you installed above.

I highly recommend creating a separate account on your machine to run
the chess daemon. This user should own all files in the chessd
directory.

After you launch chessd for the first time you will need to login as
the special user 'admin'. That username will be recognised as the
server administrator and you will be logged in with the rather unusual
combination of a head administrator who is also an unregistered
player.

The first thing you will want to do as the admin user is create a
proper 'admin' account with a password. Use the command 'addplayer'
while logged in as the admin user to create the admin account. You
will be told the password. Then you should immediately logout and log
back in using the admin password you have just been given. You will
probably want to change this password using the 'asetpass' command. 

You may also find the following commands useful:
    ahelp addplayer
    ahelp asetpass
    ahelp commands


Securing your server
--------------------

The source code for this chess server has been hacked on by dozens of
people over the years. It almost certainly has exploitable buffer
overruns or other security holes. If you are like me then you won't
like the idea of running an insecure program like that on your server.

To make it a lot more secure you can choose to run the chess server in
a chroot jail. This makes it much harder for an attacker to gain a
foothold on your server. It won't prevent them from crashing the
chessd process but it will prevent them from gaining access to other
parts of the system.

To run chessd in a chroot jail you need to do the following:

   1) chessd needs to be setuid root. I know this sounds bizarre, but
      it needs root privileges to use the chroot() system
      call. Immediately after the chroot chessd will permanently lose
      its root privileges and instead become the user that launched
      chessd. To make chessd setuid root do this as root:
	      chown root chessd
	      chmod u+s chessd

   2) pass the command line option -R to tell chessd that it should
      chroot to the current directory. So to launch chessd you can use
      this:
		chessd -p 5000 -T /usr/local/bin/timeseal_decoder -R /usr/local/chessd

      You may also like to look at the start_chessd script in the
      scripts directory. This is the script I use to keep chessd
      always running on my machine.

If you do use the -R option then I also recommend that you don't place
any of the chess server binaries (or any other binaries or libraries)
inside the chessd directory. That will increase the security of your
server a little.


Email spool
-----------

This chess server does not send emails directly, instead it puts
outgoing emails in the spool/ directory and waits for some external
program or script to deliver them. I designed it this way as it makes
it possible to send email from a chess server in a chroot jail, and
offers more flexibility in how email is handled (as that tends to vary
a lot between systems).

If you run sendmail then the sample script in scripts/spool_sendmail
might be good enough. Just run this script at startup and it will send
all spooled mail every minute.


Server reload
-------------

This version of chessd supports reloading the server code without
having to shutdown the server or disturb any games that are in
progress. This allows for on the fly upgrades, hopefully without the
users even noticing that the system is being upgraded.

In order to support this functionality I had to make the source code
rather more Linux specific than it was previously, but I think that is
worth it for the functionality. It would be possible to port the code
to many other platforms, but I have not yet done so.

To reload the server use the command 'areload'. You must be at the
ADMIN_GOD admin level to issue this command.

License
-------

This chess server release is under the GNU General Public License,
which is the license used by the original chess server written by
Richard Nash. Various parts of the server are under different
licenses, depending on who wrote what part, but I believe that all of
the licenses are compatible with the GPL.

The reason I chose the GPL for this release is that I don't want this
code to become proprietary again. This has happened at least 3 times
in the past with this source code and while I'm sure there were very
good reasons at the time, it does mean that the freely available chess
servers have not benefited from the considerable development that has
happened over the past seven years. 

I also chose the GPL because it allows me to incorporate source code
from other GPLd projects. This saved me quite a lot of time, and is
sure to be useful again.


--------------------------------
Andrew Tridgell
tridge@chess.samba.org June 2002
--------------------------------
