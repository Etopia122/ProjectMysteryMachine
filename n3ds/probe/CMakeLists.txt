# New 3DS bring-up probe (toolchain, GPU, memory, SD access).
#   cmake -S n3ds/probe -B build/n3ds-probe -G Ninja -DCMAKE_TOOLCHAIN_FILE=n3ds/cmake/n3ds.cmake
cmake_minimum_required(VERSION 3.13)
project(scooby_probe C ASM)

add_executable(scooby_probe main.c)
n3ds_add_shader(scooby_probe vshader.v.pica)
target_link_libraries(scooby_probe PRIVATE citro3d ctru m)
n3ds_add_3dsx(scooby_probe "Scooby probe" "New 3DS bring-up checks")
