cmake_minimum_required(VERSION 3.4.1)

if (${ANDROID_PLATFORM_LEVEL} LESS 18)
    message(FATAL_ERROR "OpenGL 3 is not supported before API level 18 (currently using ${ANDROID_PLATFORM_LEVEL}).")
    return()
endif (${ANDROID_PLATFORM_LEVEL} LESS 18)


######## Set global constants ########

set(root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../../..)
set(extern_dir ${root_dir}/extern)
set(externtool_dir ${root_dir}/externtool)


######## Make main JNI library ########

add_library( interface4jni SHARED
        interface4jni.cpp
        javautil.cpp
)
target_include_directories(interface4jni PRIVATE .)
target_compile_features(interface4jni PUBLIC cxx_std_17)


######## Import libraries ########

add_subdirectory(${root_dir}/engine/Dalbaragi/runtime ${CMAKE_CURRENT_BINARY_DIR}/dalbaragi_runtime)
target_link_libraries(interface4jni dalbaragi_runtime)


######## Link ########

# Include libraries needed for gles3jni lib
target_link_libraries(interface4jni
        GLESv3
        android
        EGL
        log
        #z  # This is zlib. What a name!
)
