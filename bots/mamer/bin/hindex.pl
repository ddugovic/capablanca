#!/usr/local/bin/perl

$dirpath = "/home/mlong/mamer/help/";

opendir(FILELIST, "$dirpath");
@allfiles = readdir(FILELIST);
closedir(FILELIST);

open(OUT, ">$dirpath/index");
$i = 0;
$j = 0;
$offset = $#allfiles/4;
@allfiles = sort @allfiles;
foreach $a (@allfiles) {
    if($allfiles[$i] !~ /\./) {
	$temp[$j] = $allfiles[$i];
	$j++;
    }
    $i++;
}
@allfiles = @temp;
$i = 0;
#print OUT "$#allfiles files listed\n";
while ($i <= ($#allfiles/4)) {
    printf OUT 
	(" %-18s %-18s %-18s %-18s\n", 
	 $allfiles[$i], $allfiles[$i+($offset*1)],
	 $allfiles[$i+($offset*2)], $allfiles[$i+($offset*3)]);
    $i++;
}
#print OUT "\n";
close(OUT);
