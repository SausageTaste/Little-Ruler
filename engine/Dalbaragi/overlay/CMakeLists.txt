cmake_minimum_required(VERSION 3.4.1)

project(Dalbaragi-Overlay
    LANGUAGES CXX
)


add_library(dalbaragi_overlay
    d_overlay_base.h  d_overlay_base.cpp
    d_widget.h        d_widget.cpp
    d_text_overlay.h  d_text_overlay.cpp
)
target_include_directories(dalbaragi_overlay PUBLIC .)
target_compile_features(dalbaragi_overlay PUBLIC cxx_std_17)


######## Import libraries ########

set(repoRoot_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(extern_dir ${repoRoot_dir}/extern)

# fmt
if (NOT TARGET fmt::fmt)
    add_subdirectory(${extern_dir}/fmt ${CMAKE_CURRENT_BINARY_DIR}/fmt)
endif()
target_link_libraries(dalbaragi_overlay PUBLIC fmt::fmt)

# Freetype
if (NOT TARGET freetype)
    add_subdirectory(${extern_dir}/freetype2 ${CMAKE_CURRENT_BINARY_DIR}/freetype2)
endif()
target_link_libraries(dalbaragi_overlay PRIVATE freetype)
target_include_directories(dalbaragi_overlay PRIVATE ${extern_dir}/freetype2/include)

# GLM
target_include_directories(dalbaragi_overlay PUBLIC ${extern_dir}/glm)


# Dalbaragi Lightweight
if (NOT TARGET dalbaragi_lightweight)
    add_subdirectory(../lightweight ${CMAKE_CURRENT_BINARY_DIR}/dalbaragi_lightweight)
endif()
target_link_libraries(dalbaragi_overlay PUBLIC dalbaragi_lightweight)
