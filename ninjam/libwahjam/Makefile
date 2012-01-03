DESTDIR=
PREFIX=/usr
INSTALLDIR=$(DESTDIR)$(PREFIX)
#VERSION=$(shell head -n 1 debian/changelog|sed -e "s,^wahjam (\(.*\)) .*,\1,")
VERSION=0.0.1

#############################################################
# CPU optimization section
#############################################################

OPTFLAGS =  -O2

ifdef MAC
OPTFLAGS += -D_MAC -mcpu=7450
LFLAGS = -framework coreaudio -lncurses.5 -lm
else
OPTFLAGS += -pthread
MACH=$(shell uname -m)
ifeq ($(MACH), x86_64)
OPTFLAGS += -fPIC
else
OPTFLAGS += -malign-double
endif
LFLAGS = -lm -lpthread
endif

#############################################################
# Basic Configuration
#############################################################

# we MUST have -fomit-frame-pointer and -lm, otherwise we hate life
CFLAGS = $(OPTFLAGS) -s -I.
# CFLAGS += -Wshadow
CC=gcc
CXX=g++

COMMON_OBJS = ../../WDL/jnetlib/asyncdns.o
COMMON_OBJS += ../../WDL/jnetlib/connection.o
COMMON_OBJS += ../../WDL/jnetlib/listen.o
COMMON_OBJS += ../../WDL/jnetlib/util.o
COMMON_OBJS += ../../WDL/jnetlib/httpget.o
COMMON_OBJS += ../../WDL/rng.o
COMMON_OBJS += ../../WDL/sha.o
COMMON_OBJS += ../mpb.o
COMMON_OBJS += ../netmsg.o

CLIENT_OBJS = ../njclient.o
ifdef MAC
CLIENT_OBJS += ../audiostream_mac.o
else
CLIENT_OBJS += ../audiostream_jack.o
CLIENT_OBJS += ../audiostream_alsa.o
LFLAGS += -ljack -lasound
endif
CLIENT_OBJS += ../njmisc.o

CXXFLAGS = $(CFLAGS)

default: libwahjam-common.so libwahjam-client.so \
	 libwahjam-common.a libwahjam-client.a \
	 wahjam.pc wahjam-client.pc
	
libwahjam-common.so: $(COMMON_OBJS)
	$(CXX) -shared $(CXXFLAGS) -o libwahjam-common.so $(COMMON_OBJS)

libwahjam-client.so: $(CLIENT_OBJS) libwahjam-common.so
	$(CXX) -shared $(CXXFLAGS) -o libwahjam-client.so $(CLIENT_OBJS) \
	$(LFLAGS) -logg -lvorbis -lvorbisenc -L. -lwahjam-common

libwahjam-common.a: $(COMMON_OBJS)
	ar cr libwahjam-common.a $(COMMON_OBJS)

libwahjam-client.a: $(CLIENT_OBJS) libwahjam-common.a
	ar cr libwahjam-client.a $(CLIENT_OBJS) libwahjam-common.a

%.pc: %.pc.in
	sed 's,^\(prefix=\).*,\1$(PREFIX),; s,^\(Version: \).*,\1$(VERSION),' $< >$@

clean:
	-rm $(COMMON_OBJS) $(CLIENT_OBJS) \
	libwahjam-common.so libwahjam-client.so \
	libwahjam-common.a libwahjam-client.a \
	wahjam.pc wahjam-client.pc

install: libwahjam-common.so libwahjam-client.so libwahjam-common.a libwahjam-client.a \
	 wahjam.pc wahjam-client.pc
	mkdir -p $(INSTALLDIR)/include/libwahjam/wahjam
	mkdir -p $(INSTALLDIR)/include/libwahjam/WDL
	mkdir -p $(INSTALLDIR)/include/libwahjam/WDL/jnetlib
	mkdir -p $(INSTALLDIR)/lib
	mkdir -p $(INSTALLDIR)/lib/pkgconfig
	install -m 644 wahjam/*.h $(INSTALLDIR)/include/libwahjam/wahjam/
	install -m 644 WDL/*.h $(INSTALLDIR)/include/libwahjam/WDL/
	install -m 644 WDL/jnetlib/*.h $(INSTALLDIR)/include/libwahjam/WDL/jnetlib/
	install *.so $(INSTALLDIR)/lib/
	install -m 755 *.a $(INSTALLDIR)/lib/
	install -m 644 wahjam.pc $(INSTALLDIR)/lib/pkgconfig/
	install -m 644 wahjam-client.pc $(INSTALLDIR)/lib/pkgconfig/
