win32 {
    CONFIG(release, debug|release) {
        common_path = ../common/release
    } else {
        common_path = ../common/debug
    }
} else {
    common_path = ../common
}

LIBS += -L$${common_path} -lcommon
PRE_TARGETDEPS += $${common_path}/libcommon.a
QMAKE_CXXFLAGS += -Wno-write-strings
