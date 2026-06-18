#include "Dumper.h"

#include <Windows.h>

#include "detours.h"
#include "Pattern16.h"

#include "Util/Offsets.h"
#include "Util/XUtil.h"

#include "Decima/Core/RTTI.h"
#include "Decima/Core/ExportedSymbolGroup.h"
#include "Decima/Core/FactoryManager.h"
#include "Decima/Core/GraphProgramResource.h"
#include "Decima/Core/FRGBAColor.h"

#include "Exporter/AttrExporter.h"
#include "Exporter/JsonExporter.h"
#include "Exporter/IdaExporter.h"
#include "Exporter/JsonSymbolExporter.h"

#include "nixxes_log.h"

#include <array>
#include <algorithm>
#include <set>
#include <string>
#include <print>
#include <ranges>

using namespace std::string_view_literals;

namespace nx {
    INxLog *INxLog::Instance() {
        return *Offsets::ResolveID<"NxLogImpl::Instance", NxLogImpl **>();
    }
}

static auto TypeComparator = [](const RTTI *inFirst, const RTTI *inSecond) -> bool {
    static constexpr std::array Order{
        RTTIKind::Compound,
        RTTIKind::Enum,
        RTTIKind::EnumFlags,
        RTTIKind::BitSet,
        RTTIKind::Atom,
        RTTIKind::Pointer,
        RTTIKind::Container,
        RTTIKind::POD
    };
    if (inFirst->mKind != inSecond->mKind)
        return std::ranges::find(Order, inFirst->mKind) < std::ranges::find(Order, inSecond->mKind);
    return inFirst->Name() < inSecond->Name();
};

static void ScanType(const RTTI &inType) {
    static std::set<const RTTI *, decltype(TypeComparator)> SeenTypes;

    auto name = inType.Name();
    if (SeenTypes.contains(&inType))
        return;

    SeenTypes.emplace(&inType);
    FactoryManager::Get().Register(inType);

    std::print("[RTTI] Registered '{}' (total: {})\n", name, SeenTypes.size());
    std::fflush(stdout);

    if (const auto type = inType.AsCompound(); type) {
        for (auto &message: type->MessageHandlers())
            ScanType(*message.mMessage);
        for (auto &base: type->Bases())
            ScanType(*base.mType);
        for (auto &base: type->Bases())
            ScanType(*base.mType);
    }
    if (const auto type = inType.AsAtom())
        ScanType(*type->mParentType);
    if (const auto type = inType.AsContainer())
        ScanType(*type->mItemType);
    if (const auto type = inType.AsPointer())
        ScanType(*type->mItemType);
}

static void ScanMemoryForTypes() {
    auto [rdataBase, rdataEnd] = Offsets::GetRdataSection();
    auto [dataBase, dataEnd] = Offsets::GetDataSection();

    auto IsValidPtr = [&]<typename T>(T *ptr) {
        if (ptr == nullptr)
            return false;
        auto value = reinterpret_cast<uintptr_t>(ptr);
        return value >= dataBase && value < dataEnd || value >= rdataBase && value < rdataEnd;
    };

    auto cur = reinterpret_cast<void *>(dataBase);
    auto end = dataEnd;

    while (true) {
        auto type = static_cast<RTTI *>(Pattern16::scan(
            cur,
            end - reinterpret_cast<uintptr_t>(cur),
            "FF FF FF FF [00000???]"
        ));

        if (type == nullptr)
            break;

        cur = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(type) + 5);

        if (auto atom = type->AsAtom(); atom) {
            if (atom->mSize == 0 ||
                atom->mAlignment == 0 ||
                atom->mConstructor && !IsValidPtr(atom->mConstructor) ||
                atom->mDestructor && !IsValidPtr(atom->mDestructor) ||
                !IsValidPtr(atom->mTypeName) ||
                !IsValidPtr(atom->mParentType)
            ) {
                continue;
            }
        } else if (auto enum_ = type->AsEnum(); enum_) {
            if (enum_->mSize == 0 ||
                !IsValidPtr(enum_->mTypeName) ||
                !IsValidPtr(enum_->mValues)
            ) {
                continue;
            }
        } else if (auto enum_ = type->AsBitSet(); enum_) {
            if (enum_->mSize == 0 ||
                !IsValidPtr(enum_->mTypeName) ||
                !IsValidPtr(enum_->mRepresentationType)
            ) {
                continue;
            }
        } else if (auto container = type->AsContainer(); container) {
            if (!IsValidPtr(container->mItemType) ||
                !IsValidPtr(container->mContainerType) ||
                !IsValidPtr(container->mContainerType->mTypeName) ||
                container->mContainerType->mConstructor && !IsValidPtr(container->mContainerType->mConstructor) ||
                container->mContainerType->mDestructor && !IsValidPtr(container->mContainerType->mDestructor)
            ) {
                continue;
            }
        } else if (auto pointer = type->AsPointer(); pointer) {
            if (!IsValidPtr(pointer->mItemType) ||
                !IsValidPtr(pointer->mPointerType) ||
                !IsValidPtr(pointer->mPointerType->mTypeName) ||
                pointer->mPointerType->mConstructor && !IsValidPtr(pointer->mPointerType->mConstructor) ||
                pointer->mPointerType->mDestructor && !IsValidPtr(pointer->mPointerType->mDestructor)
            ) {
                continue;
            }
        } else if (auto compound = type->AsCompound(); compound) {
            if (!IsValidPtr(compound->mTypeName) ||
                compound->mNumBases && !IsValidPtr(compound->mBases) ||
                compound->mNumAttrs && !IsValidPtr(compound->mAttrs) ||
                compound->mNumMessageHandlers && !IsValidPtr(compound->mMessageHandlers)
            ) {
                continue;
            }
        } else {
            continue;
        }

        ScanType(*type);
    }
}

static void DSDebugPrintString_PrintString_Hook(pcTChar inText, bool inUnk1, bool inUnk2, FRGBAColor inColor, float inUnk3) {
    (void) inColor;
    std::println("[DEBUG] {} (inUnk1: {}, inUnk2: {}, inUnk3: {})", inText, inUnk1, inUnk2, inUnk3);
}

static void GraphProgramInstance_sOnNodeGraphAlert_Hook(pcTChar inText, bool inUnk) {
    std::println("[GRAPH ALERT] {} (inUnk: {})", inText, inUnk);
}

static void GraphProgramInstance_sOnNodeGraphAlertWithName_Hook(pcTChar inText, pcTChar inUnk1, pcTChar inUnk2, bool inUnk) {
    std::println("[GRAPH ALERT] {} (inUnk1: {}, inUnk2: {}, inUnk: {})", inText, inUnk1, inUnk2, inUnk);
}

static void GraphProgramInstance_sOnNodeGraphTrace_Hook(const GGUUID& inUUID, pcTChar inText) {
    std::println("[GRAPH TRACE] {} {}", inText, inUUID);
}

static void (*RTTIFactory_RegistersSymbols)(void *);

static void RTTIFactory_RegistersSymbols_Hook(void *inUnk) {
    RTTIFactory_RegistersSymbols(inUnk);

    static std::unordered_map<std::string_view, void*> Hooks{
        // @formatter:off
        {"DSDebugPrintString_sExportedPrintString"sv, reinterpret_cast<void*>(DSDebugPrintString_PrintString_Hook)},
        {"GraphProgramInstance::sOnNodeGraphAlert"sv, reinterpret_cast<void*>(GraphProgramInstance_sOnNodeGraphAlert_Hook)},
        {"GraphProgramInstance::sOnNodeGraphAlertWithName"sv, reinterpret_cast<void*>(GraphProgramInstance_sOnNodeGraphAlertWithName_Hook)},
        {"GraphProgramInstance::sOnNodeGraphTrace"sv, reinterpret_cast<void*>(GraphProgramInstance_sOnNodeGraphTrace_Hook)}
        // @formatter:on
    };

    for (auto & [symbol, _] : ExportedSymbols::Get().mAllSymbols) {
        if (auto it = Hooks.find(symbol->mName); it != Hooks.end()) {
            symbol->mLanguage[0].mAddress = it->second;
            std::println("Hooked {}", symbol->mName);
        }
    }

    Dumper::Dump();
    ExitProcess(0);
}

void Dumper::Attach() {
    // @formatter:off
    Offsets::MapAddress("String::sEmptyBuffer", Offsets::OffsetFromInstruction("48 8D 35 ? ? ? ? 89 2D ? ? ? ? 48 8D 48 F0 48 3B CE 74 08", 3) + 8);
    Offsets::MapSignature("String::~String", "48 8B 11 48 8D 05 ? ? ? ? 48 83 EA 10 48 3B D0 0F 84 ? ? ? ? B8");

    Offsets::MapAddress("FactoryManager::Instance", Offsets::OffsetFromInstruction("48 8B 0D ? ? ? ? 89 44 24 2C E8 ? ? ? ? 48 89 83 B0 00 00 00 48 83 C4 30 5B C3", 3));
    Offsets::MapAddress("RTTIFactory::sExportedSymbols", Offsets::OffsetFromInstruction("48 8D 0D ? ? ? ? 48 89 7C 24 58 4C 89 6C 24 48 4C 89 74 24 40 E8 ? ? ? ? 4C 8B", 3));
    Offsets::MapSignature("RTTIFactory::RegisterType", "48 89 54 24 10 55 56 57 48 8D 6C 24 F0 48 81 EC 10 01 00 00 0F B6 42");
    Offsets::MapSignature("RTTIFactory::RegisterAllTypes", "40 55 48 8B EC 48 83 EC 70 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 48 89");
    Offsets::MapSignature("RTTIFactory::RegisterSymbols", "48 89 4C 24 08 56 48 83 EC 70 48 89 5C 24 68 48 8D 0D ? ? ? ? 48 89");
    // @formatter:on

    RTTIFactory_RegistersSymbols = Offsets::ResolveID<"RTTIFactory::RegisterSymbols", decltype(RTTIFactory_RegistersSymbols)>();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(reinterpret_cast<PVOID *>(&RTTIFactory_RegistersSymbols), static_cast<PVOID>(RTTIFactory_RegistersSymbols_Hook));
    DetourTransactionCommit();
}

void Dumper::Detach() {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(reinterpret_cast<PVOID *>(&RTTIFactory_RegistersSymbols), static_cast<PVOID>(RTTIFactory_RegistersSymbols_Hook));
    DetourTransactionCommit();
}

void Dumper::Dump() {
    auto &factory = FactoryManager::Get();

    auto &type = *factory.Find("MotionMatchingVecN"sv).AsAtom();
    auto data = malloc(type.mSize);
    type.mConstructor(type, data);
    String str;
    type.mToString(data, str);
    if (type.mDestructor)
        type.mDestructor(type, data);
    free(data);

    puts("Scanning types for unreferenced members...");
    for (auto type: factory.Types())
        ScanType(*type);

    puts("Scanning memory for unreferenced types...");
    ScanMemoryForTypes();

    std::vector<pcRTTI> types{factory.Types().begin(), factory.Types().end()};
    std::sort(types.begin(), types.end(), TypeComparator);

    puts("Exporting types...");
    AttrExporter("dump/ds2").Export(types);
    JsonExporter("dump/ds2").Export(types);
    IdaExporter("dump/ds2").Export(types);
    JsonSymbolExporter("dump/ds2_symbols.json").ExportSymbols();
}
