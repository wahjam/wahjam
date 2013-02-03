TEMPLATE = subdirs
SUBDIRS = common \
          qtclient \
	  server
qtclient.depends = common
server.depends = common
