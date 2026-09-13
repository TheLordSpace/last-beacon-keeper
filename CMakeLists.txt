cmake_minimum_required(VERSION 3.16)
project(TheLastBeaconKeeper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
pkg_check_modules(FRIBIDI REQUIRED fribidi)

add_executable(beacon_keeper
    src/main.cpp
    src/Localization.cpp
    src/Audio.cpp
    src/Particles.cpp
    src/Map.cpp
    src/Entities.cpp
    src/Lighthouse.cpp
    src/UIManager.cpp
    src/InputHandler.cpp
    src/Game.cpp
)

target_include_directories(beacon_keeper PRIVATE
    src
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
    ${FRIBIDI_INCLUDE_DIRS}
)

target_link_libraries(beacon_keeper PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    ${FRIBIDI_LIBRARIES}
    pthread
    m
)
