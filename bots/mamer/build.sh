# First, create AUTHORS, NEWS, README, and ChangeLog.
# Second, create configure.ac and makefile.am
# Finally, run this script
autoreconf -fi
./configure "$@"
make clean
make -f Makefile -j2
