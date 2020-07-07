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
endif()
target_link_libraries(dalbaragi_runtime PRIVATE freetype)
target_include_directories(dalbaragi_runtime PRIVATE ${extern_dir}/freetype2/include)

# Lua
if (NOT TARGET lua_static)
    add_subdirectory(${externtool_dir}/lua ${CMAKE_CURRENT_BINARY_DIR}/lua)
    target_link_libraries(dalbaragi_runtime PRIVATE lua_static)
endif()

# zlib
if (WIN32)
    add_subdirectory(${extern_dir}/zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib)
    target_link_libraries(dalbaragi_runtime PRIVATE zlibstatic)
    target_include_directories(dalbaragi_runtime PRIVATE ${extern_dir}/zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib)
else()
    target_link_libraries(dalbaragi_runtime PRIVATE z)
endif()


if (WIN32)
    # GLFW
    if (NOT TARGET glfw)
        add_subdirectory(${extern_dir}/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)
    endif()
    target_link_libraries(dalbaragi_runtime PRIVATE glfw)

    # GLaD
    if (NOT TARGET glad)
        add_subdirectory(${extern_dir}/glad ${CMAKE_CURRENT_BINARY_DIR}/glad)
    endif()
    target_link_libraries(dalbaragi_runtime PRIVATE glad)
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

# Dalbaragi overlay
if (NOT TARGET dalbaragi_overlay)
    add_subdirectory(../overlay ${CMAKE_CURRENT_BINARY_DIR}/dalbaragi_overlay)
endif()
target_link_libraries(dalbaragi_runtime PUBLIC dalbaragi_overlay)
