project(cablib LANGUAGES C)

set(CORE_SOURCES
    Cab_adler32.c
    Cab_deflate.c Cab_deflate.h
    Cab_trees.c
    Cab_zconf.h
    Cab_zlib.h
    Cab_zutil.c Cab_zutil.h
)

add_library(cablib OBJECT ${CORE_SOURCES})

target_include_directories(cablib PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(cablib PUBLIC Z_PREFIX)
