INCLUDEPATH += $$PWD/library

DEFINES += ZD_EXPORT=

SOURCES += \
    $$PWD/library/zonedetect.c \

HEADERS += \
    $$PWD/library/zonedetect.h \

QMAKE_RESOURCE_FLAGS += -no-compress

# Update the database here: https://github.com/BertoldVdb/ZoneDetect/tree/master/database.
RESOURCES += \
    $$files($$PWD/timezone21.bin) \
