#include "Dumper.h"

#include "Decima/Core/FileDevice.h"
#include "Decima/Core/ExportedSymbolGroup.h"
#include "Decima/Core/FactoryManager.h"
#include "Decima/PCore/String.h"
#include "Decima/PCore/Array.h"
#include "Decima/PCore/Ref.h"
#include "Export/IdaExporter.h"
#include "Util/Offsets.h"

#include <Pattern16.h>

#include <Windows.h>
#include <detours.h>
#include <algorithm>
#include <print>
#include <cassert>

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

static bool *(*PackFileDevice_LoadPackfile)(FileDevice *, const String &, int);

static bool PackFileDevice_LoadPackfile_Hook(FileDevice *inDevice, const String &inPath, int inIndex) {
    std::println("Loading {} at index {}", inPath, inIndex);
    return PackFileDevice_LoadPackfile(inDevice, inPath, inIndex);
}

static void *(*PackFileDevice_LoadPackfiles)(FileDevice *inDevice, const String &);

static void PackFileDevice_LoadPackfiles_Hook(FileDevice *inDevice, const String &inSearchPath) {
    PackFileDevice_LoadPackfiles(inDevice, inSearchPath);

    Array<Filename> files;
    FileSystem::Find("source:data/patches/*.bin", &files, nullptr);

    for (const auto &file: files) {
        PackFileDevice_LoadPackfile(inDevice, file, -1);
    }
}

static void (*ExportedSymbols_Collect)(ExportedSymbols *);

static void (ExportedSymbols_Collect_Hook)(ExportedSymbols *inSymbols) {
    ExportedSymbols_Collect(inSymbols);
    Dumper::Dump();
}

static auto TypeComparator = [](const RTTI *inFirst, const RTTI *inSecond) -> bool {
    static constexpr std::array Order{
        RTTIKind::Compound,
        RTTIKind::Enum,
        RTTIKind::EnumFlags,
        RTTIKind::Atom,
        RTTIKind::Pointer,
        RTTIKind::Container,
        RTTIKind::POD
    };
    if (inFirst->mKind != inSecond->mKind)
        return std::ranges::find(Order, inFirst->mKind) < std::ranges::find(Order, inSecond->mKind);
    return inFirst->GetName() < inSecond->GetName();
};

static std::set<const RTTI *> PendingTypes;

static void ScanType(const RTTI &inType) {
    static std::set<const RTTI *> SeenTypes;

    auto name = inType.GetName();
    if (SeenTypes.contains(&inType))
        return;

    SeenTypes.emplace(&inType);
    FactoryManager::Get().Register(inType);

    std::println("[RTTI] Registered '{}' (total: {})", name, SeenTypes.size());

    if (const auto type = inType.AsCompound(); type) {
        for (auto &message: type->MessageHandlers())
            ScanType(*message.mMessage);
        for (auto &base: type->Bases())
            ScanType(*base.mType);
        for (auto &base: type->Bases())
            ScanType(*base.mType);
    }
    if (const auto type = inType.AsAtom())
        ScanType(*type->mBaseType);
    if (const auto type = inType.AsContainer())
        ScanType(*type->mItemType);
    if (const auto type = inType.AsPointer())
        ScanType(*type->mItemType);
}

static void ScanMemoryForTypes() {
    auto [rdataBase, rdataEnd] = Offsets::GetRdataSection();
    auto [dataBase, dataEnd] = Offsets::GetDataSection();
    auto [textBase, textEnd] = Offsets::GetDataSection();

    auto IsValidDataPtr = [&]<typename T>(T *ptr) {
        if (ptr == nullptr)
            return false;
        auto value = reinterpret_cast<uintptr_t>(ptr);
        return value >= textBase && value < textEnd ||
               value >= dataBase && value < dataEnd ||
               value >= rdataBase && value < rdataEnd;
    };

    auto cur = dataBase;
    auto end = dataEnd;

    while (cur + 5 < end) {
        auto type = static_cast<RTTI *>(Pattern16::scan(
            reinterpret_cast<void *>(cur),
            end - cur,
            "FF FF FF FF [00000???]"
        ));

        if (type == nullptr)
            break;

        cur = reinterpret_cast<uintptr_t>(type) + 5;

        if (auto atom = type->AsAtom(); atom) {
            if (atom->mSize == 0 ||
                atom->mAlignment == 0 ||
                atom->mConstructor && !IsValidDataPtr(atom->mConstructor) ||
                atom->mDestructor && !IsValidDataPtr(atom->mDestructor) ||
                !IsValidDataPtr(atom->mTypeName) ||
                !IsValidDataPtr(atom->mBaseType)
            ) {
                continue;
            }
        } else if (auto enum_ = type->AsEnum(); enum_) {
            if (enum_->mSize == 0 ||
                !IsValidDataPtr(enum_->mTypeName) ||
                !IsValidDataPtr(enum_->mValues)
            ) {
                continue;
            }
        } else if (auto container = type->AsContainer(); container) {
            if (!IsValidDataPtr(container->mItemType) ||
                !IsValidDataPtr(container->mContainerType) ||
                !IsValidDataPtr(container->mContainerType->mTypeName)
            ) {
                continue;
            }
        } else if (auto pointer = type->AsPointer(); pointer) {
            if (!IsValidDataPtr(pointer->mItemType) ||
                !IsValidDataPtr(pointer->mPointerType) ||
                !IsValidDataPtr(pointer->mPointerType->mTypeName)
            ) {
                continue;
            }
        } else if (auto compound = type->AsCompound(); compound) {
            if (!IsValidDataPtr(compound->mTypeName) ||
                compound->mNumBases && !IsValidDataPtr(compound->mBases) ||
                compound->mNumAttrs && !IsValidDataPtr(compound->mAttrs) ||
                compound->mNumMessageHandlers && !IsValidDataPtr(compound->mMessageHandlers)
            ) {
                continue;
            }
        } else {
            continue;
        }

        ScanType(*type);
    }
}

void Dumper::Dump() {
    auto &factory = FactoryManager::Get();

    puts("Scanning types for unreferenced members...");
    for (auto type: factory.Types())
        ScanType(*type);

    puts("Scanning memory for unreferenced types...");
    ScanMemoryForTypes();

    for (auto type: PendingTypes)
        ScanType(*type);

    std::vector<const RTTI *> types{factory.Types().begin(), factory.Types().end()};
    std::sort(types.begin(), types.end(), TypeComparator);

    puts("Exporting types...");
    IdaExporter("dump/dsdc").Export(types);

    ExitProcess(0);
}

void Dumper::Attach() {
    // @formatter:off
    Offsets::MapAddress("ExportedSymbols::Instance", Offsets::OffsetFromInstruction("48 89 35 ? ? ? ? 8D 5E 03 0F 1F 44 00 00 48 8B CF E8 ? ? ? ? 48", 3) - 8);
    Offsets::MapAddress("FactoryManager::Instance", Offsets::OffsetFromInstruction("48 89 05 ? ? ? ? EB 07 48 89 3D ? ? ? ? B9 18 01 02 00 E8 ? ? ? ? 48", 3));

    Offsets::MapSignature("CoreFileManager::Constructor", "40 53 48 83 EC 20 48 8D 05 ? ? ? ? 48 89 51 08 48 89 01 48 8B D9 48 83 C1 10 FF 15 ? ? ? ? 48 8D 4B 18 FF 15 ? ? ? ? 33 C0");
    Offsets::MapSignature("CoreFileManager::RegisterEventListener", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8D 59 18 48 8B F9 48 8B CB 48 8B F2 FF 15 ? ? ? ? 84 C0 75 09 48 8B CB FF 15 ? ? ? ? 48 8D 4F 28 48 8B D6 E8");
    Offsets::MapSignature("RTTIRefObject::DecrementRef", "40 53 48 83 EC 20 48 8B D9 B8 ? ? ? ? F0 0F C1 41 ? 25 ? ? ? ? 83 F8 01 75 34 8B 41");
    Offsets::MapSignature("String::FromCString", "40 53 48 83 EC 20 48 8B D9 48 C7 01 00 00 00 00 49 C7 C0 FF FF FF FF");
    Offsets::MapSignature("String::FromString", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B 39 48 8B F2 48 8B D9 48 3B 3A 74 54 48 89 6C 24");
    Offsets::MapSignature("String::~String", "40 53 48 83 EC 20 48 8B 19 48 8D 05 ? ? ? ? 48 83 EB 10 48 3B D8 74 27 B8 ? ? ? ? F0 0F C1 03 0F BA F0 1F 83 F8 01 75 15 48 8B");
    Offsets::MapSignature("RTTI::GetName", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 70 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 0F B6 41 04 48 8B FA 48 8B F1 83 F8");
    Offsets::MapSignature("RTTI::ToString", "4C 8B DC 57 41 54 41 55 48 83 EC 70 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 4C 8B E9 4D 8B E0 0F B6");
    Offsets::MapSignature("PackFileDevice::LoadPackfile", "40 55 53 57 41 55 41 57 48 8D 6C 24 E0 48 81 EC 20 01 00 00 48");
    Offsets::MapSignature("PackFileDevice::LoadPackfiles", "4C 8B DC 55 53 56 41 56 49 8D 6B A1 48 81 EC B8 00 00 00 48");
    Offsets::MapSignature("FileSystem::Find", "4C 8B DC 53 57 48 83 EC 78 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 40 33 DB 49 89 6B E8 49 89 73 E0 48 8B FA 49 8B");
    Offsets::MapSignature("ExportedSymbols::Collect", "48 89 5C 24 18 48 89 6C 24 20 56 57 41 54 41 56 41 57 48 83 EC 40 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 30 4C 8B F1 E8 ");

    // Exported symbols
    Offsets::MapSignature("gMemFree", "48 83 EC 28 4C 8B C1 48 85 C9 0F 84 ? ? ? ? 80 3D ? ? ? ? ? 4C 8B 0D ? ? ? ? 8B 0D ? ? ? ? 41 BA");

    CoreFileManager_Constructor = Offsets::ResolveID<"CoreFileManager::Constructor", decltype(CoreFileManager_Constructor)>();
    CoreFileManager_RegisterEventListener = Offsets::ResolveID<"CoreFileManager::RegisterEventListener", decltype(CoreFileManager_RegisterEventListener)>();
    PackFileDevice_LoadPackfile = Offsets::ResolveID<"PackFileDevice::LoadPackfile", decltype(PackFileDevice_LoadPackfile)>();
    PackFileDevice_LoadPackfiles = Offsets::ResolveID<"PackFileDevice::LoadPackfiles", decltype(PackFileDevice_LoadPackfiles)>();
    ExportedSymbols_Collect = Offsets::ResolveID<"ExportedSymbols::Collect", decltype(ExportedSymbols_Collect)>();
    // @formatter:on

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach((PVOID *) &CoreFileManager_Constructor, (PVOID) CoreFileManager_Constructor_Hook);
    DetourAttach((PVOID *) &PackFileDevice_LoadPackfile, (PVOID) PackFileDevice_LoadPackfile_Hook);
    DetourAttach((PVOID *) &PackFileDevice_LoadPackfiles, (PVOID) PackFileDevice_LoadPackfiles_Hook);
    DetourAttach((PVOID *) &ExportedSymbols_Collect, (PVOID) ExportedSymbols_Collect_Hook);
    DetourTransactionCommit();
}

void Dumper::Detach() {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach((PVOID *) &CoreFileManager_Constructor, (PVOID) CoreFileManager_Constructor_Hook);
    DetourDetach((PVOID *) &PackFileDevice_LoadPackfile, (PVOID) PackFileDevice_LoadPackfile_Hook);
    DetourDetach((PVOID *) &PackFileDevice_LoadPackfiles, (PVOID) PackFileDevice_LoadPackfiles_Hook);
    DetourDetach((PVOID *) &ExportedSymbols_Collect, (PVOID) ExportedSymbols_Collect_Hook);
    DetourTransactionCommit();
}
