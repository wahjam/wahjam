#!/bin/sh

export TARFILE=ninjam/cocoaclient/cocoaclientsource.tar


cd ../..

tar cvf $TARFILE WDL/jnetlib/*.cpp WDL/jnetlib/*.h WDL/sha.cpp WDL/sha.h WDL/rng.cpp WDL/rng.h WDL/lineparse.h WDL/ptrlist.h WDL/heapbuf.h WDL/string.h WDL/queue.h WDL/mutex.h WDL/dirscan.h WDL/pcmfmtcvt.h WDL/vorbisencdec.h WDL/wavwrite.h

tar rvf $TARFILE ninjam/mpb.cpp ninjam/mpb.h ninjam/netmsg.cpp ninjam/netmsg.h ninjam/njclient.cpp ninjam/njclient.h ninjam/njmisc.cpp ninjam/njmisc.h 

tar rvf $TARFILE ninjam/audiostream.h ninjam/audiostream_mac.cpp

tar rvf $TARFILE --exclude=\*.tar ninjam/cocoaclient/*

cd ninjam/cocoaclient

