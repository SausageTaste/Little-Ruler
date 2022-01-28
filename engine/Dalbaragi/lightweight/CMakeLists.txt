cmake_minimum_required(VERSION 3.4.1)

project(Dalbaragi-Lightweight
    LANGUAGES CXX
)


add_library(dalbaragi_lightweight
    d_pool.h
    u_byteutils.cpp  u_byteutils.h
    d_input_data.h   d_input_data.cpp
    d_aabb_2d.h      d_aabb_2d.cpp
)

target_compile_features(dalbaragi_lightweight PUBLIC cxx_std_17)

target_include_directories(dalbaragi_lightweight
    PUBLIC
        .
        ${extern_dir}/glm
)
