cmake_minimum_required(VERSION 3.6)
project(AndroidHideImport LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

add_library(xdl STATIC
    xdl/xdl.c
    xdl/xdl_iterate.c
    xdl/xdl_linker.c
    xdl/xdl_lzma.c
    xdl/xdl_util.c
)

target_include_directories(xdl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/xdl/include
)

add_executable(sample
    Main.cpp
)

target_link_libraries(sample PRIVATE xdl dl)

