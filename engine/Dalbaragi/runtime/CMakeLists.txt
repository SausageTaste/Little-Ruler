cmake_minimum_required(VERSION 3.4.1)

project(Dalbaragi-Runtime
    LANGUAGES CXX
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
target_include_directories(dalbaragi_runtime PUBLIC .)
target_compile_features(dalbaragi_runtime PUBLIC cxx_std_17)


######## Import libraries ########

set(repoRoot_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(extern_dir ${repoRoot_dir}/extern)
set(externcpy_dir ${repoRoot_dir}/externcpy)
set(externtool_dir ${repoRoot_dir}/externtool)


# Freetype
if (NOT TARGET freetype)
    add_subdirectory(${extern_dir}/freetype2 ${CMAKE_CURRENT_BINARY_DIR}/freetype2)
    target_link_libraries(dalbaragi_runtime PRIVATE freetype)
endif()

# Lua
if (NOT TARGET lua_static)
    add_subdirectory(${externtool_dir}/lua ${CMAKE_CURRENT_BINARY_DIR}/lua)
    target_link_libraries(dalbaragi_runtime PRIVATE lua_static)
endif()

# zlib
if (NOT TARGET zlibstatic)
    add_subdirectory(${extern_dir}/zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib)
    target_link_libraries(dalbaragi_runtime PRIVATE zlibstatic)
    target_include_directories(dalbaragi_runtime PRIVATE ${extern_dir}/zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib)
endif()


if (WIN32)
    # GLFW
    if (NOT TARGET glfw)
        add_subdirectory(${externcpy_dir}/glfw-3.3.2 ${CMAKE_CURRENT_BINARY_DIR}/glfw)
    endif()
    target_link_libraries(dalbaragi_runtime PUBLIC glfw)
    target_include_directories(dalbaragi_runtime PRIVATE ${externcpy_dir}/glfw-3.3.2/include)

    # GLaD
    add_subdirectory(${externcpy_dir}/glad-0.1.33 ${CMAKE_CURRENT_BINARY_DIR}/glad)
    target_link_libraries(dalbaragi_runtime PUBLIC glad)
endif (WIN32)


# Dalbaragi util
if (NOT TARGET dalbaragi_util)
    add_subdirectory(../util ${CMAKE_CURRENT_BINARY_DIR}/dalbaragi_util)
    target_link_libraries(dalbaragi_runtime PUBLIC dalbaragi_util)
endif()

# Dalbaragi physics
if (NOT TARGET dalbaragi_physics)
    add_subdirectory(../physics ${CMAKE_CURRENT_BINARY_DIR}/dalbaragi_physics)
    target_link_libraries(dalbaragi_runtime PUBLIC dalbaragi_physics)
endif()

# Dalbaragi resparser
if (NOT TARGET dalbaragi_resparser)
    add_subdirectory(../resparser ${CMAKE_CURRENT_BINARY_DIR}/dalbaragi_resparser)
    target_link_libraries(dalbaragi_runtime PUBLIC dalbaragi_resparser)
endif()
