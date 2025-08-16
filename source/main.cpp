#include <Windows.h>
#include <cstdio>

#include "Dumper.h"
#include "Overlay.h"

#include "TlHelp32.h"

[[maybe_unused]] BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID *reserved) {
    (void) instance;
    (void) reserved;

    CreateToolhelp32Snapshot();

    if (reason == DLL_PROCESS_ATTACH) {
        AllocConsole();
        AttachConsole(ATTACH_PARENT_PROCESS);
        SetConsoleOutputCP(CP_UTF8);
        freopen("CON", "w", stdout);

        Overlay::Attach();
        Dumper::Attach();
    }

    if (reason == DLL_PROCESS_DETACH) {
        FreeConsole();

        Dumper::Detach();
    }

    return TRUE;
}
