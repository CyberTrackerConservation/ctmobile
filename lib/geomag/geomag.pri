INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/GeomagnetismLibrary.c \
    $$PWD/geomag.cpp \

HEADERS += \
    $$PWD/GeomagnetismHeader.h \
    $$PWD/geomag.h \

RESOURCES += \
    $$files($$PWD/WMM.COF) \
