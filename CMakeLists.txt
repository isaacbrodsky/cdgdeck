cmake_minimum_required(VERSION 3.9)

project(CDGDeck LANGUAGES CXX)

find_package(ZLIB REQUIRED)
find_package(libpng CONFIG REQUIRED)
find_package(FLTK CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(sdl2-ttf CONFIG REQUIRED)
find_package(sdl2-mixer CONFIG REQUIRED)

if(WIN32)
    set(RC_FILE_NAME cdgproj.rc)
endif()
add_executable(cdgdeck
    cdg.cpp
    cdgmain.cpp
    cdgrenderer.cpp
    cdgui.cpp
    gen.cpp
    sdlutils.cpp
    vera.cpp
    ${RC_FILE_NAME})

target_link_libraries(cdgdeck PRIVATE ZLIB::ZLIB png fltk SDL2::SDL2 SDL2::SDL2_ttf SDL2::SDL2_mixer SDL2::SDL2main)
# Note: 4 target(s) were omitted.
#target_link_libraries(cdgdeck PRIVATE fltk fltk_gl fltk_forms fltk_SHARED)
#target_link_libraries(cdgdeck PRIVATE SDL2::SDL2 SDL2::SDL2main)
