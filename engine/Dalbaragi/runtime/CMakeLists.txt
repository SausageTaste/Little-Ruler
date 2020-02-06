cmake_minimum_required(VERSION 3.4.1)


project(Dalbaragi-Runtime 
    VERSION        0.1.0
    DESCRIPTION    ""
    LANGUAGES      CXX
)


add_library(dalbaragi_runtime
    c_input_apply.cpp
    c_input_apply.h
    d_context.h
    d_context_main.cpp
    d_context_main.h
    d_widgettext.cpp
    d_widgettext.h
    g_actor.cpp
    g_actor.h
    g_charastate.cpp
    g_charastate.h
    m_collider.cpp
    m_collider.h
    o_widgetbase.cpp
    o_widgetbase.h
    o_widgetcache.cpp
    o_widgetcache.h
    o_widgetmanager.cpp
    o_widgetmanager.h
    o_widget_textbox.cpp
    o_widget_textbox.h
    o_widget_texview.cpp
    o_widget_texview.h
    p_animation.cpp
    p_animation.h
    p_dalopengl.cpp
    p_dalopengl.h
    p_globalfsm.h
    p_light.cpp
    p_light.h
    p_meshStatic.cpp
    p_meshStatic.h
    p_model.cpp
    p_model.h
    p_render_master.cpp
    p_render_master.h
    p_resource.cpp
    p_resource.h
    p_scene.cpp
    p_scene.h
    p_shader_master.cpp
    p_shader_master.h
    p_uniloc.cpp
    p_uniloc.h
    p_water.cpp
    p_water.h
    s_configs.cpp
    s_configs.h
    s_input_queue.cpp
    s_input_queue.h
    s_threader.cpp
    s_threader.h
    u_dlbparser.cpp
    u_dlbparser.h
    u_loadinfo.cpp
    u_loadinfo.h
    u_luascript.cpp
    u_luascript.h
    u_objparser.cpp
    u_objparser.h
    x_init_windows.cpp
    x_init_windows.h
    x_mainloop.cpp
    x_mainloop.h
)
target_include_directories(dalbaragi_runtime PRIVATE .)
target_compile_features(dalbaragi_runtime PUBLIC cxx_std_17)


######## Import libraries ########

set(repoRoot_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(extern_dir ${repoRoot_dir}/extern)
set(externcpy_dir ${repoRoot_dir}/externcpy)
set(externtool_dir ${repoRoot_dir}/externtool)

# entt
add_subdirectory(${extern_dir}/entt ${CMAKE_CURRENT_BINARY_DIR}/entt)
target_link_libraries(dalbaragi_runtime PUBLIC EnTT::EnTT)

# glew
add_subdirectory(${externcpy_dir}/glew/build/cmake ${CMAKE_CURRENT_BINARY_DIR}/glew)
target_link_libraries(dalbaragi_runtime PUBLIC glew_s)
target_include_directories(dalbaragi_runtime PUBLIC ${externcpy_dir}/glew/include)

# SDL2
add_subdirectory(${externcpy_dir}/SDL2 ${CMAKE_CURRENT_BINARY_DIR}/SDL2)
target_link_libraries(dalbaragi_runtime PUBLIC SDL2main SDL2-static)
target_include_directories(dalbaragi_runtime PUBLIC ${externcpy_dir}/SDL2/include)

# Freetype
add_subdirectory(${extern_dir}/freetype2 ${CMAKE_CURRENT_BINARY_DIR}/freetype2)
target_link_libraries(dalbaragi_runtime PUBLIC freetype)

# Lua
add_subdirectory(${externtool_dir}/lua ${CMAKE_CURRENT_BINARY_DIR}/lua)
target_link_libraries(dalbaragi_runtime PUBLIC lua_static)


# Dalbaragi utils
add_subdirectory(../util ${CMAKE_CURRENT_BINARY_DIR}/dalbaragi_util)
target_link_libraries(dalbaragi_runtime PUBLIC dalbaragi_util)
target_include_directories(dalbaragi_runtime PUBLIC ../util)
