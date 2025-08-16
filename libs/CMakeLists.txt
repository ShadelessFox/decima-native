add_library(pattern16 INTERFACE)
target_include_directories(pattern16 INTERFACE pattern16/include)

add_library(detours STATIC
        detours/src/disolia64.cpp
        detours/src/disolx64.cpp
        detours/src/detours.cpp
        detours/src/disolx86.cpp
        detours/src/disolarm.cpp
        detours/src/creatwth.cpp
        detours/src/disolarm64.cpp
        detours/src/image.cpp
        detours/src/disasm.cpp
        detours/src/modules.cpp
)
target_include_directories(detours PUBLIC detours/src)

add_library(imgui STATIC
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_win32.cpp
        imgui/backends/imgui_impl_dx12.cpp
)
target_include_directories(imgui PUBLIC imgui imgui/backends imgui_club/imgui_memory_editor)
set_target_properties(imgui PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(nixxes INTERFACE)
target_include_directories(nixxes INTERFACE nixxes)
