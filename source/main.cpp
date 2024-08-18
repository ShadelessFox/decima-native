#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <detours.h>

#include "Offsets.h"

#include "PCore/Array.h"
#include "PCore/Ref.h"
#include "PCore/String.h"
#include "Core/RTTIRefObject.h"

class Listener {
public:
    [[maybe_unused]] virtual void OnCoreLoad(const String &inPath, Array<Ref<RTTIRefObject>> &inObjects) = 0;

    [[maybe_unused]] virtual void OnCoreUnload(const String &inPath, Array<Ref<RTTIRefObject>> &inObjects) = 0;
};

class LoggingListener : public Listener {
public:
    void OnCoreLoad(const String &inPath, Array<Ref<RTTIRefObject>> &inObjects) override {
        printf("Loaded file '%s' (%zu object%s)\n", inPath.c_str(), inObjects.size(), inObjects.size() == 1 ? "" : "s");

        for (auto &object: inObjects) {
            // Fix menu buttons not fitting on screen
            // interface/menu/ds/title/ui_title_menu_view.core
            if (object->mObjectUUID == "27235008-8628-8547-8e56-4f0928fe20b8") {
                object->Get<int>("Y") -= 50;
                object->Get<int>("Height") += 50;
            }

            // Skip intro logos
            // interface/menu/ds/splash_screen/ui_splash_screen_menu_view.core
            if (object->mObjectUUID == "49954e51-af5f-4e42-a909-ee290c6d96ad") {
                object->Set("Trigger", "FocusReceived");
                object->Get<Ref<RTTIRefObject>>("OnAnimationStart") = object->Get<Ref<RTTIRefObject>>("OnAnimationEnd");
                object->Get<Ref<RTTIRefObject>>("OnAnimationStart")->Set("FunctionName", "OnFinishDecimaLogo");
            }
        }
    }

    void OnCoreUnload(const String &inPath, Array<Ref<RTTIRefObject>> &inObjects) override {
        printf("Unloaded file '%s' (%zu object%s)\n", inPath.c_str(), inObjects.size(), inObjects.size() == 1 ? "" : "s");
    }
};

static void (*CoreFileManager_RegisterEventListener)(void *, void *);

static void *(*CoreFileManager_Constructor)(void *, void *);

static void *CoreFileManager_Constructor_Hook(void *inThis, void *inSStreamingManager) {
    static LoggingListener listener;

    CoreFileManager_Constructor(inThis, inSStreamingManager);
    CoreFileManager_RegisterEventListener(inThis, &listener);

    return inThis;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID *reserved) {
    (void) instance;
    (void) reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        AllocConsole();
        AttachConsole(ATTACH_PARENT_PROCESS);
        freopen("CON", "w", stdout);

        // @formatter:off
        Offsets::MapSignature("CoreFileManager::Constructor", "40 53 48 83 EC 20 48 8D 05 ? ? ? ? 48 89 51 08 48 89 01 48 8B D9 48 83 C1 10 FF 15 ? ? ? ? 48 8D 4B 18 FF 15 ? ? ? ? 33 C0");
        Offsets::MapSignature("CoreFileManager::RegisterEventListener", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8D 59 18 48 8B F9 48 8B CB 48 8B F2 FF 15 ? ? ? ? 84 C0 75 09 48 8B CB FF 15 ? ? ? ? 48 8D 4F 28 48 8B D6 E8");
        Offsets::MapSignature("RTTIRefObject::DecrementRef", "40 53 48 83 EC 20 48 8B D9 B8 ? ? ? ? F0 0F C1 41 ? 25 ? ? ? ? 83 F8 01 75 34 8B 41");
        Offsets::MapSignature("String::FromCString", "40 53 48 83 EC 20 48 8B D9 48 C7 01 00 00 00 00 49 C7 C0 FF FF FF FF");
        Offsets::MapSignature("String::FromString", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B 39 48 8B F2 48 8B D9 48 3B 3A 74 54 48 89 6C 24");
        Offsets::MapSignature("String::~String", "40 53 48 83 EC 20 48 8B 19 48 8D 05 ? ? ? ? 48 83 EB 10 48 3B D8 74 27 B8 ? ? ? ? F0 0F C1 03 0F BA F0 1F 83 F8 01 75 15 48 8B");
        Offsets::MapSignature("RTTI::GetName", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 70 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 0F B6 41 04 48 8B FA 48 8B F1 83 F8");
        Offsets::MapSignature("RTTI::ToString", "4C 8B DC 57 41 54 41 55 48 83 EC 70 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 4C 8B E9 4D 8B E0 0F B6");

        CoreFileManager_Constructor = (void *(*)(void *, void *)) Offsets::ResolveID<"CoreFileManager::Constructor">();
        CoreFileManager_RegisterEventListener = (void (*)(void *, void *)) Offsets::ResolveID<"CoreFileManager::RegisterEventListener">();
        // @formatter:on

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