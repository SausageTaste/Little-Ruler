cmake_minimum_required(VERSION 3.4.1)

if (${ANDROID_PLATFORM_LEVEL} LESS 18)
    message(FATAL_ERROR "OpenGL 3 is not supported before API level 18 (currently using ${ANDROID_PLATFORM_LEVEL}).")
    return()
endif (${ANDROID_PLATFORM_LEVEL} LESS 18)


######## Set global constants ########

set(extern_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../extern)


######## Make main JNI library ########

add_library( interface4jni SHARED
        interface4jni.cpp
        javautil.cpp

        dalbaragi/utils/d_logger.cpp
        dalbaragi/utils/d_filesystem.cpp
        dalbaragi/utils/u_timer.cpp
        dalbaragi/utils/u_byteutils.cpp
        dalbaragi/utils/u_fileutils.cpp
        dalbaragi/utils/u_imagebuf.cpp
        dalbaragi/utils/u_math.cpp
        dalbaragi/utils/d_shaderProcessor.cpp

        dalbaragi/c_input_apply.cpp
        dalbaragi/g_actor.cpp
        dalbaragi/g_charastate.cpp
        dalbaragi/m_collider.cpp
        dalbaragi/o_widget_textbox.cpp
        dalbaragi/o_widget_texview.cpp
        dalbaragi/o_widgetbase.cpp
        dalbaragi/o_widgetmanager.cpp
        dalbaragi/p_animation.cpp
        dalbaragi/p_dalopengl.cpp
        dalbaragi/p_light.cpp
        dalbaragi/p_meshStatic.cpp
        dalbaragi/p_render_master.cpp
        dalbaragi/p_resource.cpp
        dalbaragi/p_scene.cpp
        dalbaragi/p_shader_master.cpp
        dalbaragi/p_uniloc.cpp
        dalbaragi/p_water.cpp
        dalbaragi/s_configs.cpp
        dalbaragi/s_input_queue.cpp
        dalbaragi/s_threader.cpp
        dalbaragi/u_dlbparser.cpp
        dalbaragi/u_loadinfo.cpp
        dalbaragi/u_luascript.cpp
        dalbaragi/u_objparser.cpp
        dalbaragi/x_init_windows.cpp
        dalbaragi/x_mainloop.cpp
        dalbaragi/p_model.cpp
        dalbaragi/o_widgetcache.cpp
        dalbaragi/d_context_main.cpp
        dalbaragi/d_widgettext.cpp
)
target_include_directories(interface4jni PRIVATE ./dalbaragi)
target_include_directories(interface4jni PRIVATE ./dalbaragi/utils)

target_compile_features(interface4jni PUBLIC cxx_std_17)


######## Import libraries ########

# Lodepng
add_library(lib_lodepng STATIC
        ${extern_dir}/lodepng/lodepng.h
        ${extern_dir}/lodepng/lodepng.cpp
)
target_include_directories(lib_lodepng PUBLIC ${extern_dir}/lodepng)
target_link_libraries(interface4jni lib_lodepng)

# Lua
file(GLOB luaHeaders ${extern_dir}/lua/*.h)
file(GLOB luaSources ${extern_dir}/lua/*.c)
add_library(lib_lua STATIC
        ${luaHeaders}
        ${luaSources}
)
target_include_directories(lib_lua PUBLIC ${extern_dir}/lua)
target_link_libraries(interface4jni lib_lua)

# Freetype
add_subdirectory(${extern_dir}/freetype2 ${CMAKE_CURRENT_BINARY_DIR}/freetype2)
target_link_libraries(interface4jni freetype)

# fmt
add_subdirectory(${extern_dir}/fmt ${CMAKE_CURRENT_BINARY_DIR}/fmt)
target_link_libraries(interface4jni fmt::fmt)

# entt
add_subdirectory(${extern_dir}/entt ${CMAKE_CURRENT_BINARY_DIR}/entt)
target_link_libraries(interface4jni EnTT::EnTT)

# GLM
target_include_directories(interface4jni PRIVATE ${extern_dir}/glm)

# TGA
target_include_directories(interface4jni PRIVATE ${extern_dir}/TGA)


######## Link ########

# Include libraries needed for gles3jni lib
target_link_libraries(interface4jni
        GLESv3
        android
        EGL
        log
        z  # This is zlib. What a name!
)
