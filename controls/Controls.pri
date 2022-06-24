QT += core gui opengl network positioning sensors qml quick svg sql quickcontrols2 xml multimedia
qtHaveModule(webengine): QT += webengine
qtHaveModule(webview): QT += webview

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/cpp

QML_IMPORT_PATH += $$PWD/imports

HEADERS += \
    $$PWD/ControlsPlugin.h \
    $$PWD/Skyplot.h \
    $$PWD/Sketchpad.h \


SOURCES += \
    $$PWD/ControlsPlugin.cpp \
    $$PWD/Skyplot.cpp \
    $$PWD/Sketchpad.cpp \

RESOURCES += \
    $$PWD/Controls.qrc

lupdate_only {
    SOURCES += \
        $$PWD/imports/CyberTracker.1/*.qml \
        $$PWD/imports/CyberTracker.1/+webengine\*.qml \
        $$PWD/imports/CyberTracker.1/+webview\*.qml \
}
