cmake_minimum_required(VERSION 3.4.1)

project(Dalbaragi-Overlay
    LANGUAGES CXX
)


add_library(dalbaragi_overlay
    d_overlay_base.h    d_overlay_base.cpp
    d_widget.h          d_widget.cpp
    d_text_overlay.h    d_text_overlay.cpp
    d_widget_view.h     d_widget_view.cpp
    d_glyph.h           d_glyph.cpp
    d_w_text_view.h     d_w_text_view.cpp
    d_widget_manager.h  d_widget_manager.cpp
)

target_compile_features(dalbaragi_overlay PUBLIC cxx_std_17)

target_include_directories(dalbaragi_overlay
    PUBLIC
        .
        ${extern_dir}/freetype2/include
        ${extern_dir}/glm
)

target_link_libraries(dalbaragi_overlay
    PRIVATE
        fmt::fmt
        freetype
    PUBLIC
        dalbaragi_lightweight
)
