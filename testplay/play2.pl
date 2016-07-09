#!/usr/bin/perl
#
# Quick and nasty script to load a game record and play it 
# through via a FICS server.
#
# Anthony Wesley and Andrew Tridgell, July 2004
#
# Usage: play2.pl server client1/passwd1 client2/passwd2
#
# Reads game records on stdin.
#

use IO::Socket;

my $server = shift @ARGV;
my ($client1,$pass1) = split(/\//, shift @ARGV);
my ($client2,$pass2) = split(/\//, shift @ARGV);

print "Connecting to $server , port 5000\n";
print "[$client1 / $pass1 ] , [$client2 / $pass2 ]\n";

my $S1 = new IO::Socket::INET(
			      PeerAddr => $server,
			      PeerPort => 5000,
			      Proto => 'tcp',
			      ReuseAddr => 1
			      ) or die "Could not connect";

my $S2 = new IO::Socket::INET(
			      PeerAddr => $server,
			      PeerPort => 5000,
			      Proto => 'tcp',
			      ReuseAddr => 1
			      ) or die "Could not connect";

$S1->setsockopt(&Socket::IPPROTO_TCP, &Socket::TCP_NODELAY, 1) || die;
$S2->setsockopt(&Socket::IPPROTO_TCP, &Socket::TCP_NODELAY, 1) || die;

print "Connection opened. Logging in...\n";

Login($S1,$client1,$pass1) or die "Could not login $client1";
Login($S2,$client2,$pass2) or die "Could not login $client2";

sleep(3);

print "Logged in\n"; 

# ensure we don't have any left over games
$S1->print("resign $client2\n");

print "Reading game record from stdin...\n";
my $game = "";
my $game_comment = "";
my $game_num = 0;

while(my $line = <STDIN>) {
	chomp $line;
	if ($line =~ /^\[/ || $line eq "") {
		if ($game eq "") {
			$game_comment .= "$line\n";
			next;
		}
		$game = extract_moves($game);
		$game_num++;
		print "Starting game $game_num\n";
		PlayGame($S1,$S2,$game);
		$game = "";
		$game_comment = "";
		next;
	}
	$game .= $line . " ";
}

sub PlayGame($$$)
{
	my $s1 = shift;
	my $s2 = shift;
	my $game = shift;

	Challenge($s1,$client1,$s2,$client2) or die "Could not setup game";

	my $res = Play($s1, $s2, $game);
	if ($res ne "OK") {
		$s1->print("resign\n");

		print "Writing bad game - $res\n";

		local(*FILE);
		open(FILE, ">>badgames.txt") || die "can't open badgames.txt";    
		print FILE "[$res]\n$game\n\n";
		close(FILE);
	}

	WaitFor($s1, "$client1 vs. $client2");
	WaitFor($s2, "$client1 vs. $client2");

	WaitFor($s1, "No ratings adjustment done");
	WaitFor($s2, "No ratings adjustment done");
}


sub Login($$$)
{
	my $s = shift;
	my $name = shift;
	my $pass = shift;

	$s->print($name . "\n");
	$s->print($pass . "\n");
	$s->print("set style 12\n\n");

	return 1;
}

sub extract_moves($)
{
	my $txt = shift;
	my $moves = "";
	
	my @lines = split(/[\n\r]+/, $txt);
	foreach my $l (@lines) {
		# Strip
		$l =~ s/^\s+//; $l =~ s/\s+$//;
		
		# Maybe metadata?
		next if $l =~ /^\s+\[/;
		
		# Maybe contains moves?
		$moves .= $l . " ";
	}

	# Strip out the move numbers
#	$moves =~ s/\d+\.\s+//g;

	return $moves;
}

sub Challenge($$$$)
{
	my $s1 = shift;  	# socket for client1
	my $client1 = shift;	# username for initiator
	my $s2 = shift;		# socket for receipient
	my $client2 = shift;	# name of receipient

	$s1->print("match $client2 1 1 w u\n");

	WaitFor($s1, "Issuing: $client1");
	WaitFor($s2, "Challenge: $client1");
	WaitFor($s2, "You can \"accept\"");
	$s2->print("accept\n");
	WaitFor($s2,"You accept the challenge of $client1");
	WaitFor($s1,"$client2 accepts your challenge");
	WaitFor($s1,"Creating: $client1");
	WaitFor($s1,".$client1 vs. $client2. Creating");
	WaitFor($s2,"Creating: $client1");
	WaitFor($s2,".$client1 vs. $client2. Creating");

	return 1;
}

sub Play($$$)
{
	my $s1 = shift;
	my $s2 = shift;
	my $game = shift;
	
	my $who_to_move = "w";
	my @moves = split(/\s+/, $game);
	
	my $movenum = 1;
	my $res = "OK";

	CheckMove($s1,"none") || return "Initialisation failed for white";
	CheckMove($s2,"none") || return "Initialisation failed for black";
	
	foreach my $m (@moves) {
		next if ($m =~ /^\d/);
		last if ($m =~ /^\[/);
		my $p1, $p2;

		if ($who_to_move eq "w") {
			$p1 = $s1;
			$p2 = $s2;
#			print "White moves $movenum. $m\n";
		} else {
			$p1 = $s2;
			$p2 = $s1;
#			print "Black moves $movenum ... $m\n";
		}

		SendMove($p1,$m);

		$res = CheckMove($p1,$m);
		if ($res ne "OK") {
			return "Failed on move $movenum - $res";
		}
		
		$res = CheckMove($p2,$m);
		if ($res ne "OK") {
			return "Failed on move $movenum - $res";
		}
		
		# Change sides
		if ($who_to_move eq "b") { 
			$movenum++;
			$who_to_move = "w"; 
		} else { 
			$who_to_move = "b"; 
		}
	}
	
	# Resign
	if ($who_to_move eq "w") { 
		Resign($s1); 
	} else { 
		Resign($s2); 
	}

	return "OK";
}

sub SendMove($$)
{
	my $s = shift;
	my $m = shift;

	$s->print($m . "\n");

	return 0;
}

sub CheckMove($$)
{
	my $s = shift;
	my $m = shift;

	# Readback to check
	while ($l = $s->getline) {
		chomp $l;
#		print "line3=\n[[[$l\n]]]\n";
		if ($l =~ /^\s+fics%\s+$/) {
			next;
		}
		if ($l =~ /^\s+$/) {
			next;
		}
		if ($l =~ /Illegal Move/i) {
			return "Server reports illegal move $m";
		}
		if ($l =~ /Ambiguous Move/i) {
			return "Server reports ambiguous move $m";
		}
		my $rmove = ParseStyle12($l);
		if ($rmove eq "") {
			print "$l\n";
			next;
		}
		if (!MoveEqual($rmove,$m)) {
			return "wrong RecvMove $rmove should be $m";
		}
		return "OK";
	}

	return "eof from server";
}

sub ParseStyle12($)
{
	my $s = shift;
	my $m = "";
	if ($s =~ /\<12\>.*\(\d+\:\d\d\)\s(\S+)/m) {
		$m = $1;
#		print "Move=$m\n";
	}
	return $m;
}

sub Resign($)
{
	my $s = shift;

	$s->print("resign\n");
}

sub MoveEqual($$)
{
	my $m1 = shift;
	my $m2 = shift;

	# remove check and good move markers
	if ($m1 =~ /^(.*)[+!]/) {
		$m1 = $1;
	}

	if ($m2 =~ /^(.*)[+!]/) {
		$m2 = $1;
	}

	if ($m1 eq $m2) {
		return 1;
	}

	return 0;
}
		
sub WaitFor($$)
{
	my $s = shift;
	my $str = shift;


	while (my $l = $s->getline) {
		chomp $l;
		if ($l =~ /$str/) {
#			print "GOT: $str\n";
			return;
		}
		if ($l =~ /^\s+fics%\s+$/) {
			next;
		}
		if ($l =~ /^\s+$/) {
			next;
		}
		print "$l\n";
	}
}
