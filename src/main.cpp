#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <detours.h>
#include <cstdio>

#include "scan.hpp"
#include "rtti.hpp"

template<typename T>
class Array {
public:
    const T &operator[](size_t index) const {
        return m_Entries[index];
    }

    [[nodiscard]] std::size_t size() const { return m_Count; }

private:
    uint32_t m_Count;
    uint32_t m_Capacity;
    T *m_Entries;
};

template<typename T>
class Ref {
public:
    [[nodiscard]] T *get() const {
        return m_Ptr;
    }

    explicit operator bool() const {
        return get() != nullptr;
    }

    T *operator->() const {
        return get();
    }

    const T &operator*() const {
        return *get();
    }

    T &operator*() {
        return *get();
    }

private:
    T *m_Ptr;
};

class String {
public:
    [[nodiscard]] const char *c_str() const { return mData; }

private:
    const char *mData;
};

class RTTI;

class RTTIObject {
public:
    virtual const RTTI *GetRTTI() const = 0;

    virtual ~RTTIObject() = 0;
};

struct GGUUID {
    uint8_t mData[16];
};

class RTTIRefObject : public RTTIObject {
public:
    GGUUID mObjectUUID;
    uint32_t mRefCount;
};

class LocalizedTextResource : public RTTIRefObject /* Resource */ {
public:
    const char *mText{nullptr};
    uint16_t mLength{0};
};

class Listener {
public:
    virtual void OnCoreLoad(const String &inPath, const Array<Ref<RTTIRefObject>> &inObjects) = 0;

    virtual void OnCoreUnload(const String &inPath, const Array<Ref<RTTIRefObject>> &inObjects) = 0;
};

class LoggingListener : public Listener {
public:
    void OnCoreLoad(const String &inPath, const Array<Ref<RTTIRefObject>> &inObjects) override {
        printf("Loaded file '%s' (%zu objects)\n", inPath.c_str(), inObjects.size());

        if (true) {
            return;
        }

        for (size_t i = 0; i < inObjects.size(); i++) {
            auto &object = const_cast<Ref<RTTIRefObject> &>(inObjects[i]);
            auto &type = reinterpret_cast<const RTTIClass &>(*object->GetRTTI());

            if (type.GetName() == "LocalizedTextResource") {
                auto &resource = reinterpret_cast<LocalizedTextResource &>(*object);
                resource.mText = "Amogus";
                resource.mLength = 6;
            }
        }
    }

    void OnCoreUnload(const String &inPath, const Array<Ref<RTTIRefObject>> &inObjects) override {
        printf("Unloaded file '%s' (%zu objects)\n", inPath.c_str(), inObjects.size());
    }
};

extern "C" {

static void (*CoreFileManager_RegisterEventListener)(void *, void *);

static void *(*CoreFileManager_Constructor)(void *, void *);

static void *CoreFileManager_Constructor_Hook(void *inThis, void *inSStreamingManager) {
    static LoggingListener listener;

    CoreFileManager_Constructor(inThis, inSStreamingManager);
    CoreFileManager_RegisterEventListener(inThis, &listener);

    return inThis;
}

}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID *reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        AllocConsole();
        AttachConsole(ATTACH_PARENT_PROCESS);
        freopen("CON", "w", stdout);

        struct Section section{};
        if (!FindSection(GetModuleHandleA(NULL), ".text", &section)) {
            perror("Unable to find '.text' section in the executable");
            return FALSE;
        }

        if (!FindPattern(section.start, section.end,
                         "40 53 48 83 EC 20 48 8D 05 ? ? ? ? 48 89 51 08 48 89 01 48 8B D9 48 83 C1 10 FF 15 ? ? ? ? 48 8D 4B 18 FF 15 ? ? ? ? 33 C0",
                         (void **) &CoreFileManager_Constructor)) {
            perror("Unable to find 'CoreFileManager::Constructor' function in the executable");
            return FALSE;
        }

        if (!FindPattern(section.start, section.end,
                         "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8D 59 18 48 8B F9 48 8B CB 48 8B F2 FF 15 ? ? ? ? 84 C0 75 09 48 8B CB FF 15 ? ? ? ? 48 8D 4F 28 48 8B D6 E8",
                         (void **) &CoreFileManager_RegisterEventListener)) {
            perror("Unable to find 'CoreFileManager::RegisterEventListener' function in the executable");
            return FALSE;
        }

        printf("Found CoreFileManager::Constructor at %p\n", CoreFileManager_Constructor);
        printf("Found CoreFileManager::RegisterEventListener at %p\n", CoreFileManager_RegisterEventListener);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((PVOID *) &CoreFileManager_Constructor, (PVOID) CoreFileManager_Constructor_Hook);
        DetourTransactionCommit();
    }

    if (reason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach((PVOID *) &CoreFileManager_Constructor, (PVOID) CoreFileManager_Constructor_Hook);
        DetourTransactionCommit();
    }

    return TRUE;
}