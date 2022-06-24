DEFINES += QTBUILD

*msvc* {
    DEFINES += VSBUILD
} else {
    QMAKE_CXXFLAGS += -Wno-unknown-pragmas
    QMAKE_CXXFLAGS += -Wno-multichar
    QMAKE_CXXFLAGS += -Wno-unused-parameter
    QMAKE_CXXFLAGS += -Wno-write-strings
    QMAKE_CXXFLAGS += -Wno-sign-compare
    QMAKE_CXXFLAGS += -Wno-narrowing
    QMAKE_CXXFLAGS += -Wno-unused-variable
    QMAKE_CXXFLAGS += -Wno-missing-field-initializers
    QMAKE_CXXFLAGS += -Wno-type-limits
    QMAKE_CXXFLAGS += -Wno-switch
    QMAKE_CXXFLAGS += -Wno-parentheses
    QMAKE_CXXFLAGS += -Wno-switch
    QMAKE_CXXFLAGS += -Wno-reorder
    QMAKE_CXXFLAGS += -Wno-switch
    QMAKE_CXXFLAGS += -Wno-unused-const-variable
    QMAKE_CXXFLAGS += -Wno-delete-non-virtual-dtor
    QMAKE_CXXFLAGS += -Wno-sequence-point
    QMAKE_CXXFLAGS += -Wno-address
    QMAKE_CXXFLAGS += -Wno-unused-function
    QMAKE_CXXFLAGS += -Wno-unused-value
    QMAKE_CXXFLAGS += -Wno-uninitialized
    QMAKE_CXXFLAGS += -Wno-char-subscripts
    QMAKE_CXXFLAGS += -Wno-extra
    QMAKE_CXXFLAGS += -Wno-strict-aliasing
}

# Disable strict string checks
QMAKE_CFLAGS_RELEASE -= -Zc:strictStrings
QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings
QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO -= -Zc:strictStrings
QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO -= -Zc:strictStrings
QMAKE_CFLAGS -= -Zc:strictStrings
QMAKE_CXXFLAGS -= -Zc:strictStrings

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/Framework
INCLUDEPATH += $$PWD/Controls
INCLUDEPATH += $$PWD/Main
INCLUDEPATH += $$PWD/Host/Qt
INCLUDEPATH += $$PWD/Host/Common

*msvc* {
PRECOMPILED_HEADER = $$PWD/pch.h
CONFIG += precompile_header
}

HEADERS += \
    $$PWD/pch.h \
    $$PWD/Framework/pch.h \
    $$PWD/Framework/fxApplication.h \
    $$PWD/Framework/fxBrand.h \
    $$PWD/Framework/fxCanvas.h \
    $$PWD/Framework/fxClasses.h \
    $$PWD/Framework/fxControl.h \
    $$PWD/Framework/fxDatabase.h \
    $$PWD/Framework/fxDialog.h \
    $$PWD/Framework/fxEngine.h \
    $$PWD/Framework/fxGPSParse.h \
    $$PWD/Framework/fxHost.h \
    $$PWD/Framework/fxLaserAtlanta.h \
    $$PWD/Framework/fxLists.h \
    $$PWD/Framework/fxMath.h \
    $$PWD/Framework/fxNMEA.h \
    $$PWD/Framework/fxParser.h \
    $$PWD/Framework/fxPlatform.h \
    $$PWD/Framework/fxProjection.h \
    $$PWD/Framework/fxRangeFinderParse.h \
    $$PWD/Framework/fxTSIP.h \
    $$PWD/Framework/fxTypes.h \
    $$PWD/Framework/fxUtils.h \
    $$PWD/Controls/pch.h \
    $$PWD/Controls/fxAction.h \
    $$PWD/Controls/fxButton.h \
    $$PWD/Controls/fxGPS.h \
    $$PWD/Controls/fxImage.h \
    $$PWD/Controls/fxKeypad.h \
    $$PWD/Controls/fxList.h \
    $$PWD/Controls/fxMarquee.h \
    $$PWD/Controls/fxMemo.h \
    $$PWD/Controls/fxNativeTextEdit.h \
    $$PWD/Controls/fxNotebook.h \
    $$PWD/Controls/fxPanel.h \
    $$PWD/Controls/fxRangeFinder.h \
    $$PWD/Controls/fxRegister.h \
    $$PWD/Controls/fxScrollBar.h \
    $$PWD/Controls/fxScrollBox.h \
    $$PWD/Controls/fxSound.h \
    $$PWD/Controls/fxSystem.h \
    $$PWD/Controls/fxTitleBar.h \
    $$PWD/Host/Qt/pch.h \
    $$PWD/Host/Qt/ArcGisClient.h \
    $$PWD/Host/Qt/QtUtils.h \
    $$PWD/Host/Qt/QtCanvas.h \
    $$PWD/Host/Qt/QtHost.h \
    $$PWD/Host/Qt/QtSessionItem.h \
    $$PWD/Host/Qt/QtTransfer.h \
    $$PWD/Host/Common/pch.h \
    $$PWD/Host/Common/Cab.h \
    $$PWD/Host/Common/Cab_deflate.h \
    $$PWD/Host/Common/Cab_zconf.h \
    $$PWD/Host/Common/Cab_zlib.h \
    $$PWD/Host/Common/Cab_zutil.h \
    $$PWD/Host/Common/ChunkDatabase.h \
    $$PWD/Host/Common/ecw.h \
    $$PWD/Host/Common/MapResource.h \
    $$PWD/Host/Common/NCSDefs.h \
    $$PWD/Host/Common/NCSEcw.h \
    $$PWD/Main/pch.h \
    $$PWD/Main/ctActionControls.h \
    $$PWD/Main/ctActions.h \
    $$PWD/Main/ctComPortList.h \
    $$PWD/Main/ctDialog_ComPortSelect.h \
    $$PWD/Main/ctDialog_Confirm.h \
    $$PWD/Main/ctDialog_GPSReader.h \
    $$PWD/Main/ctDialog_GPSViewer.h \
    $$PWD/Main/ctDialog_NumberEditor.h \
    $$PWD/Main/ctDialog_Password.h \
    $$PWD/Main/ctDialog_SightingEditor.h \
    $$PWD/Main/ctDialog_TextEditor.h \
    $$PWD/Main/ctElement.h \
    $$PWD/Main/ctElementBarcode.h \
    $$PWD/Main/ctElementCalc.h \
    $$PWD/Main/ctElementCamera.h \
    $$PWD/Main/ctElementContainer.h \
    $$PWD/Main/ctElementDate.h \
    $$PWD/Main/ctElementFilter.h \
    $$PWD/Main/ctElementImage.h \
    $$PWD/Main/ctElementImageGrid.h \
    $$PWD/Main/ctElementKeypad.h \
    $$PWD/Main/ctElementList.h \
    $$PWD/Main/ctElementLookupList.h \
    $$PWD/Main/ctElementMemo.h \
    $$PWD/Main/ctElementPhoneNumber.h \
    $$PWD/Main/ctElementR2Incendiary.h \
    $$PWD/Main/ctElementRangeFinder.h \
    $$PWD/Main/ctElementRecord.h \
    $$PWD/Main/ctElementSerial.h \
    $$PWD/Main/ctElementSound.h \
    $$PWD/Main/ctElementTextEdit.h \
    $$PWD/Main/ctElementUrlList.h \
    $$PWD/Main/ctGotos.h \
    $$PWD/Main/ctGPSTimerList.h \
    $$PWD/Main/ctHistory.h \
    $$PWD/Main/ctHistoryItem.h \
    $$PWD/Main/ctMain.h \
    $$PWD/Main/ctMovingMap.h \
    $$PWD/Main/ctNavigate.h \
    $$PWD/Main/ctNumberList.h \
    $$PWD/Main/ctOwnerInfo.h \
    $$PWD/Main/ctRegister.h \
    $$PWD/Main/ctSession.h \
    $$PWD/Main/ctSighting.h \
    $$PWD/Main/ctWaypoint.h \

SOURCES += \
    $$PWD/Framework/fxApplication.cpp \
    $$PWD/Framework/fxCanvas.cpp \
    $$PWD/Framework/fxClasses.cpp \
    $$PWD/Framework/fxControl.cpp \
    $$PWD/Framework/fxDatabase.cpp \
    $$PWD/Framework/fxDialog.cpp \
    $$PWD/Framework/fxEngine.cpp \
    $$PWD/Framework/fxHost.cpp \
    $$PWD/Framework/fxLaserAtlanta.cpp \
    $$PWD/Framework/fxLists.cpp \
    $$PWD/Framework/fxMath.cpp \
    $$PWD/Framework/fxNMEA.cpp \
    $$PWD/Framework/fxParser.cpp \
    $$PWD/Framework/fxPlatform.cpp \
    $$PWD/Framework/fxProjection.cpp \
    $$PWD/Framework/fxTSIP.cpp \
    $$PWD/Framework/fxTypes.cpp \
    $$PWD/Framework/fxUtils.cpp \
    $$PWD/Controls/fxAction.cpp \
    $$PWD/Controls/fxButton.cpp \
    $$PWD/Controls/fxGPS.cpp \
    $$PWD/Controls/fxImage.cpp \
    $$PWD/Controls/fxKeypad.cpp \
    $$PWD/Controls/fxList.cpp \
    $$PWD/Controls/fxMarquee.cpp \
    $$PWD/Controls/fxMemo.cpp \
    $$PWD/Controls/fxNativeTextEdit.cpp \
    $$PWD/Controls/fxNotebook.cpp \
    $$PWD/Controls/fxPanel.cpp \
    $$PWD/Controls/fxRangeFinder.cpp \
    $$PWD/Controls/fxRegister.cpp \
    $$PWD/Controls/fxScrollBar.cpp \
    $$PWD/Controls/fxScrollBox.cpp \
    $$PWD/Controls/fxSound.cpp \
    $$PWD/Controls/fxSystem.cpp \
    $$PWD/Controls/fxTitleBar.cpp \
    $$PWD/Host/Qt/ArcGisClient.cpp \
    $$PWD/Host/Qt/QtUtils.cpp \
    $$PWD/Host/Qt/QtCanvas.cpp \
    $$PWD/Host/Qt/QtHost.cpp \
    $$PWD/Host/Qt/QtSessionItem.cpp \
    $$PWD/Host/Qt/QtTransfer.cpp \
    $$PWD/Host/Common/Cab.cpp \
    $$PWD/Host/Common/ChunkDatabase.cpp \
    $$PWD/Host/Common/MapResource.cpp \
    $$PWD/Host/Common/NCSEcw.cpp \
    $$PWD/Host/Common/NCSUtil.cpp \
    $$PWD/Main/ctActionControls.cpp \
    $$PWD/Main/ctActions.cpp \
    $$PWD/Main/ctComPortList.cpp \
    $$PWD/Main/ctDialog_ComPortSelect.cpp \
    $$PWD/Main/ctDialog_Confirm.cpp \
    $$PWD/Main/ctDialog_GPSReader.cpp \
    $$PWD/Main/ctDialog_GPSViewer.cpp \
    $$PWD/Main/ctDialog_NumberEditor.cpp \
    $$PWD/Main/ctDialog_Password.cpp \
    $$PWD/Main/ctDialog_SightingEditor.cpp \
    $$PWD/Main/ctDialog_TextEditor.cpp \
    $$PWD/Main/ctElement.cpp \
    $$PWD/Main/ctElementBarcode.cpp \
    $$PWD/Main/ctElementCalc.cpp \
    $$PWD/Main/ctElementCamera.cpp \
    $$PWD/Main/ctElementContainer.cpp \
    $$PWD/Main/ctElementDate.cpp \
    $$PWD/Main/ctElementFilter.cpp \
    $$PWD/Main/ctElementImage.cpp \
    $$PWD/Main/ctElementImageGrid.cpp \
    $$PWD/Main/ctElementKeypad.cpp \
    $$PWD/Main/ctElementList.cpp \
    $$PWD/Main/ctElementLookupList.cpp \
    $$PWD/Main/ctElementMemo.cpp \
    $$PWD/Main/ctElementPhoneNumber.cpp \
    $$PWD/Main/ctElementR2Incendiary.cpp \
    $$PWD/Main/ctElementRangeFinder.cpp \
    $$PWD/Main/ctElementRecord.cpp \
    $$PWD/Main/ctElementSerial.cpp \
    $$PWD/Main/ctElementSound.cpp \
    $$PWD/Main/ctElementTextEdit.cpp \
    $$PWD/Main/ctElementUrlList.cpp \
    $$PWD/Main/ctGotos.cpp \
    $$PWD/Main/ctGPSTimerList.cpp \
    $$PWD/Main/ctHistory.cpp \
    $$PWD/Main/ctHistoryItem.cpp \
    $$PWD/Main/ctMain.cpp \
    $$PWD/Main/ctMovingMap.cpp \
    $$PWD/Main/ctNavigate.cpp \
    $$PWD/Main/ctNumberList.cpp \
    $$PWD/Main/ctOwnerInfo.cpp \
    $$PWD/Main/ctRegister.cpp \
    $$PWD/Main/ctSession.cpp \
    $$PWD/Main/ctSighting.cpp
