cmake_minimum_required(VERSION 3.4.1)


project(Little-Ruler
    VERSION        0.1.0
    DESCRIPTION    ""
    LANGUAGES      CXX
)


add_executable(little_ruler
    main.cpp
)

######## Import libraries ########

# Dalbaragi Runtime
add_subdirectory(../Dalbaragi/runtime ${CMAKE_CURRENT_BINARY_DIR}/dalbaragi_runtime)
target_link_libraries(little_ruler PUBLIC dalbaragi_runtime)
target_include_directories(little_ruler PUBLIC ../Dalbaragi/runtime)
