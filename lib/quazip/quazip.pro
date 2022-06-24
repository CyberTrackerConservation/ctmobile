LIB_NAME=quazip
TARGET = $$LIB_NAME
TEMPLATE = lib
#load(qt_build_config)
#MODULE_VERSION = 1.0
#load(qt_module)
QT -= gui
#!mingw:VERSION=1.0.0
CONFIG += staticlib

windows{
    ZLIB_PWD = $$PWD/zlib/
    INCLUDEPATH += $$ZLIB_PWD/
    DEPENDPATH += $$ZLIB_PWD/
#    warning ("Inscluded Path: " $$ZLIB_PWD/zlib-1.2.8/);
}else{
    LIBS += -lz
}

# This one handles dllimport/dllexport directives.
DEFINES += QUAZIP_BUILD

## You'll need to define this one manually if using a build system other
## than qmake or using QuaZIP sources directly in your project.
CONFIG(staticlib): DEFINES += QUAZIP_STATIC

include($$PWD/quazip.pri)
