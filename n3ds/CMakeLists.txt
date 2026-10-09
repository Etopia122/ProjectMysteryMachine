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
# Debug: N3DS_O0_UNITS=<regex> builds the matching game sources without
# optimisation (bisecting code that only misbehaves at -O2).
set(N3DS_O0_UNITS "" CACHE STRING "Regex of game sources to build at -O0")
if(N3DS_O0_UNITS)
    foreach(src ${GAME_SOURCES})
        if(src MATCHES "${N3DS_O0_UNITS}")
            set_source_files_properties(${src} PROPERTIES COMPILE_OPTIONS "-O0")
        endif()
    endforeach()
endif()
target_include_directories(scoobygame PUBLIC
    ${PC}/include
    ${ROOT}/include
    ${ROOT}/src/Core/x
    ${ROOT}/src/Core/gc
    ${ROOT}/src/Game)
target_compile_definitions(scoobygame PUBLIC PC_PORT=1 N3DS_PORT=1 VERSION_GIHE78 BUILD_VERSION=0)
target_compile_options(scoobygame PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++98 -fpermissive>
    -w -g -O2 -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks -fno-aggressive-loop-optimizations
    # The PC build (x86) has signed char; keep the same behaviour on ARM.
    -fsigned-char
    # Bare-metal ARM makes enums as small as their values; the game's structs
    # and asset layouts have 4-byte enums (as on GameCube and PC).
    -fno-short-enums
    -fno-unreachable-traps
    -include ${PC}/include/pc_compat.h)

# Platform code shared with the PC port, and the 3DS platform layer.
set(SHARED_PC_SOURCES
    ${PC}/src/aram.cpp ${PC}/src/asset_swap.cpp ${PC}/src/autopad.cpp ${PC}/src/audio.cpp ${PC}/src/card.cpp ${PC}/src/dvd.cpp
    ${PC}/src/game_data.cpp ${PC}/src/game_leftovers.cpp ${PC}/src/irq.cpp ${PC}/src/msl_data.cpp
    ${PC}/src/os_heap.cpp ${PC}/src/vi_time.cpp ${PC}/src/xbase_outofline.cpp)
file(GLOB PC_RW_SOURCES ${PC}/rw/*.cpp)
list(FILTER PC_RW_SOURCES EXCLUDE REGEX "/gpu_gl[.]cpp$")
file(GLOB N3DS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.S)

add_executable(scooby ${SHARED_PC_SOURCES} ${PC_RW_SOURCES} ${N3DS_SOURCES})
target_include_directories(scooby PRIVATE ${PC}/include ${PC}/rw)
target_compile_options(scooby PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17> -g -O2 -fsigned-char)
n3ds_add_shader(scooby ${CMAKE_CURRENT_SOURCE_DIR}/src/scene.v.pica)
target_link_libraries(scooby PRIVATE scoobygame citro3d ctru m -Wl,--no-enum-size-warning)
# Shared platform and RenderWare code shares structs with the game: 4-byte
# enums too. n3ds/src keeps the toolchain default, which libctru and citro3d
# (with enum fields in their structs) are built with.
set_source_files_properties(${SHARED_PC_SOURCES} ${PC_RW_SOURCES} PROPERTIES COMPILE_OPTIONS "-fno-short-enums")
n3ds_add_3dsx(scooby "Scooby-Doo! Night of 100 Frights" "Experimental New 3DS port")
