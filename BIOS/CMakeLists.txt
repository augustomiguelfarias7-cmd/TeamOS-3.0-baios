cmake_minimum_required(VERSION 3.14)
project(Baios VERSION 3.0.0 LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -O2 -g)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(BAIOS_C_SOURCES
    ipc/shared_memory.c
    ipc/zero_copy.c
    hw/memory.c
    hw/interrupts.c
    hw/power.c
    hw/linux_compat.c
    hw/main.c
    common/utils.c
    demo_main.c
)

set(BAIOS_CXX_SOURCES
    sw/process_manager.cpp
    sw/filesystem.cpp
    sw/permissions.cpp
    sw/ipc_bridge.cpp
    sw/services/audio.cpp
    sw/services/network.cpp
    sw/services/render.cpp
    sw/main.cpp
)

set(BAIOS_ASM_SOURCES
    hw/entry.S
)

add_executable(baios_demo
    ${BAIOS_C_SOURCES}
    ${BAIOS_CXX_SOURCES}
    ${BAIOS_ASM_SOURCES}
)

target_link_libraries(baios_demo PRIVATE pthread rt)

install(TARGETS baios_demo DESTINATION bin)
