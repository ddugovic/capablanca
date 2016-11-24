#!/usr/local/bin/perl

$PLAYER_DATA_PATH = "/usr/local/chessd/mamer/players";

@dirList = ("a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", 
            "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z");

foreach $i (@dirList) {
    if(-d "$PLAYER_DATA_PATH/$i") {
        opendir(THISDIR, "$PLAYER_DATA_PATH/$i");
        @tempdir = sort(readdir(THISDIR));
        close(THISDIR);
        foreach $file (@tempdir) {
            ##print ("FILE = :$file:\n");
            if($file !~ /\.$/) {
                if($file =~ /\s*/) {
                    print("unlink :$file :\n");
                    unlink("$PLAYER_DATA_PATH/$i/$file");
                }
            }
        }
    }
}
