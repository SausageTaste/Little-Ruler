cmake_minimum_required(VERSION 3.4.1)

project(Little-Ruler
    VERSION        0.1.0
    DESCRIPTION    ""
    LANGUAGES      CXX
)


add_executable(little_ruler
    main.cpp
)
target_include_directories(little_ruler PUBLIC .)
target_compile_features(little_ruler PUBLIC cxx_std_17)


# Dalbaragi Runtime
if (NOT TARGET dalbaragi_runtime)
    add_subdirectory(../Dalbaragi/runtime ${CMAKE_CURRENT_BINARY_DIR}/dalbaragi_runtime)
    target_link_libraries(little_ruler PUBLIC dalbaragi_runtime)
endif()
