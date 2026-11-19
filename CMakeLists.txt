cmake_minimum_required(VERSION 3.20)
project(Flare C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Configure raylib build options
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
set(USE_AUDIO OFF CACHE BOOL "" FORCE)
set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/raylib)

# Source files
set(SOURCES
    src/main.c
    src/window.c
    src/window.h
    src/renderer.c
    src/renderer.h
    src/animations.c
    src/animations.h
    src/keyboard.c
    src/keyboard.h
    src/search.c
    src/search.h
    src/applications.c
    src/applications.h
    src/calc.c
    src/calc.h
)

add_executable(Flare ${SOURCES})

target_include_directories(Flare PRIVATE
    src
    vendor/raylib/src
)

target_link_libraries(Flare PRIVATE
    raylib
    user32
    gdi32
    shell32
    winmm
)

if (MSVC)
    target_compile_options(Flare PRIVATE /W3 /D_CRT_SECURE_NO_WARNINGS)
    target_link_options(Flare PRIVATE "$<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>" "$<$<CONFIG:Release>:/ENTRY:mainCRTStartup>")
else()
    target_compile_options(Flare PRIVATE -Wall)
    target_link_options(Flare PRIVATE "$<$<CONFIG:Release>:-mwindows>")
endif()
