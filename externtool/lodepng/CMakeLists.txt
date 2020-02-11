cmake_minimum_required(VERSION 3.4.1)

project(LuaCMake
    LANGUAGES CXX
)


set(extern_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../extern)


add_library(lib_lodepng STATIC
        ${extern_dir}/lodepng/lodepng.h
        ${extern_dir}/lodepng/lodepng.cpp
)
target_include_directories(lib_lodepng PUBLIC ${extern_dir}/lodepng)
