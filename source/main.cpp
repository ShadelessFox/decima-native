#include <Windows.h>
#include <cstdio>

#include "Dumper.h"

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID *reserved) {
    (void) instance;
    (void) reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        AllocConsole();
        AttachConsole(ATTACH_PARENT_PROCESS);
        freopen("CON", "w", stdout);

        Dumper::Attach();
    }

    if (reason == DLL_PROCESS_DETACH) {
        Dumper::Detach();
    }

    return TRUE;
}
