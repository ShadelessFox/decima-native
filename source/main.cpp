#include <Windows.h>
#include <cstdio>
#include <array>
#include <filesystem>

#include "Dumper.h"

static BOOL WINAPI CtrlHandler([[maybe_unused]] DWORD fdwCtrlType) {
    Dumper::Dump();
    return TRUE;
}

[[maybe_unused]] BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID *reserved) {
    (void) instance;
    (void) reserved;

    CHAR filename[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleW(nullptr), filename, sizeof(filename));

    if (auto length = strlen(filename); length < 7 || strcmp(filename + length - 7, "DS2.exe") != 0)
        return TRUE;

    if (reason == DLL_PROCESS_ATTACH) {
        AllocConsole();
        AttachConsole(ATTACH_PARENT_PROCESS);
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCtrlHandler(CtrlHandler, TRUE);
        freopen("CON", "w", stdout);

        Dumper::Attach();
    }

    if (reason == DLL_PROCESS_DETACH) {
        SetConsoleCtrlHandler(CtrlHandler, FALSE);
        FreeConsole();

        Dumper::Detach();
    }

    return TRUE;
}
