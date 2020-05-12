TEMPLATE = lib
CONFIG += staticlib
QT += network
QT -= gui

SOURCES = mpb.cpp \
          netmsg.cpp \
          njmisc.cpp \
          UserPrivs.cpp
HEADERS = mpb.h \
          netmsg.h \
          njmisc.h \
          UserPrivs.h \
          ConcurrentQueue.h
