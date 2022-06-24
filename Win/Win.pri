RC_FILE = $$sprintf("%1/Resources_%2.rc",$$PWD,$$PRODUCT_SELECT)

OTHER_FILES += \
    $$PWD/Resources_ct.rc \
    $$PWD/Resources_sm.rc \
    $$PWD/AppIcon_ct.ico \
    $$PWD/AppIcon_sm.ico \

LIBS += User32.lib Ole32.lib
