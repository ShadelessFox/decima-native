#pragma once

#include "Util/Assert.h"

class NxLog {
public:
    static NxLog *Instance() {
        return *Offsets::ResolveID<"NxLogImpl::Instance", NxLog **>();
    }

    // @formatter:off
    virtual ~NxLog() = 0;
    virtual void Startup() = 0;
    virtual void OpenLog() = 0;
    virtual void AllocConsole() = 0;
    virtual void CloseConsole() = 0;
    virtual void Unk05() = 0;
    virtual void Unk06() = 0;
    virtual void PrintA(const char *) = 0;
    virtual void PrintLnA(const char *) = 0;
    virtual void LogA(const char *category, const char *fmt, ...) = 0;
    virtual void LogW(const wchar_t *category, const wchar_t *fmt, ...) = 0;
    virtual void LogMemoryStatistics() = 0;
    virtual void Unk12() = 0;
    virtual void Unk13() = 0;
    // @formatter:on
};

class NxLogImpl : public NxLog {
public:
    bool Initialized;
    FILE *FileHandle;
    int Unk18;
    int Unk1C;
    wchar_t Unk20[0x100000 / sizeof(wchar_t)];
    wchar_t Unk10020[0x100 / sizeof(wchar_t)];
    wchar_t Unk10120[0x100 / sizeof(wchar_t)];
    HANDLE ConsoleHandle;
    CRITICAL_SECTION Lock;
};

assert_size(NxLogImpl, 0x100250);
assert_offset(NxLogImpl, Initialized, 0x8);
assert_offset(NxLogImpl, FileHandle, 0x10);
