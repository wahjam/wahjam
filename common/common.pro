TEMPLATE = lib
CONFIG += staticlib
QT += network
QT -= gui

QMAKE_CXXFLAGS += -Wno-write-strings
SOURCES = mpb.cpp \
          netmsg.cpp \
          njmisc.cpp \
          UserPrivs.cpp
HEADERS = mpb.h \
          netmsg.h \
          njmisc.h \
          UserPrivs.h \
          ConcurrentQueue.h
