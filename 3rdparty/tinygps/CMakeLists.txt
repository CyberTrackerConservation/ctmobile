project(tinygps LANGUAGES CXX)

set(CORE_COMPONENTS Core)
find_package(Qt${QT_DEFAULT_MAJOR_VERSION} REQUIRED COMPONENTS ${CORE_COMPONENTS})

set(CORE_LIBRARIES
    Qt::Core
)

set(CORE_SOURCES
    TinyGPS++.cpp TinyGPS++.h TinyGPSPlus.h
)

add_library(tinygps OBJECT ${CORE_SOURCES} ${PLATFORM_SOURCES})

target_link_libraries(tinygps PRIVATE ${CORE_LIBRARIES} ${PLATFORM_LIBRARIES})
target_include_directories(tinygps PUBLIC ${CMAKE_CURRENT_LIST_DIR})
