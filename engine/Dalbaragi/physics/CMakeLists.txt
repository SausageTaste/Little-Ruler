cmake_minimum_required(VERSION 3.4.1)

project(Dalbaragi-Physics
    LANGUAGES CXX
)


add_library(dalbaragi_physics
    d_precision.h    d_precision.cpp
    d_geometrymath.h d_geometrymath.cpp
    d_particle.h     d_particle.cpp
    d_phyworld.h     d_phyworld.cpp
    d_modifierabc.h
    d_modifiers.h    d_modifiers.cpp
)
target_include_directories(dalbaragi_physics PUBLIC .)
target_compile_features(dalbaragi_physics PUBLIC cxx_std_17)


######## Import libraries ########

set(extern_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../extern)

# entt
if (NOT TARGET EnTT::EnTT)
    add_subdirectory(${extern_dir}/entt ${CMAKE_CURRENT_BINARY_DIR}/entt)
endif()
target_link_libraries(dalbaragi_physics PUBLIC EnTT::EnTT)

# GLM
target_include_directories(dalbaragi_physics PUBLIC ${extern_dir}/glm)
