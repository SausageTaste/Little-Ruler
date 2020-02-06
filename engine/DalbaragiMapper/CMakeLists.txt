cmake_minimum_required(VERSION 3.4.1)

# Set this value to your local Qt5 installation path.
set (CMAKE_PREFIX_PATH "C:/Qt/5.12.1/msvc2017_64")

project(DalbaragiMapper 
    VERSION        0.1.0
    DESCRIPTION    "Map editor for Dalbaragi engine."
    LANGUAGES      CXX
)


add_executable(dalmapper
    d_dalbaragimapper.cpp
    d_dalbaragimapper.h
    d_daldef.h
    d_graphics_view.cpp
    d_graphics_view.h
    d_meshgeo.cpp
    d_meshgeo.h
    d_opengl.cpp
    d_opengl.h
    d_opengl_renderunit.cpp
    d_opengl_renderunit.h
    d_opengl_shader.cpp
    d_opengl_shader.h
    d_opengl_state.cpp
    d_opengl_state.h
    d_opengl_texture.cpp
    d_opengl_texture.h
    d_property_view.cpp
    d_property_view.h
    d_scene.cpp
    d_scene.h
    d_sharedinfo.cpp
    d_sharedinfo.h
    d_time.cpp
    d_time.h
    d_uniloc.cpp
    d_uniloc.h
    Fixed.h
    main.cpp
)
target_include_directories(dalmapper PRIVATE .)
target_compile_features(dalmapper PUBLIC cxx_std_17)


######## Import libraries ########

set(extern_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../extern)
set(lib_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)


# entt
add_subdirectory(${extern_dir}/entt ${CMAKE_CURRENT_BINARY_DIR}/entt)
target_link_libraries(dalmapper EnTT::EnTT)

# Qt5
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets Core REQUIRED)
set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_RELEASE} -fprofile-arcs -ftest-coverage")
set_target_properties(Qt5::Core PROPERTIES MAP_IMPORTED_CONFIG_COVERAGE "RELEASE")
target_link_libraries(dalmapper Qt5::Widgets Qt5::Core)


# Dalbaragi utils
add_subdirectory(../Dalbaragi/util ${CMAKE_CURRENT_BINARY_DIR}/dalbaragi_util)
target_link_libraries(dalmapper dalbaragi_util)
