cmake_minimum_required(VERSION 3.16)
project(decima_native)

set(CMAKE_C_STANDARD 11)

add_library(decima_native SHARED
        libs/detours/src/disolia64.cpp
        libs/detours/src/disolx64.cpp
        libs/detours/src/detours.cpp
        libs/detours/src/disolx86.cpp
        libs/detours/src/disolarm.cpp
        libs/detours/src/creatwth.cpp
        libs/detours/src/disolarm64.cpp
        libs/detours/src/image.cpp
        libs/detours/src/disasm.cpp
        libs/detours/src/modules.cpp

        libs/hashmap/hashmap.c

        src/rtti.c
        src/exports.c
        src/json.c
        src/main.c
)


set_property(TARGET decima_native PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_definitions(-D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN)
target_include_directories(decima_native PRIVATE include libs/detours/src libs/hashmap)

add_custom_command(TARGET decima_native POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:decima_native>
        "\"E:\\SteamLibrary\\steamapps\\common\\Horizon Forbidden West Complete Edition\\winhttp.dll\""
)