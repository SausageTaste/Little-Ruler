cmake_minimum_required(VERSION 3.4.1)

project(Dalbaragi-Util
    LANGUAGES CXX
)


add_library(dalbaragi_util
    d_filesystem.cpp
    d_filesystem.h
    d_fixednum.h
    d_logchannel.h
    d_logger.cpp
    d_logger.h
    d_pool.h
    d_shaderProcessor.cpp
    d_shaderProcessor.h
    s_configs.cpp
    s_configs.h
    u_byteutils.cpp
    u_byteutils.h
    u_fileutils.cpp
    u_fileutils.h
    u_imagebuf.cpp
    u_imagebuf.h
    u_math.cpp
    u_math.h
    u_strbuf.h
    u_timer.cpp
    u_timer.h
)
target_include_directories(dalbaragi_util PUBLIC .)
target_compile_features(dalbaragi_util PUBLIC cxx_std_17)


######## Import libraries ########

set(repoRoot_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(extern_dir ${repoRoot_dir}/extern)
set(externcpy_dir ${repoRoot_dir}/externcpy)
set(externtool_dir ${repoRoot_dir}/externtool)


# Lodepng
if (NOT TARGET lib_lodepng)
    add_subdirectory(${externtool_dir}/lodepng ${CMAKE_CURRENT_BINARY_DIR}/lodepng)
    target_link_libraries(dalbaragi_util PRIVATE lib_lodepng)
endif()

# fmt
if (NOT TARGET fmt::fmt)
    add_subdirectory(${extern_dir}/fmt ${CMAKE_CURRENT_BINARY_DIR}/fmt)
    target_link_libraries(dalbaragi_util PUBLIC fmt::fmt)
endif()

# zlib
if (NOT TARGET zlibstatic)
    add_subdirectory(${extern_dir}/zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib)
    target_link_libraries(dalbaragi_util PRIVATE zlibstatic)
    target_include_directories(dalbaragi_util PRIVATE ${extern_dir}/zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib)
endif()

# GLM
target_include_directories(dalbaragi_util PUBLIC ${extern_dir}/glm)

# TGA
target_include_directories(dalbaragi_util PRIVATE ${extern_dir}/TGA)
