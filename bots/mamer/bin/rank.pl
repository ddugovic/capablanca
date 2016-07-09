#!/usr/local/bin/perl

$PLAYER_DATA_PATH = "/home/mlong/mamer/players";
$OUTFILE = "/home/mlong/mamer/data/rank.new";

# you should not have to change this its just where it belongs
@dirList = ("a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", 
	    "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "_");

&main();

sub main {
    foreach $a (@dirList) {
        if(-d "$PLAYER_DATA_PATH/$a") {
#	    printf("$PLAYER_DATA_PATH/$a\n");
            opendir(THISDIR, "$PLAYER_DATA_PATH/$a");
            @tempdir = sort(readdir(THISDIR));
            close(THISDIR);
	    
            foreach $file (@tempdir) {
		if(!(-d "$PLAYER_DATA_PATH/$a/$file")) {
#		    printf ("Converting %s %s\n", $file, $PLAYER_DATA_PATH);
		    open(IN, "$PLAYER_DATA_PATH/$a/$file") || die "Can't open $file\n";
		    while($line = <IN>) {
			chop($line);
			($n, $abuse, $tourneys, $w, $l, $d, $fsts, $scds, $thds, $rating, 
			 $placePoints, $managerLevel, $managedTourneys, $last, $tourneyLocation) = split(/ /, $line);
			if($tourneys > 4) {
			    $rank{$n} = "$rating $n $tourneys";
			}
		    }
		    close(IN);
		}
	    }
	}
    }
    $i = 0;
    $last = 0;
    foreach $a (reverse (sort (values %rank))) {
	($b, $c, $d) = split(/ /, $a);
	if($b != $last) {
	    $i++;
	    $last = $b;
	}
	open(OUT, ">>$OUTFILE") || die "Can't open $OUTFILE\n";
	printf OUT ("%-18s %d %4.2f\n", $c, $d, $b);
	close(OUT);
    }
}

