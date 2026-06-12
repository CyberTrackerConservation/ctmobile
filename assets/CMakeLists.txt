find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(Assets OBJECT)

qt_add_resources(Assets "config" PREFIX "/" FILES
    config/config.json
    config/languages.json
    config/projections.json
)

qt_add_resources(Assets "app" PREFIX "/" FILES
    app/appicon.svg
    app/esrilogo.svg
    app/legalAtlas.svg
    app/qtlogo.svg
    app/target.svg
    app/targetDark.svg
    app/trackTemplate.kml
    app/gabon.nmea
)

qt_add_resources(Assets "integrations" PREFIX "/" FILES
    Classic/logo.svg
    Classic/World.ecw
    Classic/BioSample.CTS
    Classic/BioSample.png
    Classic/PhotoNotes.CTS
    Classic/PhotoNotes.png
    Classic/Simple.CTS
    Classic/Simple.png

    CTO/index.svg
    CTO/logo.svg
    CTO/map.svg
    CTO/save.svg
    CTO/back.svg
    CTO/next.svg

    ConSoSci/logo.svg

    EarthRanger/logo.svg
    EarthRanger/logoDark.svg

    Esri/logo.svg
    Esri/Survey123.svg
    Esri/CreateService.json
    Esri/CreateLayers.json
    Esri/LocationTrackingXlsForm.png

    Google/ggforms.svg
    Google/GoogleLogin.svg
    Google/forms.svg
    Google/logo.svg

    KoBo/logo.svg

    ODK/logo.svg

    SMART/logo.svg
    SMART/logoDark.svg
    SMART/logo2.svg
    SMART/logo3.svg

    TrillionTrees/logo.svg
)

# Country flags
file(GLOB CF_FILES "countryflags/*.svg")
foreach(FILE IN LISTS CF_FILES)
    file(RELATIVE_PATH REL_FILE ${CMAKE_CURRENT_SOURCE_DIR} ${FILE})
    list(APPEND RELATIVE_CF_FILES ${REL_FILE})
endforeach()
qt_add_resources(Assets "countryflags" PREFIX "/" FILES ${RELATIVE_CF_FILES})

# Map symbols
qt_add_resources(Assets "map" PREFIX "/" FILES
    map/acquiringSymbol.svg
    map/courseSymbol.svg
    map/defaultSymbol.svg
    map/headingSymbol.svg
)

# Icons
file(GLOB ICON_FILES "icons/*.svg")
foreach(FILE IN LISTS ICON_FILES)
    file(RELATIVE_PATH REL_FILE ${CMAKE_CURRENT_SOURCE_DIR} ${FILE})
    list(APPEND RELATIVE_ICON_FILES ${REL_FILE})
endforeach()
qt_add_resources(Assets "icons" PREFIX "/" FILES ${RELATIVE_ICON_FILES})

# Translations
file(GLOB QM_FILES "i18n/*.qm")
foreach(FILE IN LISTS QM_FILES)
    file(RELATIVE_PATH REL_FILE ${CMAKE_CURRENT_SOURCE_DIR} ${FILE})
    list(APPEND RELATIVE_QM_FILES ${REL_FILE})
endforeach()
qt_add_resources(Assets "i18n" PREFIX "/" FILES ${RELATIVE_QM_FILES})
