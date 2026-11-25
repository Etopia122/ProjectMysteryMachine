cmake_minimum_required(VERSION 3.20)
project(ProjectMysteryMachine C CXX ASM)

# Configure (MSYS2 mingw32 shell or with C:/msys64/mingw32/bin on PATH):
#   cmake -S pc -B build/pc -G Ninja
#   cmake --build build/pc --target scoobygame
#   python pc/tools/genstubs.py
#   cmake --build build/pc

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Reproducible programs: no build timestamp in the PE header, and __FILE__ and
# debug paths mapped off the checkout path, so the same source and toolchain
# link to byte-identical exes. package.py then packs them with fixed entry
# times, which is what makes the published SHA-256 checkable by rebuilding.
# The toolchain's headers are recorded with their install path too (asserts and
# __FILE__ in SDL2 and the system headers), so map that as well: the same source
# and the same MSYS2 packages then build to the same bytes on any machine.
# build_ffmpeg.sh applies the same maps to FFmpeg's static libraries.
get_filename_component(PC_TOOLCHAIN_BIN "${CMAKE_C_COMPILER}" DIRECTORY)
get_filename_component(PC_TOOLCHAIN_PREFIX "${PC_TOOLCHAIN_BIN}" DIRECTORY)
add_compile_options(-ffile-prefix-map=${ROOT}=. -ffile-prefix-map=${PC_TOOLCHAIN_PREFIX}=/mingw32)
set(PC_REPRODUCIBLE_LINK -Wl,--no-insert-timestamp)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Build with a 32-bit compiler (MSYS2 mingw32): game data assumes 4-byte pointers.")
endif()

file(GLOB GAME_SOURCES
    ${ROOT}/src/Core/x/*.cpp
    ${ROOT}/src/Core/gc/*.cpp ${ROOT}/src/Core/gc/*.c
    ${ROOT}/src/Game/*.cpp)
# Platform files replaced on PC (pc/src): the Bink/GX movie player and its
# GX blitter.
list(FILTER GAME_SOURCES EXCLUDE REGEX "/src/Core/gc/(iFMV\.cpp|iRad3D\.c)$")
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
    # Decompiled functions sometimes fall off the end without a return; on
    # GameCube that returns whatever is in r3. Don't let GCC turn it into a trap.
    -fno-unreachable-traps
    -include ${CMAKE_CURRENT_SOURCE_DIR}/include/pc_compat.h)

# Debug: report call sites that leak values on the x87 stack (see src/fpcheck.cpp).
option(PC_FPCHECK "Instrument game code to find x87 stack leaks" OFF)
if(PC_FPCHECK)
    target_compile_options(scoobygame PRIVATE -finstrument-functions)
endif()

find_package(SDL2 REQUIRED CONFIG)

# Recomp Achievements: the client that reports achievements to RecompLauncher
# over its local channel (pc/src/achievements.cpp, pc/third_party). Without
# RecompLauncher it does nothing, so it is linked into every build.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/rl-achievements
    ${CMAKE_BINARY_DIR}/rl-achievements)

file(GLOB PC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.S
    ${CMAKE_CURRENT_SOURCE_DIR}/rw/*.cpp)
# The PC platform sources build once and link into two executables:
#   scooby.exe        release: Windows GUI subsystem, so no console window.
#   scooby_debug.exe  testing: console subsystem, so pc_log and the stub
#                     reports are visible. Never packaged.
add_library(scooby_pc OBJECT ${PC_SOURCES})
target_include_directories(scooby_pc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/rw)
target_compile_options(scooby_pc PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17> -g -O0 -pthread)
# Only for the usage requirements (includes, PC_PORT, ...) when compiling the
# PC sources; the executables below link it again for real.
target_link_libraries(scooby_pc PRIVATE scoobygame SDL2::SDL2 rl_achievements)

add_executable(scooby $<TARGET_OBJECTS:scooby_pc>)
# PC_EXE_NAME lets a test build link while the normal exe is running.
set(PC_EXE_NAME "scooby" CACHE STRING "Output name of the executable")
set_target_properties(scooby PROPERTIES OUTPUT_NAME ${PC_EXE_NAME})
# FFmpeg: the minimal static Bink-only build from pc/tools/build_ffmpeg.sh if
# present (no FFmpeg DLLs), else MSYS2's full shared FFmpeg.
set(PC_FFMPEG_DIR "${ROOT}/build/ffmpeg/install" CACHE PATH "Minimal static FFmpeg install prefix")
if(EXISTS "${PC_FFMPEG_DIR}/lib/libavcodec.a")
    message(STATUS "FFmpeg: static from ${PC_FFMPEG_DIR}")
    set(PC_FFMPEG_INCLUDE "${PC_FFMPEG_DIR}/include")
    set(PC_FFMPEG_LIBS
        ${PC_FFMPEG_DIR}/lib/libavformat.a ${PC_FFMPEG_DIR}/lib/libavcodec.a
        ${PC_FFMPEG_DIR}/lib/libswscale.a ${PC_FFMPEG_DIR}/lib/libswresample.a
        ${PC_FFMPEG_DIR}/lib/libavutil.a ole32 user32 bcrypt -l:libatomic.a)
else()
    message(STATUS "FFmpeg: shared from MSYS2")
    set(PC_FFMPEG_INCLUDE "")
    set(PC_FFMPEG_LIBS avformat avcodec swscale swresample avutil)
endif()
target_include_directories(scooby_pc PRIVATE ${PC_FFMPEG_INCLUDE})
# -mwindows sets the GUI subsystem: main() still runs, but Windows never
# creates a console for the game.
target_link_options(scooby PRIVATE -mwindows ${PC_REPRODUCIBLE_LINK})
target_link_libraries(scooby PRIVATE scoobygame SDL2::SDL2 opengl32 rl_achievements
    ${PC_FFMPEG_LIBS} -Wl,--large-address-aware)
# Link the GCC runtime statically. scooby.exe then needs only SDL2.dll; the
# launcher's C++ runtime still imports libwinpthread-1.dll, which package.py
# copies into the release next to SDL2.dll.
target_link_options(scooby PRIVATE -static-libgcc -static-libstdc++
    -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive,-Bdynamic
    ${PC_REPRODUCIBLE_LINK})

# Debug/testing build: same game, console subsystem so pc_log output and stub
# reports are readable. It is not stripped and not packaged (package.py only
# copies scooby.exe and scooby_launcher.exe).
add_executable(scooby_debug $<TARGET_OBJECTS:scooby_pc>)
set_target_properties(scooby_debug PROPERTIES OUTPUT_NAME "${PC_EXE_NAME}_debug")
target_link_libraries(scooby_debug PRIVATE scoobygame SDL2::SDL2 opengl32 rl_achievements
    ${PC_FFMPEG_LIBS} -Wl,--large-address-aware)
target_link_options(scooby_debug PRIVATE -static-libgcc -static-libstdc++
    -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive,-Bdynamic
    ${PC_REPRODUCIBLE_LINK})

# The launcher: extracts the game files from a disc image, edits scooby.ini,
# starts scooby.exe (pc/launcher).
add_executable(scooby_launcher WIN32 launcher/launcher.cpp launcher/launcher.rc)
set_property(SOURCE launcher/launcher.rc PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/launcher/launcher.manifest;${CMAKE_CURRENT_SOURCE_DIR}/launcher/launcher.ico;${CMAKE_CURRENT_SOURCE_DIR}/launcher/logo.png;${CMAKE_CURRENT_SOURCE_DIR}/launcher/scooby.png;${CMAKE_CURRENT_SOURCE_DIR}/launcher/van.png")
target_compile_definitions(scooby_launcher PRIVATE UNICODE _UNICODE)
target_compile_options(scooby_launcher PRIVATE -std=gnu++17 -O2 -municode)
target_link_options(scooby_launcher PRIVATE -municode -static-libgcc -static-libstdc++
    ${PC_REPRODUCIBLE_LINK})
target_link_libraries(scooby_launcher PRIVATE SDL2::SDL2 comctl32 comdlg32 shell32 ole32 dwmapi msimg32 uxtheme windowscodecs uuid)
