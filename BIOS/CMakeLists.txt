cmake_minimum_required(VERSION 3.14)
project(baios VERSION 3.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -O2 -g)
add_definitions(-DBAIOS_DEBUG=1)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(BAIOS_SOURCES
    src/hw/phys_memory.c
    src/hw/interrupt.c
    src/hw/driver_compat.c
    src/hw/hw_kernel.c
    src/ipc/zero_copy.c
    src/sw/process_manager.c
    src/sw/sw_kernel.cpp
    src/boot/entry.c
)

add_executable(baios_stub ${BAIOS_SOURCES})
target_link_libraries(baios_stub pthread)

add_library(baios STATIC
    src/hw/phys_memory.c
    src/hw/interrupt.c
    src/hw/driver_compat.c
    src/hw/hw_kernel.c
    src/ipc/zero_copy.c
    src/sw/process_manager.c
    src/sw/sw_kernel.cpp
)
target_include_directories(baios PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
