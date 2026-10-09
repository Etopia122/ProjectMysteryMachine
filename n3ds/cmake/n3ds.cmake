# CMake toolchain for the New 3DS port, used with the mingw32 cmake + ninja the
# PC build already uses. devkitPro's own CMake files insist on MSYS2's cmake, so
# this calls devkitARM and the devkitPro tools directly.
#   cmake -S n3ds -B build/n3ds -G Ninja -DCMAKE_TOOLCHAIN_FILE=n3ds/cmake/n3ds.cmake
cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED DEVKITPRO)
    if(DEFINED ENV{DEVKITPRO} AND NOT "$ENV{DEVKITPRO}" MATCHES "^/opt")
        set(DEVKITPRO "$ENV{DEVKITPRO}")
    else()
        set(DEVKITPRO "C:/msys64/opt/devkitpro")
    endif()
endif()
set(DEVKITPRO "${DEVKITPRO}" CACHE PATH "devkitPro root")
set(DEVKITARM "${DEVKITPRO}/devkitARM")
set(CTR_ROOT "${DEVKITPRO}/libctru")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR armv6k)
set(CMAKE_C_COMPILER "${DEVKITARM}/bin/arm-none-eabi-gcc.exe")
set(CMAKE_CXX_COMPILER "${DEVKITARM}/bin/arm-none-eabi-g++.exe")
set(CMAKE_ASM_COMPILER "${DEVKITARM}/bin/arm-none-eabi-gcc.exe")
set(CMAKE_AR "${DEVKITARM}/bin/arm-none-eabi-gcc-ar.exe" CACHE FILEPATH "")
set(CMAKE_RANLIB "${DEVKITARM}/bin/arm-none-eabi-gcc-ranlib.exe" CACHE FILEPATH "")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

set(N3DS_ARCH "-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft")
set(N3DS_COMMON "${N3DS_ARCH} -mword-relocations -ffunction-sections -D__3DS__ -isystem ${CTR_ROOT}/include -isystem ${DEVKITPRO}/portlibs/3ds/include")
set(CMAKE_C_FLAGS_INIT "${N3DS_COMMON}")
set(CMAKE_CXX_FLAGS_INIT "${N3DS_COMMON}")
set(CMAKE_ASM_FLAGS_INIT "${N3DS_ARCH}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${N3DS_ARCH} -specs=3dsx.specs -L${CTR_ROOT}/lib -L${DEVKITPRO}/portlibs/3ds/lib")

set(CMAKE_FIND_ROOT_PATH "${DEVKITARM}" "${CTR_ROOT}" "${DEVKITPRO}/portlibs/3ds")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(N3DS_TOOLS "${DEVKITPRO}/tools/bin")

# n3ds_add_shader(<target> <file.v.pica>): assembles a PICA200 shader with
# picasso and links it in as <name>_shbin / <name>_shbin_size (header
# <name>_shbin.h in the build directory).
function(n3ds_add_shader target src)
    get_filename_component(name "${src}" NAME_WE)
    get_filename_component(abs "${src}" ABSOLUTE)
    set(out "${CMAKE_CURRENT_BINARY_DIR}/${name}.shbin")
    set(asm "${CMAKE_CURRENT_BINARY_DIR}/${name}_shbin.s")
    set(hdr "${CMAKE_CURRENT_BINARY_DIR}/${name}_shbin.h")
    add_custom_command(
        OUTPUT "${asm}" "${hdr}"
        COMMAND "${N3DS_TOOLS}/picasso.exe" -o "${out}" "${abs}"
        COMMAND "${N3DS_TOOLS}/bin2s.exe" -a 4 -H "${hdr}" "${out}" > "${asm}"
        DEPENDS "${abs}"
        COMMENT "Assembling shader ${name}"
        VERBATIM)
    target_sources(${target} PRIVATE "${asm}" "${hdr}")
    set_source_files_properties("${asm}" PROPERTIES LANGUAGE ASM)
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
endfunction()

# n3ds_add_3dsx(<target> <title> <description>): packs <target>.elf into
# <target>.3dsx with an SMDH (title, icon) for the Homebrew Launcher.
function(n3ds_add_3dsx target title desc)
    set(smdh "${CMAKE_CURRENT_BINARY_DIR}/${target}.smdh")
    set(out "${CMAKE_CURRENT_BINARY_DIR}/${target}.3dsx")
    add_custom_command(
        OUTPUT "${out}"
        COMMAND "${N3DS_TOOLS}/smdhtool.exe" --create "${title}" "${desc}" "Scooby decomp" "${CTR_ROOT}/default_icon.png" "${smdh}"
        COMMAND "${N3DS_TOOLS}/3dsxtool.exe" "$<TARGET_FILE:${target}>" "${out}" "--smdh=${smdh}"
        DEPENDS ${target}
        COMMENT "Packing ${target}.3dsx"
        VERBATIM)
    add_custom_target(${target}_3dsx ALL DEPENDS "${out}")
endfunction()
