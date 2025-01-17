#include <Windows.h>
#include <cstdio>

#include "Dumper.h"

[[maybe_unused]] BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID *reserved) {
    (void) instance;
    (void) reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        switch (MessageBoxA(nullptr, "Do you want to attach the injector?", "Choose action", MB_YESNOCANCEL | MB_ICONQUESTION)) {
            case IDYES:
                break;
            case IDNO:
                return TRUE;
            case IDCANCEL:
                ExitProcess(0);
        }

        AllocConsole();
        AttachConsole(ATTACH_PARENT_PROCESS);
        freopen("CON", "w", stdout);

        Dumper::Attach();
    }

    if (reason == DLL_PROCESS_DETACH) {
        FreeConsole();

        Dumper::Detach();
    }

    return TRUE;
}
