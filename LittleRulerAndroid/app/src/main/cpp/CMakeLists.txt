cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fno-rtti -Wall")

if (${ANDROID_PLATFORM_LEVEL} LESS 18)
    message(FATAL_ERROR "OpenGL 3 is not supported before API level 18 (currently using ${ANDROID_PLATFORM_LEVEL}).")
    return()
endif (${ANDROID_PLATFORM_LEVEL} LESS 18)


# Set global konstants
set(library_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Library)

# Make main JNI library
file(GLOB dalbaragiSrcFiles ./dalbaragi/*.cpp)
add_library( interface4jni SHARED interface4jni.cpp javautil.cpp
        dalbaragi/c_input_apply.cpp
        dalbaragi/g_actor.cpp
        dalbaragi/g_charastate.cpp
        dalbaragi/m_collider.cpp
        dalbaragi/o_overlay_master.cpp
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
        dalbaragi/s_event.cpp
        dalbaragi/s_input_queue.cpp
        dalbaragi/s_logger_god.cpp
        dalbaragi/s_threader.cpp
        dalbaragi/u_dlbparser.cpp
        dalbaragi/u_loadinfo.cpp
        dalbaragi/u_luascript.cpp
        dalbaragi/u_objparser.cpp
        dalbaragi/u_timer.cpp
        dalbaragi/u_math.cpp
        dalbaragi/x_init_windows.cpp
        dalbaragi/x_mainloop.cpp
        dalbaragi/p_model.cpp
        dalbaragi/p_skybox.cpp
        dalbaragi/o_widgetcache.cpp
        dalbaragi/u_byteutils.cpp
        dalbaragi/u_filesystem.cpp
        dalbaragi/u_fileutils.cpp
)
target_include_directories(interface4jni PRIVATE ./dalbaragi)


######## Import libraries ########


# Lodepng lib
add_library(lib_lodepng STATIC IMPORTED)
set_target_properties(lib_lodepng PROPERTIES IMPORTED_LOCATION
        ${library_dir}/lodepng/a/${ANDROID_ABI}/liblodepng.a)
target_include_directories(interface4jni PRIVATE ${library_dir}/lodepng/include)


# Lua
add_library(lib_lua STATIC IMPORTED)
set_target_properties(lib_lua PROPERTIES IMPORTED_LOCATION
        ${library_dir}/lua-5.3.5/a/${ANDROID_ABI}/liblua.a)
target_include_directories(interface4jni PRIVATE ${library_dir}/lua-5.3.5/include)


# Freetype
add_library(lib_freetype SHARED IMPORTED)
set_target_properties(lib_freetype PROPERTIES IMPORTED_LOCATION
        ${library_dir}/freetype-2.10.0/so/${ANDROID_ABI}/libfreetype.so)
target_include_directories(interface4jni PRIVATE ${library_dir}/freetype-2.10.0/include)


# fmt
add_library(lib_fmt SHARED IMPORTED)
set_target_properties(lib_fmt PROPERTIES IMPORTED_LOCATION
        ${library_dir}/fmt-5.3.0/so/${ANDROID_ABI}/libfmt.so)
target_include_directories(interface4jni PRIVATE ${library_dir}/fmt-5.3.0/include)


# GLM
target_include_directories(interface4jni PRIVATE ${library_dir}/glm-0.9.8.4/include)


# TGA
target_include_directories(interface4jni PRIVATE ${library_dir}/tga/include)


# entt
target_include_directories(interface4jni PRIVATE ${library_dir}/entt/include)


######## Link ########

# Include libraries needed for gles3jni lib
target_link_libraries(
        interface4jni
        GLESv3
        android
        EGL
        log
        z  # This is zlib. What a name!
        lib_lodepng
        lib_lua
        lib_freetype
        lib_fmt
)