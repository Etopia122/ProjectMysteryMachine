# New 3DS port. Game code is built the same way as for PC (pc/CMakeLists.txt).
#   cmake -S n3ds -B build/n3ds -G Ninja -DCMAKE_TOOLCHAIN_FILE=$PWD/n3ds/cmake/n3ds.cmake
cmake_minimum_required(VERSION 3.13)
project(scooby_n3ds C CXX ASM)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(PC ${ROOT}/pc)

file(GLOB GAME_SOURCES
    ${ROOT}/src/Core/x/*.cpp
    ${ROOT}/src/Core/gc/*.cpp ${ROOT}/src/Core/gc/*.c
    ${ROOT}/src/Game/*.cpp)
list(FILTER GAME_SOURCES EXCLUDE REGEX "/src/Core/gc/(iFMV\\.cpp|iRad3D\\.c)$")
set_source_files_properties(${GAME_SOURCES} PROPERTIES LANGUAGE CXX)

add_library(scoobygame STATIC ${GAME_SOURCES})
target_include_directories(scoobygame PUBLIC
    ${PC}/include
    ${ROOT}/include
    ${ROOT}/src/Core/x
    ${ROOT}/src/Core/gc
    ${ROOT}/src/Game)
target_compile_definitions(scoobygame PUBLIC PC_PORT=1 N3DS_PORT=1 VERSION_GIHE78 BUILD_VERSION=0)
target_compile_options(scoobygame PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++98 -fpermissive>
    -w -g -O0 -fno-strict-aliasing
    # The PC build (x86) has signed char; keep the same behaviour on ARM.
    -fsigned-char
    -fno-unreachable-traps
    -include ${PC}/include/pc_compat.h)
