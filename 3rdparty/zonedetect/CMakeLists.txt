project(zonedetect LANGUAGES C)

set(CORE_COMPONENTS Core)
find_package(Qt${QT_DEFAULT_MAJOR_VERSION} REQUIRED COMPONENTS ${CORE_COMPONENTS})

set(CORE_LIBRARIES
    Qt::Core
)

set(CORE_SOURCES 
    zonedetect.c zonedetect.h
)

add_library(zonedetect OBJECT ${CORE_SOURCES} ${PLATFORM_SOURCES})

qt_add_resources(zonedetect "zonedetect"
    PREFIX "/"
    FILES
        timezone21.bin
)

target_compile_definitions(zonedetect PRIVATE ZD_EXPORT=)

target_link_libraries(zonedetect PRIVATE ${CORE_LIBRARIES} ${PLATFORM_LIBRARIES})
target_include_directories(zonedetect PUBLIC ${CMAKE_CURRENT_LIST_DIR})
