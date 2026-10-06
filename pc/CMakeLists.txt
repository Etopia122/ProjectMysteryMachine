cmake_minimum_required(VERSION 3.20)
project(ScoobyPC C CXX ASM)

# Configure (MSYS2 mingw32 shell or with C:/msys64/mingw32/bin on PATH):
#   cmake -S pc -B build/pc -G Ninja
#   cmake --build build/pc --target scoobygame
#   python pc/tools/genstubs.py
#   cmake --build build/pc

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Build with a 32-bit compiler (MSYS2 mingw32): game data assumes 4-byte pointers.")
endif()

file(GLOB GAME_SOURCES
    ${ROOT}/src/Core/x/*.cpp
    ${ROOT}/src/Core/gc/*.cpp ${ROOT}/src/Core/gc/*.c
    ${ROOT}/src/Game/*.cpp)
# The original build compiles the .c files as C++ too.
set_source_files_properties(${GAME_SOURCES} PROPERTIES LANGUAGE CXX)

add_library(scoobygame STATIC ${GAME_SOURCES})
target_include_directories(scoobygame PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ROOT}/include
    ${ROOT}/src/Core/x
    ${ROOT}/src/Core/gc
    ${ROOT}/src/Game)
target_compile_definitions(scoobygame PUBLIC PC_PORT=1 VERSION_GIHE78 BUILD_VERSION=0)
target_compile_options(scoobygame PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++98 -fpermissive>
    -w -g -O0 -fno-strict-aliasing
    -include ${CMAKE_CURRENT_SOURCE_DIR}/include/pc_compat.h)

file(GLOB PC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.S)
add_executable(scooby ${PC_SOURCES})
target_link_libraries(scooby PRIVATE scoobygame)
target_compile_options(scooby PRIVATE -g -O0)
