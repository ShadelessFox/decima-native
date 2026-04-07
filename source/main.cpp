#include <Windows.h>
#include <cstdio>

#include "Dumper.h"
#include "Overlay.h"

static BOOL WINAPI CtrlHandler([[maybe_unused]] DWORD fdwCtrlType) {
    Dumper::Dump();
    return TRUE;
}

[[maybe_unused]] BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID *reserved) {
    (void) instance;
    (void) reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        AllocConsole();
        AttachConsole(ATTACH_PARENT_PROCESS);
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCtrlHandler(CtrlHandler, TRUE);
        freopen("CON", "w", stdout);

        // Overlay::Attach();
        Dumper::Attach();
    }

    if (reason == DLL_PROCESS_DETACH) {
        SetConsoleCtrlHandler(CtrlHandler, FALSE);
        FreeConsole();

        Dumper::Detach();
    }

    return TRUE;
}
