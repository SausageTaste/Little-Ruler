cmake_minimum_required(VERSION 3.11.0)

include(FetchContent)

set(repoRoot_dir ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(extern_dir ${repoRoot_dir}/extern)
set(externcpy_dir ${repoRoot_dir}/externcpy)
set(externtool_dir ${repoRoot_dir}/externtool)


# Import external libraries
# ------------------------------------------------------------------------------

# Freetype
add_subdirectory(${extern_dir}/freetype2 ${CMAKE_CURRENT_BINARY_DIR}/freetype2)

# entt
add_subdirectory(${extern_dir}/entt ${CMAKE_CURRENT_BINARY_DIR}/entt)

# Lodepng
add_subdirectory(${externtool_dir}/lodepng ${CMAKE_CURRENT_BINARY_DIR}/lodepng)

# Lua
add_subdirectory(${externtool_dir}/lua ${CMAKE_CURRENT_BINARY_DIR}/lua)

# DalbaragiTools
FetchContent_Declare(
    fetch_daltools
    GIT_REPOSITORY https://github.com/SausageTaste/DalbaragiTools
    GIT_TAG        7a08f2291d48c1be651bef4ee7cf72819b158451
)
FetchContent_MakeAvailable(fetch_daltools)

if (WIN32)
    # GLFW
    add_subdirectory(${extern_dir}/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)

    # GLaD
    add_subdirectory(${extern_dir}/glad ${CMAKE_CURRENT_BINARY_DIR}/glad)
endif()


# Add project subdirectories
# ------------------------------------------------------------------------------

add_subdirectory(./lightweight)
add_subdirectory(./util)
add_subdirectory(./physics)
add_subdirectory(./resparser)
add_subdirectory(./overlay)
add_subdirectory(./runtime)
