project(ecwlib LANGUAGES CXX)

set(SOURCES
    ecw.h
    NCSDefs.h
    NCSEcw.cpp NCSEcw.h
    NCSUtil.cpp
)

add_library(ecwlib OBJECT ${SOURCES})

target_include_directories(ecwlib PUBLIC ${CMAKE_CURRENT_LIST_DIR})
