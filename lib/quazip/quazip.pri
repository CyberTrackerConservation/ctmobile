INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
CONFIG += QUAZIP

DEFINES += _CRT_SECURE_NO_WARNINGS

# This one handles dllimport/dllexport directives.
DEFINES += QUAZIP_BUILD

## You'll need to define this one manually if using a build system other
## than qmake or using QuaZIP sources directly in your project.
CONFIG(staticlib): DEFINES += QUAZIP_STATIC

*msvc* {
PRECOMPILED_HEADER = $$PWD/pch.h
CONFIG += precompile_header
}

windows {
    ZLIB_PWD = $$PWD/zlib/
    INCLUDEPATH += $$ZLIB_PWD/
    DEPENDPATH += $$ZLIB_PWD/

    HEADERS += $$ZLIB_PWD/zlib.h
    SOURCES += \
        $$ZLIB_PWD/adler32.c \
        $$ZLIB_PWD/crc32.c \
        $$ZLIB_PWD/gzclose.c \
        $$ZLIB_PWD/gzread.c \
        $$ZLIB_PWD/gzwrite.c \
        $$ZLIB_PWD/gzlib.c \
        $$ZLIB_PWD/inflate.c \
        $$ZLIB_PWD/inftrees.c \
        $$ZLIB_PWD/infback.c \
        $$ZLIB_PWD/inffast.c \
        $$ZLIB_PWD/deflate.c \
        $$ZLIB_PWD/zutil.c \
        $$ZLIB_PWD/trees.c
} else {
    LIBS += -lz
}

android {
    QMAKE_CXXFLAGS += -Wno-deprecated
}

HEADERS += \
    $$PWD/ExtractorThread.h \
    $$PWD/crypt.h \
    $$PWD/ioapi.h \
    $$PWD/quaadler32.h \
    $$PWD/quachecksum32.h \
    $$PWD/quacrc32.h \
    $$PWD/quagzipfile.h \
    $$PWD/quaziodevice.h \
    $$PWD/quazipdir.h \
    $$PWD/quazipfile.h \
    $$PWD/quazipfileinfo.h \
    $$PWD/quazip_global.h \
    $$PWD/quazip.h \
    $$PWD/quazipnewinfo.h \
    $$PWD/unzip.h \
    $$PWD/zip.h \
    $$PWD/jlcompress.h

SOURCES += \
    $$PWD/ExtractorThread.cpp \
    $$PWD/qioapi.cpp \
    $$PWD/quaadler32.cpp \
    $$PWD/quacrc32.cpp \
    $$PWD/quagzipfile.cpp \
    $$PWD/quaziodevice.cpp \
    $$PWD/quazip.cpp \
    $$PWD/quazipdir.cpp \
    $$PWD/quazipfile.cpp \
    $$PWD/quazipfileinfo.cpp \
    $$PWD/quazipnewinfo.cpp \
    $$PWD/unzip.c \
    $$PWD/zip.c \
    $$PWD/jlcompress.cpp
