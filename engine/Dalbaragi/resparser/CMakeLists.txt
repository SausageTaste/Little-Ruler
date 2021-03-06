cmake_minimum_required(VERSION 3.4.1)

project(Dalbaragi-ResParser
    LANGUAGES CXX
)


add_library(dalbaragi_resparser
    d_mapparser.h  d_mapparser.cpp
    d_mapdata.h    d_mapdata.cpp
)
target_include_directories(dalbaragi_resparser PUBLIC .)
target_compile_features(dalbaragi_resparser PUBLIC cxx_std_17)


######## Import libraries ########

set(repoRoot_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(extern_dir ${repoRoot_dir}/extern)
set(externcpy_dir ${repoRoot_dir}/externcpy)
set(externtool_dir ${repoRoot_dir}/externtool)


# zlib
if (WIN32)
    add_subdirectory(${extern_dir}/zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib)
    target_link_libraries(dalbaragi_resparser PRIVATE zlibstatic)
    target_include_directories(dalbaragi_resparser PRIVATE ${extern_dir}/zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib)
else()
    target_link_libraries(dalbaragi_resparser PRIVATE z)
endif()

# GLM
target_include_directories(dalbaragi_resparser PUBLIC ${extern_dir}/glm)


# Dalbaragi Lightweight
if (NOT TARGET dalbaragi_lightweight)
    add_subdirectory(../lightweight ${CMAKE_CURRENT_BINARY_DIR}/dalbaragi_lightweight)
endif()
target_link_libraries(dalbaragi_resparser PUBLIC dalbaragi_lightweight)
