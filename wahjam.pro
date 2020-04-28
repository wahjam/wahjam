TEMPLATE = subdirs

# Build everything by default
!qtclient:!wahjamsrv:!cliplogcvt {
        CONFIG += common qtclient wahjamsrv cliplogcvt
}

qtclient {
        SUBDIRS += common qtclient
}
wahjamsrv {
        SUBDIRS += common server
}
cliplogcvt {
        SUBDIRS += cliplogcvt
}
qtclient.depends = common
server.depends = common
