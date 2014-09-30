TEMPLATE = subdirs
SUBDIRS = common \
          qtclient \
	  server \
	  cliplogcvt
qtclient.depends = common
server.depends = common
