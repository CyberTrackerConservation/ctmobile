IOS_PRI_FILE = $$sprintf("%1/iOS_%1.pri",$$PRODUCT_SELECT)
include ($$IOS_PRI_FILE)

OBJECTIVE_SOURCES += \
    $$PWD/../cpp/UtilsShareiOS.mm

HEADERS += \
    $$PWD/../cpp/UtilsShareiOS.h
