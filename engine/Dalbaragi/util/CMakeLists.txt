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
    d_debugview.h        d_debugview.cpp
)

target_compile_features(dalbaragi_util PUBLIC cxx_std_17)

target_include_directories(dalbaragi_util
    PUBLIC
        .
        ${extern_dir}/TGA
)

target_link_libraries(dalbaragi_util
    PUBLIC
        glm::glm
    PRIVATE
        fmt::fmt
        lib_lodepng
        dalbaragi_lightweight
)
