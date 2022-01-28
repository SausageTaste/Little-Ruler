cmake_minimum_required(VERSION 3.4.1)

project(Dalbaragi-ResParser
    LANGUAGES CXX
)


add_library(dalbaragi_resparser
    d_mapparser.h  d_mapparser.cpp
    d_mapdata.h    d_mapdata.cpp
)

target_compile_features(dalbaragi_resparser PUBLIC cxx_std_17)

target_include_directories(dalbaragi_resparser
    PUBLIC
        .
        ${extern_dir}/glm
)

target_link_libraries(dalbaragi_resparser
    PRIVATE
        dalbaragi_lightweight
)

# zlib
if (WIN32)
    target_link_libraries(dalbaragi_resparser PRIVATE zlibstatic)
    target_include_directories(dalbaragi_resparser PRIVATE ${extern_dir}/zlib ${zlib_bin_dir})
else()
    target_link_libraries(dalbaragi_resparser PRIVATE z)
endif()
