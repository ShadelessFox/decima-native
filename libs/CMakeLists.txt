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
