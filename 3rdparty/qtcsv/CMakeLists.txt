project (qtcsv LANGUAGES CXX)

option(STATIC_LIB "build as static lib if ON, otherwise build shared lib" ON)

set(CORE_COMPONENTS Core)
find_package(Qt${QT_DEFAULT_MAJOR_VERSION} REQUIRED COMPONENTS ${CORE_COMPONENTS})

set(CORE_LIBRARIES
    Qt::Core
)

set(CORE_SOURCES
    abstractdata.h
    filechecker.h
    qtcsv_global.h
    symbols.h
    contentiterator.cpp contentiterator.h
    reader.cpp reader.h
    stringdata.cpp stringdata.h
    variantdata.cpp variantdata.h
    writer.cpp writer.h
)

add_library(qtcsv OBJECT ${CORE_SOURCES} ${PLATFORM_SOURCES})

target_compile_definitions(qtcsv PRIVATE QTCSV_LIBRARY)

target_link_libraries(qtcsv PRIVATE ${CORE_LIBRARIES} ${PLATFORM_LIBRARIES})
target_include_directories(qtcsv PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# set compiler flags for the library target
# if (CMAKE_COMPILER_IS_GNUCXX)
#     target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror -Wformat=2
#         -Wuninitialized -Winit-self -Wswitch-enum -Wundef
#         -Wpointer-arith -Wdisabled-optimization -Wcast-align -Wcast-qual)
# endif()
