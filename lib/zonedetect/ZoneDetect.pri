INCLUDEPATH += $$PWD/library

DEFINES += ZD_EXPORT=

SOURCES += \
    $$PWD/library/zonedetect.c \

HEADERS += \
    $$PWD/library/zonedetect.h \

QMAKE_RESOURCE_FLAGS += -no-compress

# Update the database here: https://github.com/BertoldVdb/ZoneDetect/tree/master/database.
# Rebuild it using the builder in WSL. Will need to apt-get libshp-dev.
RESOURCES += \
    $$files($$PWD/timezone21.bin) \
