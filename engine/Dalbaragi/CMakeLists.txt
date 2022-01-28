cmake_minimum_required(VERSION 3.11.0)


set(repoRoot_dir ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(extern_dir ${repoRoot_dir}/extern)
set(externcpy_dir ${repoRoot_dir}/externcpy)
set(externtool_dir ${repoRoot_dir}/externtool)


# Import external libraries
# ------------------------------------------------------------------------------

# fmt
add_subdirectory(${extern_dir}/fmt ${CMAKE_CURRENT_BINARY_DIR}/fmt)

# Freetype
add_subdirectory(${extern_dir}/freetype2 ${CMAKE_CURRENT_BINARY_DIR}/freetype2)

# entt
add_subdirectory(${extern_dir}/entt ${CMAKE_CURRENT_BINARY_DIR}/entt)

# Lodepng
add_subdirectory(${externtool_dir}/lodepng ${CMAKE_CURRENT_BINARY_DIR}/lodepng)

# Lua
add_subdirectory(${externtool_dir}/lua ${CMAKE_CURRENT_BINARY_DIR}/lua)

if (WIN32)
    # zlib
    set(zlib_bin_dir ${CMAKE_CURRENT_BINARY_DIR}/zlib)
    add_subdirectory(${extern_dir}/zlib ${zlib_bin_dir})

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
