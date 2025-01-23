#include <Windows.h>
#include <cstdio>

#include "Dumper.h"
#include "Overlay.h"

[[maybe_unused]] BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID *reserved) {
    (void) instance;
    (void) reserved;

    if (reason == DLL_PROCESS_ATTACH) {
#if 0
        switch (MessageBoxA(nullptr, "Do you want to attach the injector?", "Choose action", MB_YESNOCANCEL | MB_ICONQUESTION)) {
            case IDYES:
                break;
            case IDNO:
                return TRUE;
            case IDCANCEL:
                ExitProcess(0);
        }
#endif

        AllocConsole();
        AttachConsole(ATTACH_PARENT_PROCESS);
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
