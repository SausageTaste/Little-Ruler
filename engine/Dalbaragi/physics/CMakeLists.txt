cmake_minimum_required(VERSION 3.4.1)

project(Dalbaragi-Physics
    LANGUAGES CXX
)


add_library(dalbaragi_physics
    d_precision.h    d_precision.cpp
    d_particle.h     d_particle.cpp
    d_phyworld.h     d_phyworld.cpp
    d_modifierabc.h
    d_modifiers.h    d_modifiers.cpp
    d_collider.h     d_collider.cpp
)

target_compile_features(dalbaragi_physics PUBLIC cxx_std_17)

target_include_directories(dalbaragi_physics
    PUBLIC
        .
)

target_link_libraries(dalbaragi_physics
    PRIVATE
        EnTT::EnTT
        dalbaragi_util
)
