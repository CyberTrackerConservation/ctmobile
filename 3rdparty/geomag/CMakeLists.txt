project(geomag LANGUAGES C CXX)

set(CORE_COMPONENTS Core)
find_package(Qt${QT_DEFAULT_MAJOR_VERSION} REQUIRED COMPONENTS ${CORE_COMPONENTS})

set(CORE_LIBRARIES
    Qt::Core
)

set(CORE_SOURCES
    geomag.cpp geomag.h
    GeomagnetismLibrary.c GeomagnetismHeader.h
)

add_library(geomag OBJECT ${CORE_SOURCES} ${PLATFORM_SOURCES})

qt_add_resources(geomag "geomag"
    PREFIX "/"
    FILES
        WMM.COF
)

target_compile_definitions(geomag PRIVATE ZD_EXPORT=)

target_link_libraries(geomag PRIVATE ${CORE_LIBRARIES} ${PLATFORM_LIBRARIES})
target_include_directories(geomag PUBLIC ${CMAKE_CURRENT_LIST_DIR})
