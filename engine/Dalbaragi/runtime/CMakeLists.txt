cmake_minimum_required(VERSION 3.4.1)

project(Dalbaragi-Runtime
    LANGUAGES CXX
)


add_library(dalbaragi_runtime
    c_input_apply.h         c_input_apply.cpp
    d_context.h
    d_context_main.h        d_context_main.cpp
    g_actor.h               g_actor.cpp
    g_charastate.h          g_charastate.cpp
    p_animation.h           p_animation.cpp
    p_dalopengl.h           p_dalopengl.cpp
    p_globalfsm.h
    p_light.h               p_light.cpp
    p_meshStatic.h          p_meshStatic.cpp
    p_model.h               p_model.cpp
    p_render_master.h       p_render_master.cpp
    p_resource.h            p_resource.cpp
    p_scene.h               p_scene.cpp
    p_shader_master.h       p_shader_master.cpp
    p_uniloc.h              p_uniloc.cpp
    p_water.h               p_water.cpp
    s_input_queue.h         s_input_queue.cpp
    s_threader.h            s_threader.cpp
    u_loadinfo.h            u_loadinfo.cpp
    u_luascript.h           u_luascript.cpp
    u_objparser.h           u_objparser.cpp
    x_init_windows.h        x_init_windows.cpp
    x_mainloop.h            x_mainloop.cpp
    d_global_macro.h
    d_overlay_interface.h   d_overlay_interface.cpp
    d_camera.h              d_camera.cpp
)

target_compile_features(dalbaragi_runtime PUBLIC cxx_std_17)

target_include_directories(dalbaragi_runtime
    PUBLIC
        .
        ${extern_dir}/freetype2/include
)

target_link_libraries(dalbaragi_runtime
    PRIVATE
        fmt::fmt
        EnTT::EnTT
        freetype
        lua_static
        dalbaragi::daltools
        dalbaragi_util
        dalbaragi_physics
        dalbaragi_resparser
        dalbaragi_overlay
)

if (WIN32)
    target_link_libraries(dalbaragi_runtime
        PRIVATE
            glfw
            glad
    )
endif()
