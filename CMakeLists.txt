cmake_minimum_required(VERSION 3.5)

# On Windows, do the following:
#   1. vcpkg install zlib:x64-windows
#   2. For the build in question, set the CMake variable CMAKE_TOOLCHAIN_FILE=D:/github/vcpkg/scripts/buildsystems/vcpkg.cmake
#   3. Set the CMake variable VCPKG_TARGET_TRIPLET=x64-windows

## MacOS: Must come before project()
if(APPLE AND NOT IOS)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS version")
endif()

project(ct VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT PRODUCT_SELECT)
    set(PRODUCT_SELECT "CT")
endif()

if (NOT PRODUCT_VERSION)
    set(PRODUCT_VERSION "603")
endif()

set(BUILD_EXAMPLES OFF)

if(IOS)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

################################################################################

message(STATUS "[CMAKE] CMAKE_VERSION               = ${CMAKE_VERSION}")
message(STATUS "[CMAKE] CMAKE_PREFIX_PATH           = ${CMAKE_PREFIX_PATH}")
message(STATUS "[SYSTEM] (HOST) SYSTEM NAME         = ${CMAKE_HOST_SYSTEM_NAME}")
message(STATUS "[SYSTEM] (HOST) SYSTEM VERSION      = ${CMAKE_HOST_SYSTEM_VERSION}")
message(STATUS "[SYSTEM] (TARGET) SYSTEM NAME       = ${CMAKE_SYSTEM_NAME}")
message(STATUS "[SYSTEM] (TARGET) SYSTEM VERSION    = ${CMAKE_SYSTEM_VERSION}")
message(STATUS "[PROJECT] SOURCE_DIR                = ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "[PROJECT] BUILD_TYPE                = ${CMAKE_BUILD_TYPE}")
message(STATUS "[PROJECT] TOOLCHAIN_FILE            = ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "[PROJECT] QT_HOST_PATH              = ${QT_HOST_PATH}")
message(STATUS "[PROJECT] PRODUCT_SELECT            = ${PRODUCT_SELECT}")
message(STATUS "[PROJECT] PRODUCT_VERSION           = ${PRODUCT_VERSION}")

################################################################################

find_package(Qt6 REQUIRED COMPONENTS
    Core
    CorePrivate
    Gui
    Qml
    Quick
    QuickControls2Material
    Svg
    Sql
    Xml
    Concurrent
    Multimedia
    Positioning
    Sensors
    WebSockets
    NetworkAuth
    WebView
    Core5Compat
)

qt_standard_project_setup(REQUIRES 6.8)

set(SOURCES_MAIN
    src/pch.h
    src/Main.cpp
    src/App.cpp src/App.h
    src/AppLink.cpp src/AppLink.h
    src/Audit.cpp src/Audit.h
    src/BackupFile.cpp src/BackupFile.h
    src/Compass.cpp src/Compass.h
    src/Connector.cpp src/Connector.h
    src/Database.cpp src/Database.h
    src/Element.cpp src/Element.h
    src/ElementListModel.cpp src/ElementListModel.h
    src/ElementTreeModel.cpp src/ElementTreeModel.h
    src/Evaluator.cpp src/Evaluator.h
    src/EvaluatorFunctions.h
    src/ExportFile.cpp src/ExportFile.h
    src/Fields.cpp src/Fields.h
    src/FieldBinding.cpp src/FieldBinding.h
    src/FieldListModel.cpp src/FieldListModel.h
    src/Form.cpp src/Form.h
    src/Goto.cpp src/Goto.h
    src/HasUpdate.cpp src/HasUpdate.h
    src/Location.cpp src/Location.h
    src/Log.cpp src/Log.h
    src/MapController.cpp src/MapController.h
    src/MapLayerListModel.cpp src/MapLayerListModel.h
    src/NativeImagePicker.h src/NativeImagePicker.cpp
    src/OfflineMap.cpp src/OfflineMap.h
    src/Place.cpp src/Place.h
    src/Project.cpp src/Project.h
    src/ProjectListModel.cpp src/ProjectListModel.h
    src/PropertyHelpers.h
    src/ProviderInterface.cpp src/ProviderInterface.h
    src/Record.cpp src/Record.h
    src/RecordListModel.cpp src/RecordListModel.h
    src/Register.cpp src/Register.h
    src/Renderer.cpp src/Renderer.h
    src/Satellite.cpp src/Satellite.h
    src/Schema.cpp src/Schema.h
    src/Settings.cpp src/Settings.h
    src/Sighting.cpp src/Sighting.h
    src/SightingListModel.cpp src/SightingListModel.h
    src/SightingModel.cpp src/SightingModel.h
    src/Telemetry.cpp src/Telemetry.h
    src/TimeManager.cpp src/TimeManager.h
    src/TrackFile.cpp src/TrackFile.h
    src/TranscribeQueue.cpp src/TranscribeQueue.h
    src/UploadQueue.cpp src/UploadQueue.h
    src/UploadQueueModel.cpp src/UploadQueueModel.h
    src/Utils.cpp src/Utils.h
    src/UtilsAudio.cpp src/UtilsAudio.h
    src/UtilsMap.cpp src/UtilsMap.h
    src/UtilsZip.cpp src/UtilsEsri.cpp src/UtilsGoogle.cpp
    src/UtilsPlatform.cpp src/UtilsPlatform.h
    src/VariantListModel.cpp src/VariantListModel.h
    src/Vault.cpp src/Vault.h
    src/VoiceRecorder.cpp src/VoiceRecorder.h
    src/WaveFileRecorder.cpp src/WaveFileRecorder.h
    src/Wizard.cpp src/Wizard.h
    src/XlsForm.cpp src/XlsForm.h
)

set(SOURCES_CONTROLS
    src/BarcodeReader.cpp src/BarcodeReader.h
    src/Camera.cpp src/Camera.h
    src/ClassicItem.cpp src/ClassicItem.h
    src/Skyplot.h src/Skyplot.cpp
    src/SketchPad.h src/SketchPad.cpp
    src/SquareImage.h src/SquareImage.cpp
)

set(SOURCES_CLASSIC
    src/Classic/ClassicConnector.cpp src/Classic/ClassicConnector.h
    src/Classic/ClassicProvider.cpp src/Classic/ClassicProvider.h
    src/Classic/ClassicRenderer.cpp src/Classic/ClassicRenderer.h
    src/Classic/ClassicSchema.cpp src/Classic/ClassicSchema.h
    src/Classic/ClassicView.cpp src/Classic/ClassicView.h
)

set(SOURCES_CLASSIC_LIB
    src/Classic/lib/Controls/fxAction.cpp src/Classic/lib/Controls/fxAction.h
    src/Classic/lib/Controls/fxButton.cpp src/Classic/lib/Controls/fxButton.h
    src/Classic/lib/Controls/fxGPS.cpp src/Classic/lib/Controls/fxGPS.h
    src/Classic/lib/Controls/fxImage.cpp src/Classic/lib/Controls/fxImage.h
    src/Classic/lib/Controls/fxKeypad.cpp src/Classic/lib/Controls/fxKeypad.h
    src/Classic/lib/Controls/fxList.cpp src/Classic/lib/Controls/fxList.h
    src/Classic/lib/Controls/fxMarquee.cpp src/Classic/lib/Controls/fxMarquee.h
    src/Classic/lib/Controls/fxMemo.cpp src/Classic/lib/Controls/fxMemo.h
    src/Classic/lib/Controls/fxNotebook.cpp src/Classic/lib/Controls/fxNotebook.h
    src/Classic/lib/Controls/fxPanel.cpp src/Classic/lib/Controls/fxPanel.h
    src/Classic/lib/Controls/fxRangeFinder.cpp src/Classic/lib/Controls/fxRangeFinder.h
    src/Classic/lib/Controls/fxRegister.cpp src/Classic/lib/Controls/fxRegister.h
    src/Classic/lib/Controls/fxScrollBar.cpp src/Classic/lib/Controls/fxScrollBar.h
    src/Classic/lib/Controls/fxScrollBox.cpp src/Classic/lib/Controls/fxScrollBox.h
    src/Classic/lib/Controls/fxSound.cpp src/Classic/lib/Controls/fxSound.h
    src/Classic/lib/Controls/fxSystem.cpp src/Classic/lib/Controls/fxSystem.h
    src/Classic/lib/Controls/fxTitleBar.cpp src/Classic/lib/Controls/fxTitleBar.h
    src/Classic/lib/Framework/fxTranslate.cpp src/Classic/lib/Framework/fxTranslate.h
    src/Classic/lib/Framework/fxApplication.cpp src/Classic/lib/Framework/fxApplication.h
    src/Classic/lib/Framework/fxBrand.h src/Classic/lib/Framework/fxCanvas.cpp
    src/Classic/lib/Framework/fxCanvas.h src/Classic/lib/Framework/fxClasses.cpp
    src/Classic/lib/Framework/fxClasses.h src/Classic/lib/Framework/fxControl.cpp
    src/Classic/lib/Framework/fxControl.h src/Classic/lib/Framework/fxDatabase.cpp
    src/Classic/lib/Framework/fxDatabase.h src/Classic/lib/Framework/fxDialog.cpp
    src/Classic/lib/Framework/fxDialog.h src/Classic/lib/Framework/fxEngine.cpp
    src/Classic/lib/Framework/fxEngine.h
    src/Classic/lib/Framework/fxHost.cpp src/Classic/lib/Framework/fxHost.h
    src/Classic/lib/Framework/fxLists.cpp src/Classic/lib/Framework/fxLists.h
    src/Classic/lib/Framework/fxMath.cpp src/Classic/lib/Framework/fxMath.h
    src/Classic/lib/Framework/fxParser.cpp src/Classic/lib/Framework/fxParser.h
    src/Classic/lib/Framework/fxPlatform.cpp src/Classic/lib/Framework/fxPlatform.h
    src/Classic/lib/Framework/fxProjection.cpp src/Classic/lib/Framework/fxProjection.h
    src/Classic/lib/Framework/fxTypes.cpp src/Classic/lib/Framework/fxTypes.h
    src/Classic/lib/Framework/fxUtils.cpp src/Classic/lib/Framework/fxUtils.h
    src/Classic/lib/Host/Common/Cab.cpp src/Classic/lib/Host/Common/Cab.h
    src/Classic/lib/Main/ctActionControls.cpp src/Classic/lib/Main/ctActionControls.h
    src/Classic/lib/Main/ctActions.cpp src/Classic/lib/Main/ctActions.h
    src/Classic/lib/Main/ctComPortList.cpp src/Classic/lib/Main/ctComPortList.h
    src/Classic/lib/Main/ctDialog_NumberEditor.cpp src/Classic/lib/Main/ctDialog_NumberEditor.h
    src/Classic/lib/Main/ctElement.cpp src/Classic/lib/Main/ctElement.h
    src/Classic/lib/Main/ctElementBarcode.cpp src/Classic/lib/Main/ctElementBarcode.h
    src/Classic/lib/Main/ctElementCalc.cpp src/Classic/lib/Main/ctElementCalc.h
    src/Classic/lib/Main/ctElementCamera.cpp src/Classic/lib/Main/ctElementCamera.h
    src/Classic/lib/Main/ctElementContainer.cpp src/Classic/lib/Main/ctElementContainer.h
    src/Classic/lib/Main/ctElementDate.cpp src/Classic/lib/Main/ctElementDate.h
    src/Classic/lib/Main/ctElementFilter.cpp src/Classic/lib/Main/ctElementFilter.h
    src/Classic/lib/Main/ctElementImage.cpp src/Classic/lib/Main/ctElementImage.h
    src/Classic/lib/Main/ctElementImageGrid.cpp src/Classic/lib/Main/ctElementImageGrid.h
    src/Classic/lib/Main/ctElementKeypad.cpp src/Classic/lib/Main/ctElementKeypad.h
    src/Classic/lib/Main/ctElementList.cpp src/Classic/lib/Main/ctElementList.h
    src/Classic/lib/Main/ctElementLookupList.cpp src/Classic/lib/Main/ctElementLookupList.h
    src/Classic/lib/Main/ctElementMemo.cpp src/Classic/lib/Main/ctElementMemo.h
    src/Classic/lib/Main/ctElementPhoneNumber.cpp src/Classic/lib/Main/ctElementPhoneNumber.h
    src/Classic/lib/Main/ctElementR2Incendiary.cpp src/Classic/lib/Main/ctElementR2Incendiary.h
    src/Classic/lib/Main/ctElementRangeFinder.cpp src/Classic/lib/Main/ctElementRangeFinder.h
    src/Classic/lib/Main/ctElementRecord.cpp src/Classic/lib/Main/ctElementRecord.h
    src/Classic/lib/Main/ctElementSerial.cpp src/Classic/lib/Main/ctElementSerial.h
    src/Classic/lib/Main/ctElementSound.cpp src/Classic/lib/Main/ctElementSound.h
    src/Classic/lib/Main/ctElementTextEdit.cpp src/Classic/lib/Main/ctElementTextEdit.h
    src/Classic/lib/Main/ctElementUrlList.cpp src/Classic/lib/Main/ctElementUrlList.h
    src/Classic/lib/Main/ctGotos.cpp src/Classic/lib/Main/ctGotos.h
    src/Classic/lib/Main/ctGPSTimerList.cpp src/Classic/lib/Main/ctGPSTimerList.h
    src/Classic/lib/Main/ctHistory.cpp src/Classic/lib/Main/ctHistory.h
    src/Classic/lib/Main/ctHistoryItem.cpp src/Classic/lib/Main/ctHistoryItem.h
    src/Classic/lib/Main/ctMovingMap.cpp src/Classic/lib/Main/ctMovingMap.h
    src/Classic/lib/Main/ctNavigate.cpp src/Classic/lib/Main/ctNavigate.h
    src/Classic/lib/Main/ctNumberList.cpp src/Classic/lib/Main/ctNumberList.h
    src/Classic/lib/Main/ctOwnerInfo.cpp src/Classic/lib/Main/ctOwnerInfo.h
    src/Classic/lib/Main/ctRegister.cpp src/Classic/lib/Main/ctRegister.h
    src/Classic/lib/Main/ctSession.cpp src/Classic/lib/Main/ctSession.h
    src/Classic/lib/Main/ctSighting.cpp src/Classic/lib/Main/ctSighting.h
    src/Classic/lib/Main/ctSightingEditor.cpp src/Classic/lib/Main/ctSightingEditor.h
    src/Classic/lib/Main/ctWaypoint.h
)

set(SOURCES_CTO
    src/CTO/CTOConnector.cpp src/CTO/CTOConnector.h
)

set(SOURCES_EARTHRANGER
    src/EarthRanger/EarthRangerConnector.cpp src/EarthRanger/EarthRangerConnector.h
    src/EarthRanger/EarthRangerProvider.cpp src/EarthRanger/EarthRangerProvider.h
    src/EarthRanger/EarthRangerRenderer.cpp src/EarthRanger/EarthRangerRenderer.h
    src/EarthRanger/EarthRangerSchema.cpp src/EarthRanger/EarthRangerSchema.h
)

set(SOURCES_ESRI
    src/Esri/EsriConnector.cpp src/Esri/EsriConnector.h
    src/Esri/EsriProvider.cpp src/Esri/EsriProvider.h
)

set(SOURCES_GOOGLE
    src/Google/GoogleConnector.cpp src/Google/GoogleConnector.h
    src/Google/GoogleFormParser.cpp src/Google/GoogleFormParser.h
    src/Google/GoogleProvider.cpp src/Google/GoogleProvider.h
)

set(SOURCES_KOBO
    src/KoBo/KoBoConnector.cpp src/KoBo/KoBoConnector.h
)

set(SOURCES_NATIVE
    src/Native/NativeConnector.cpp src/Native/NativeConnector.h
    src/Native/NativeProvider.cpp src/Native/NativeProvider.h
)

set(SOURCES_ODK
    src/ODK/ODKConnector.cpp src/ODK/ODKConnector.h
    src/ODK/ODKProvider.cpp src/ODK/ODKProvider.h
)

set(SOURCES_SMART
    src/SMART/SMARTConnector.cpp src/SMART/SMARTConnector.h
    src/SMART/SMARTProvider.cpp src/SMART/SMARTProvider.h
    src/SMART/SMARTRenderer.cpp src/SMART/SMARTRenderer.h
    src/SMART/SMARTSchema.cpp src/SMART/SMARTSchema.h
)

if(IOS)
    set(PLATFORM_LIBRARIES "-framework UIKit" m)
    set(SOURCES_PLATFORM
        src/UtilsPlatformiOS.mm
        src/UtilsPlatformiOS.h
        src/NativeImagePicker.mm
    )
endif()

set(SOURCES_ALL
    ${SOURCES_PLATFORM}
    ${SOURCES_MAIN}
    ${SOURCES_CONTROLS}
    ${SOURCES_CLASSIC}
    ${SOURCES_CLASSIC_LIB}
    ${SOURCES_CTO}
    ${SOURCES_EARTHRANGER}
    ${SOURCES_ESRI}
    ${SOURCES_GOOGLE}
    ${SOURCES_KOBO}
    ${SOURCES_NATIVE}
    ${SOURCES_ODK}
    ${SOURCES_SMART}
)

if(WIN32)
    set(WIN_ICON platforms/Win/${PRODUCT_SELECT}/Resources.rc)
endif()

qt_add_executable(${CMAKE_PROJECT_NAME}
    ${SOURCES_ALL}
    ${WIN_ICON}
)

# controls
qt_add_resources(${CMAKE_PROJECT_NAME} "controls" PREFIX "imports/CyberTracker/" BASE "controls" FILES
    controls/qmldir
    controls/BarcodeScanner.qml
    controls/BusyCover.qml
    controls/CheckableBox3.qml
    controls/CheckableBox.qml
    controls/CheckBoxIconInlay.qml
    controls/CheckBoxIconOnly.qml
    controls/CheckBoxTextBesideIcon.qml
    controls/CheckBoxTextOnly.qml
    controls/ChevronRight.qml
    controls/CircleButton.qml
    controls/ColorButton.qml
    controls/ConfirmButton.qml
    controls/ConfirmPopup.qml
    controls/ContentPage.qml
    controls/CountryListModel.qml
    controls/DatePicker.qml
    controls/DateTimePicker.qml
    controls/DateTimePickerPage.qml
    controls/DetailsPage.qml
    controls/DetailsPane.qml
    controls/EsriMapView.qml
    controls/EsriMapViewPage.qml
    controls/FieldAudio.qml
    controls/FieldBarcode.qml
    controls/FieldBarcodePage.qml
    controls/FieldBarcodePane.qml
    controls/FieldCalendar.qml
    controls/FieldCalendarPage.qml
    controls/FieldCamera.qml
    controls/FieldCameraPage.qml
    controls/FieldCameraPane.qml
    controls/FieldCheckBox.qml
    controls/FieldCheckBox3.qml
    controls/FieldCheckGridView.qml
    controls/FieldCheckGridViewPage.qml
    controls/FieldCheckListView.qml
    controls/FieldCheckListViewPage.qml
    controls/FieldComboBox.qml
    controls/FieldDatePicker.qml
    controls/FieldDateTimePicker.qml
    controls/FieldDateTimePickerPage.qml
    controls/FieldEditorDelegate.qml
    controls/FieldFilePage.qml
    controls/FieldFilePane.qml
    controls/FieldGridView.qml
    controls/FieldGridViewPage.qml
    controls/FieldKeypad.qml
    controls/FieldKeypadPage.qml
    controls/FieldLikert.qml
    controls/FieldLine.qml
    controls/FieldLinePage.qml
    controls/FieldLinePane.qml
    controls/FieldListView.qml
    controls/FieldListViewEditor.qml
    controls/FieldListViewPage.qml
    controls/FieldLocation.qml
    controls/FieldLocationPage.qml
    controls/FieldLocationPane.qml
    controls/FieldName.qml
    controls/FieldNumberListViewPage.qml
    controls/FieldPhotoButton.qml
    controls/FieldPhotoGridView.qml
    controls/FieldPhotoListView.qml
    controls/FieldRange.qml
    controls/FieldRangePage.qml
    controls/FieldRecordEditorPage.qml
    controls/FieldRecordListView.qml
    controls/FieldRecordListViewPage.qml
    controls/FieldRecordNumberGridView.qml
    controls/FieldRecordNumberGridViewPage.qml
    controls/FieldRecordPhotoGridView.qml
    controls/FieldRecordPhotoGridViewPage.qml
    controls/FieldSketchPad.qml
    controls/FieldSketchPadPage.qml
    controls/FieldStateMarker.qml
    controls/FieldTextEdit.qml
    controls/FieldTextEditPage.qml
    controls/FieldTimePicker.qml
    controls/FooterButton.qml
    controls/FooterSpacer.qml
    controls/FormLanguageComboBox.qml
    controls/FormSettingsListView.qml
    controls/FormSettingsPage.qml
    controls/FormView.qml
    controls/FormWizardPage.qml
    controls/GotoLocation.qml
    controls/GridFrameDelegate.qml
    controls/GridViewV.qml
    controls/HighlightDelegate.qml
    controls/HintPage.qml
    controls/HorizontalDivider.qml
    controls/ImagePage.qml
    controls/ImagePicker.qml
    controls/ListRow.qml
    controls/ListRowDelegate.qml
    controls/ListViewH.qml
    controls/ListViewV.qml
    controls/LocationData.qml
    controls/LocationPage.qml
    controls/LocationPopup.qml
    controls/LoginItem.qml
    controls/LoginPage.qml
    controls/LoginPopup.qml
    controls/MessagePopup.qml
    controls/OAuthItem.qml
    controls/OAuthPopup.qml
    controls/OptionsPopup.qml
    controls/PageHeader.qml
    controls/PageStack.qml
    controls/PasscodePopup.qml
    controls/PopupBase.qml
    controls/PopupError.qml
    controls/PopupLoader.qml
    controls/PopupToast.qml
    controls/ProgressCircle.qml
    controls/SightingListView.qml
    controls/SightingsListView.qml
    controls/SimpleColorOverlay.qml
    controls/SimpleDropShadow.qml
    controls/Slider2.qml
    controls/SquareIcon.qml
    controls/Style.qml
    controls/SubmitButton.qml
    controls/TimePicker.qml
    controls/TranscribeButton.qml
    controls/UsernamePage.qml
    controls/VoiceRecorderButton.qml
    controls/WebView.qml
    controls/WizardFooter.qml
    controls/WizardHeader.qml
    controls/WizardIndex.qml
    controls/WizardIndexPage.qml
    controls/WizardOptionsPage.qml
    controls/WizardPage.qml
)

if(NOT ANDROID AND NOT IOS)
    qt_add_resources(${CMAKE_PROJECT_NAME} "qmlDesktop" BASE "qml" PREFIX "/" FILES
        qml/AppMainDesktop.qml
        qml/ConsoleWindow.qml
        qml/CreateEsriServiceWindow.qml
    )
endif()

# qml files
qt_add_resources(${CMAKE_PROJECT_NAME} "qml" BASE "qml" PREFIX "/" FILES
    qml/AboutPage.qml
    qml/AuditPage.qml
    qml/AppMain.qml
    qml/ArchiveConfigPage.qml
    qml/ArchivesPage.qml
    qml/BackupsPage.qml
    qml/BootstrapAuthPage.qml
    qml/BreakingChangePage.qml
    qml/BugReportPage.qml
    qml/ComingSoonPage.qml
    qml/ConnectFormPage.qml
    qml/ConnectPage.qml
    qml/ConnectPane.qml
    qml/ConnectQRCodePage.qml
    qml/ExportDetailsPage.qml
    qml/ExportsPage.qml
    qml/InspectorPage.qml
    qml/MainPage.qml
    qml/MapInspectorPage.qml
    qml/MapLayersPage.qml
    qml/MapsPage.qml
    qml/OfflineMapPage.qml
    qml/PlacePage.qml
    qml/ProjectChangePage.qml
    qml/ProjectDetailsPage.qml
    qml/ProjectionPage.qml
    qml/ProjectsEditPage.qml
    qml/ProjectsPage.qml
    qml/SettingsPage.qml
    qml/ShareProjectPage.qml
    qml/ShareUrlPage.qml
    qml/SightingFilterPage.qml
    qml/SightingGroupByPage.qml
    qml/SightingNotePage.qml
    qml/UploadQueuePage.qml
    qml/WebhookPage.qml
    qml/Classic/ConnectClassicPage.qml
    qml/Classic/ConnectOnlinePage.qml
    qml/Classic/ConnectSamplesPage.qml
    qml/Classic/ConnectWebInstallPage.qml
    qml/Classic/SightingEditorPage.qml
    qml/Classic/StartPage.qml
    qml/Classic/TextEditorPage.qml
    qml/EarthRanger/AttributesPage.qml
    qml/EarthRanger/ConnectPage.qml
    qml/EarthRanger/EventTypePage.qml
    qml/EarthRanger/ReportedByPage.qml
    qml/EarthRanger/SettingsPage.qml
    qml/EarthRanger/StartPage.qml
    qml/EarthRanger/VisibleReportsPage.qml
    qml/Esri/ConnectPage.qml
    qml/Esri/LocationServicePage.qml
    qml/Google/ConnectPage.qml
    qml/Google/UnsupportedPage.qml
    qml/ODK/ConnectKoBoPage.qml
    qml/ODK/ConnectKoBoSharedPage.qml
    qml/ODK/ConnectODKCentralPage.qml
    qml/ODK/FormPage.qml
    qml/ODK/StartPage.qml
    qml/SMART/qmldir
    qml/SMART/AttributesPage.qml
    qml/SMART/ConnectCollectPage.qml
    qml/SMART/ConnectConnectPage.qml
    qml/SMART/ConnectPage.qml
    qml/SMART/ConnectPaneSM.qml
    qml/SMART/ConnectRoutesPage.qml
    qml/SMART/Globals.qml
    qml/SMART/HistoryPage.qml
    qml/SMART/LocationPage.qml
    qml/SMART/MetadataPage.qml
    qml/SMART/ModelPage.qml
    qml/SMART/PatrolStatsPage.qml
    qml/SMART/RecoverDataPage.qml
    qml/SMART/SettingsPage.qml
    qml/SMART/SightingHomePage.qml
    qml/SMART/SightingPage.qml
    qml/SMART/StartPage.qml
    qml/SMART/StartPageButton.qml
    qml/SMART/StartPageCollect.qml
)

# Assets.
add_subdirectory(assets)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Assets)

# Precompiled header
target_precompile_headers(${CMAKE_PROJECT_NAME} PRIVATE src/pch.h)

# Included directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_BINARY_DIR} # for generated headers
    3rdparty/ src/
    src/Classic/lib/Framework src/Classic/lib/Controls src/Classic/lib/Main src/Classic/lib/Host/Common src/Classic/lib/Host/Qt
)

# ArcGIS
find_package(ArcGISRuntime 200.8.1 COMPONENTS REQUIRED Cpp)

# ArcGIS toolkit
#add_subdirectory(3rdparty/arcgistoolkit/uitools/toolkitcpp)
#target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE libtoolkitcpp)

# cablib
add_subdirectory(3rdparty/cablib)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE cablib)

# ecwlib
add_subdirectory(3rdparty/ecwlib)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ecwlib)

# tinygps
add_subdirectory(3rdparty/tinygps)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE tinygps)

# zonedetect
add_subdirectory(3rdparty/zonedetect)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE zonedetect)

# geomag
add_subdirectory(3rdparty/geomag)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE geomag)

# qtcsv
add_subdirectory(3rdparty/qtcsv)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE qtcsv)

# qtkeychain
set(BUILD_WITH_QT6 ON)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_TRANSLATIONS OFF)
set(BUILD_TEST_APPLICATION OFF)
set(BUILD_TESTING OFF)
add_subdirectory(3rdparty/qtkeychain)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE qt6keychain)

# exiv2
set(BUILD_SHARED_LIBS OFF)
set(EXV_HAVE_BROTLI OFF CACHE BOOL "" FORCE)
set(EXIV2_ENABLE_BROTLI OFF CACHE BOOL "" FORCE)
set(EXIV2_ENABLE_VIDEO OFF CACHE BOOL "" FORCE)
set(EXIV2_ENABLE_XMP OFF CACHE BOOL "" FORCE)
set(EXIV2_ENABLE_INIH OFF CACHE BOOL "" FORCE)
set(EXIV2_BUILD_EXIV2_COMMAND OFF CACHE BOOL "" FORCE)
add_subdirectory(3rdparty/exiv2)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE exiv2lib)

# xlnt
set(STATIC ON)
add_subdirectory(3rdparty/xlnt)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE xlnt)

# zxing-cpp
set(ZXING_UNIT_TESTS OFF)
set(ZXING_BLACKBOX_TESTS OFF)
set(ZXING_EXAMPLES OFF)
add_subdirectory(3rdparty/zxing-cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ZXing)

# quazip
set(QUAZIP_BZIP2 OFF)
add_subdirectory(3rdparty/quazip)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QuaZip)

# libcurl
set(BUILD_SHARED_LIBS OFF)
set(BUILD_STATIC_LIBS ON)
set(BUILD_CURL_EXE OFF)
set(BUILD_LIBCURL_DOCS OFF)
set(BUILD_MISC_DOCS OFF)
set(ENABLE_CURL_MANUAL OFF)
set(CURL_USE_LIBPSL OFF)
set(CURL_USE_OPENSSL OFF)
set(CURL_USE_LIBSSH2 OFF)
set(USE_NGHTTP2 OFF)
set(CURL_DISABLE_VERBOSE_STRINGS ON)
set(CURL_DISABLE_HTTP ON)
set(CURL_DISABLE_COOKIES ON)
set(CURL_DISABLE_CRYPTO_AUTH ON)
set(CURL_DISABLE_DOH ON)
set(CURL_DISABLE_DICT ON)
set(CURL_DISABLE_FILE ON)
set(CURL_DISABLE_GOPHER ON)
set(CURL_DISABLE_HSTS ON)
set(CURL_DISABLE_IMAP ON)
set(CURL_DISABLE_LDAP ON)
set(CURL_DISABLE_MIME ON)
set(CURL_DISABLE_MQTT ON)
set(CURL_DISABLE_NETRC ON)
set(CURL_DISABLE_NTLM ON)
set(CURL_DISABLE_POP3 ON)
set(CURL_DISABLE_PROXY ON)
set(CURL_DISABLE_RTSP ON)
set(CURL_DISABLE_SMB ON)
set(CURL_DISABLE_SMTP ON)
set(CURL_DISABLE_TELNET ON)
set(CURL_DISABLE_TFTP ON)
set(CURL_BROTLI OFF CACHE STRING "" FORCE)
add_subdirectory(3rdparty/curl)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE CURL::libcurl)

add_subdirectory(3rdparty/libfvad)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE fvad)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 3rdparty/libfvad/include)

add_subdirectory(3rdparty/libsamplerate)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE samplerate)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 3rdparty/libsamplerate/include)


# ArcGIS stuff
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

# Qt
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::CorePrivate
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2Material
    Qt6::Svg
    Qt6::Sql
    Qt6::Xml
    Qt6::Multimedia
    Qt6::Positioning
    Qt6::Sensors
    Qt6::WebSockets
    Qt6::NetworkAuth
    Qt6::WebView
    Qt6::Concurrent
    Qt6::Core5Compat
    ArcGISRuntime::Cpp
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QTBUILD)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QT_STATICPLUGIN)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE CT_PRODUCT_SELECT="${PRODUCT_SELECT}")
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE CT_VERSION="1.0.${PRODUCT_VERSION}")

################################################################################
## Windows

if(WIN32)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE ON
    )

    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        _USE_MATH_DEFINES
        NOMINMAX
    )

    qt_add_resources(${CMAKE_PROJECT_NAME} "AppIcon" FILES platforms/Win/${PRODUCT_SELECT}/Resources.rc)
endif()

## MacOS
if(APPLE AND NOT IOS)
    qt_add_resources(${CMAKE_PROJECT_NAME} "AppIcon" FILES platforms/Mac/${PRODUCT_SELECT}/AppIcon.icns)
    set_source_files_properties(platforms/Mac/${PRODUCT_SELECT}/AppIcon.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/platforms/Mac/${PRODUCT_SELECT}/Info.plist"
    )
endif()

## iOS
if(IOS)
    set_source_files_properties("platforms/iOS/${PRODUCT_SELECT}/LaunchScreen.storyboard" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    qt_add_resources(${CMAKE_PROJECT_NAME} "XcAssets" FILES "platforms/iOS/${PRODUCT_SELECT}/Assets.xcassets")
    set_source_files_properties("platforms/iOS/${PRODUCT_SELECT}/Assets.xcassets" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    if(${PRODUCT_SELECT} STREQUAL "CT")
        set(PRODUCT_BUNDLE_IDENTIFIER "org.cybertracker.mobile")
        set(PRODUCT_NAME "CyberTracker")
    elseif(${PRODUCT_SELECT} STREQUAL "SM")
        set(PRODUCT_BUNDLE_IDENTIFIER "org.cybertracker.smartcollect")
        set(PRODUCT_NAME "SMART Mobile")
    elseif(${PRODUCT_SELECT} STREQUAL "GG")
        set(PRODUCT_BUNDLE_IDENTIFIER "org.cybertracker.rillaforms")
        set(PRODUCT_NAME "Rilla Forms")
    endif()

    set(PRODUCT_DISPLAY_NAME ${PRODUCT_NAME})
    set(PRODUCT_CATEGORY "public.app-category.productivity")
    set(PRODUCT_VERSION_STRING "10.${PRODUCT_VERSION}")
    set(PRODUCT_BUILD_NUMBER 1)
    set(PRODUCT_COPYRIGHT "Copyright © CyberTracker Conservation. All rights reserved.")

    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/platforms/iOS/${PRODUCT_SELECT}/Info.plist"
        QT_IOS_LAUNCH_SCREEN "${CMAKE_CURRENT_SOURCE_DIR}/platforms/iOS/${PRODUCT_SELECT}/LaunchScreen.storyboard"
        XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/platforms/iOS/${PRODUCT_SELECT}/AppDomains.entitlements"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${PRODUCT_BUNDLE_IDENTIFIER}
        XCODE_ATTRIBUTE_PRODUCT_NAME ${PRODUCT_NAME}
        XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION ${PRODUCT_BUILD_NUMBER}
        XCODE_ATTRIBUTE_MARKETING_VERSION ${PRODUCT_VERSION_STRING}
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "15.0"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"   # iPhone,iPad
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
        XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES"
    )

    qt_import_plugins(${CMAKE_PROJECT_NAME} INCLUDE Qt6::QIosOptionalPlugin_NSPhotoLibraryPlugin)

    # Workaround for the CMake version of https://bugreports.qt.io/browse/QTBUG-129651
    # ArcGIS Maps SDK for Qt adds 'INSTALL_RPATH = ${FFMPEG_FRAMEWORKS}'
    # which contains the path to the FFmpeg frameworks.
    # This ensures that the FFmpeg frameworks are included in the application bundle.
    qt_add_ios_ffmpeg_libraries(${CMAKE_PROJECT_NAME})
    get_target_property(Qt6Core_LOCATION Qt6::Core LOCATION)
    get_filename_component(Qt6_DIR ${Qt6Core_LOCATION} DIRECTORY)
    set(FFMPEG_LIB_DIR "${Qt6_DIR}/../ffmpeg")
    if(Qt6Core_VERSION VERSION_LESS "6.8.3")
        set(FRAMEWORK "framework")
    else()
        set(FRAMEWORK "xcframework")
    endif()

    set(FFMPEG_FRAMEWORKS
        "${FFMPEG_LIB_DIR}/libavcodec.${FRAMEWORK}"
        "${FFMPEG_LIB_DIR}/libavformat.${FRAMEWORK}"
        "${FFMPEG_LIB_DIR}/libavutil.${FRAMEWORK}"
        "${FFMPEG_LIB_DIR}/libswresample.${FRAMEWORK}"
        "${FFMPEG_LIB_DIR}/libswscale.${FRAMEWORK}")

    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "@executable_path/Frameworks;@loader_path/Frameworks"
        XCODE_EMBED_FRAMEWORKS "${ArcGISRuntime_runtimecore_LIB};${FFMPEG_FRAMEWORKS}"
        XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY TRUE
        LINK_FLAGS "-F${FFMPEG_LIB_DIR} -F${ArcGISRuntime_runtimecore_LIB_DIR} -Wl,-rpath,@executable_path -Wl,-rpath,@loader_path/Frameworks")
endif()

## Android
if(ANDROID)
    set(PROJECT_DEPLOYABLE_LIBS ${ArcGISRuntime_LIBRARIES})

    list(JOIN PROJECT_DEPLOYABLE_LIBS , PROJECT_DEPLOYABLE_LIBS_STRING)

    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${ArcGISRuntime_MapsSDKNative_JAR}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/android-build/libs/MapsSDKNative.jar")

    set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY QT_ANDROID_EXTRA_LIBS ${PROJECT_DEPLOYABLE_LIBS_STRING} ${ANDROID_EXTRA_LIBS})

    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        QT_ANDROID_ABIS "armeabi-v7a;arm64-v8a"
        QT_ANDROID_MIN_SDK_VERSION 28
        QT_ANDROID_TARGET_SDK_VERSION 35
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/platforms/Android/"
        QT_ANDROID_VERSION_NAME ${PRODUCT_VERSION}
        QT_ANDROID_VERSION_CODE "100${PRODUCT_VERSION}0"
    )

    include(FetchContent)
    FetchContent_Declare(
        android_openssl
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(android_openssl)

    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
    add_android_openssl_libraries(${CMAKE_PROJECT_NAME})
endif()

install(TARGETS ${CMAKE_PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
