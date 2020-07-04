cmake_minimum_required(VERSION 3.4.1)

project(Dalbaragi-Util
    LANGUAGES CXX
)


add_library(dalbaragi_util
    d_filesystem.h       d_filesystem.cpp
    d_fixednum.h
    d_logchannel.h
    d_logger.h           d_logger.cpp
    d_shaderProcessor.h  d_shaderProcessor.cpp
    s_configs.h          s_configs.cpp
    u_fileutils.h        u_fileutils.cpp
    u_imagebuf.h         u_imagebuf.cpp
    u_math.h             u_math.cpp
    u_strbuf.h
    u_timer.h            u_timer.cpp
    d_geometrymath.h     d_geometrymath.cpp
    d_transform.h        d_transform.cpp
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

# GLM
target_include_directories(dalbaragi_util PUBLIC ${extern_dir}/glm)

# TGA
target_include_directories(dalbaragi_util PRIVATE ${extern_dir}/TGA)


# Dalbaragi Lightweight
if (NOT TARGET dalbaragi_lightweight)
    add_subdirectory(../lightweight ${CMAKE_CURRENT_BINARY_DIR}/dalbaragi_lightweight)
    target_link_libraries(dalbaragi_util PUBLIC dalbaragi_lightweight)
endif()
