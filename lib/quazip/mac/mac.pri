mac{
    CONFIG+=build_all
    CONFIG+=debug_and_release
    include(osx.pri)
    include(ios.pri)
}

DISTFILES += \
    $$PWD/ios.pri
