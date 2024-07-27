cmake_minimum_required(VERSION 3.4.1)

project(Little-Ruler
    VERSION        0.1.0
    DESCRIPTION    ""
    LANGUAGES      CXX
)


add_executable(little_ruler
    main.cpp
)

target_compile_features(little_ruler PUBLIC cxx_std_17)

target_include_directories(little_ruler PUBLIC .)

target_link_libraries(little_ruler
    PRIVATE
        dalbaragi_runtime
)
