#!/usr/local/bin/perl

@dirList = ("a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", 
	    "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z");

foreach $i (@dirList) {
    opendir(THISDIR, "/Users/mlong/mamer/players/$i");
    @tempdir = sort(readdir(THISDIR));
    close(THISDIR);
    
    foreach $file (@tempdir) {
##	print ("FILE = :$file:\n");
	if($file !~ /\.$/) {
	    if($file =~ /\s*/) {
		print("unlink :$file :\n");
		unlink("/Users/mlong/mamer/players/$i/$file");
	    }
	}
    }
}
