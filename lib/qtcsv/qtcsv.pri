CONFIG *= qt
QT *= core

!contains(DEFINES, QTCSV_LIBRARY): DEFINES += QTCSV_MAKE_LIB

INCLUDEPATH += $$PWD/include \
               $$PWD
SOURCES += \
    $$PWD/sources/csvwriter.cpp \
    $$PWD/sources/variantdata.cpp \
    $$PWD/sources/stringdata.cpp \
    $$PWD/sources/csvreader.cpp \
    $$PWD/sources/contentiterator.cpp

HEADERS += \
    $$PWD/include/qtcsv/qtcsv_global.h \
    $$PWD/include/qtcsv/csvwriter.h \
    $$PWD/include/qtcsv/variantdata.h \
    $$PWD/include/qtcsv/stringdata.h \
    $$PWD/include/qtcsv/csvreader.h \
    $$PWD/include/qtcsv/abstractdata.h \
    $$PWD/sources/filechecker.h \
    $$PWD/sources/contentiterator.h \
    $$PWD/sources/symbols.h
