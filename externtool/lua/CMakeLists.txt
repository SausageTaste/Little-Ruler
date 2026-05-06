cmake_minimum_required(VERSION 3.10)


project(LuaCMake
    LANGUAGES C
)


set(extern_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../extern)
set(lua_dir ${extern_dir}/lua)

file(GLOB luaHeaders ${lua_dir}/*.h)
file(GLOB luaSources ${lua_dir}/*.c)
list(REMOVE_ITEM luaSources
    ${lua_dir}/lua.c
    ${lua_dir}/onelua.c
    ${lua_dir}/ltests.c
)

add_library(lua_static STATIC ${luaSources})
target_include_directories(lua_static PUBLIC ${lua_dir})
