cmake_minimum_required(VERSION 3.19)
project(decima_native C)

set(CMAKE_C_STANDARD 11)

add_library(decima_native SHARED src/main.c src/memory.c src/rtti.c src/hashmap.c src/exports.c src/json.c)
set_property(TARGET decima_native PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_definitions(-D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN)
target_include_directories(decima_native PRIVATE include)

add_custom_command(TARGET decima_native POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:decima_native>
        "\"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Horizon Zero Dawn\\winhttp.dll\""
)