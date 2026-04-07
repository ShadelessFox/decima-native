cmake_minimum_required(VERSION 3.20)
project(decima_native)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(GAME_PATH "E:\\SteamLibrary\\steamapps\\common\\DEATH STRANDING 2 - ON THE BEACH")

add_definitions(-D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN -DWIN32_EXTRA_LEAN -DRTTI_STANDALONE)
add_subdirectory(libs)

add_library(decima_native SHARED
        source/Decima/Core/GGUUID.cpp
        source/Decima/Core/RTTI.cpp
        source/Exporter/Exporter.cpp
        source/Exporter/AttrExporter.cpp
        source/Exporter/IdaExporter.cpp
        source/Exporter/JsonExporter.cpp
        source/Util/json.cpp
        source/Util/Offsets.cpp
        source/Util/XUtil.cpp
        source/Dumper.cpp
        source/Overlay.cpp
        source/exports.c
        source/main.cpp
)

target_include_directories(decima_native PRIVATE source)
target_link_libraries(decima_native PRIVATE pattern16 detours imgui nixxes dxguid)
set_target_properties(decima_native PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_custom_command(TARGET decima_native POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:decima_native>
        "\"${GAME_PATH}\\winhttp.dll\""
)
