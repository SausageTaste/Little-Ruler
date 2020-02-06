cmake_minimum_required(VERSION 3.4.1)


project(LuaCMake
    LANGUAGES C
)


set(extern_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../extern)
set(lua_dir ${extern_dir}/lua)

file(GLOB luaHeaders ${lua_dir}/*.h)
file(GLOB luaSources ${lua_dir}/*.c)

add_library(lua_static STATIC ${luaSources})
target_include_directories(lua_static PUBLIC ${lua_dir})
