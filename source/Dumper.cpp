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

#include "Exporter/JsonExporter.h"
#include "Exporter/IdaExporter.h"

#include "Nixxes/NxLog.h"

#include <array>
#include <algorithm>
#include <set>
#include <print>

static auto TypeComparator = [](const RTTI *inFirst, const RTTI *inSecond) -> bool {
    static constexpr std::array Order{
        RTTIKind::Compound,
        RTTIKind::Enum,
        RTTIKind::EnumFlags,
        RTTIKind::EnumBitSet,
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

static bool (*NxInitSystems)();

static bool NxInitSystems_Hook() {
    if (!NxInitSystems())
        return false;

    auto log = NxLog::Instance();
    auto vtbl = *reinterpret_cast<void ***>(log);

    static auto NxLog_PrintA = reinterpret_cast<void(*)(NxLog *, const char *)>(vtbl[7]);
    static auto NxLog_PrintA_Hook = +[](NxLog *inLog, const char *inText) {
        NxLog_PrintA(inLog, inText);
        std::print("{}", inText);
    };

    static auto NxLog_PrintLnA = reinterpret_cast<void(*)(NxLog *, const char *)>(vtbl[8]);
    static auto NxLog_PrintLnA_Hook = +[](NxLog *inLog, const char *inText) {
        NxLog_PrintLnA(inLog, inText);
        std::print("{}\n", inText);
    };

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(reinterpret_cast<PVOID *>(&NxLog_PrintA), static_cast<PVOID>(NxLog_PrintA_Hook));
    DetourAttach(reinterpret_cast<PVOID *>(&NxLog_PrintLnA), static_cast<PVOID>(NxLog_PrintLnA_Hook));
    DetourTransactionCommit();

    return true;
}

static void (*RTTIFactory_RegistersSymbols)(void *);

static void RTTIFactory_RegistersSymbols_Hook(void *inUnk) {
    RTTIFactory_RegistersSymbols(inUnk);

    static auto registered = []() {
        for (auto &[symbol, hash]: ExportedSymbols::Get().mAllSymbols) {
            if (symbol->mKind == ExportedSymbol::Kind::Function) {
                Offsets::MapAddress(symbol->mName, reinterpret_cast<uintptr_t>(symbol->mLanguage[0].mAddress));
            }
        }
        return true;
    }();

    // Dumper::Dump();
}

static void (*GraphProgramInstance_Evaluate)(GraphProgramInstance *);

static void GraphProgramInstance_Evaluate_Hook(GraphProgramInstance *program) {
    auto &entryPoint = program->Program->EntryPoints[0];

    if (entryPoint.EntryPoint != "EntryPoint_Main_Theme_Music_Graph_4d0418bc2f5e06ab3ca104db27ab2d1c_0_Evaluate")
        std::print("Evaluating {}\n", entryPoint.EntryPoint);

    auto &inputBindings = program->InputParameterBindings[0];
    auto &outputBindings = program->OutputParameterBindings[0];
    auto &stateBindings = program->StateParameterBindings;
    auto &dataBindings = program->ExposedDataBindings;

    GraphProgramInstance_Evaluate(program);
}

void Dumper::Attach() {
    // @formatter:off
    Offsets::MapAddress("RTTIFactory::sExportedSymbols", Offsets::OffsetFromInstruction("48 8B 3D ? ? ? ? 48 63 0D ? ? ? ? 40 88 6C 24 ? 48 89 7C 24 ? 48 8D 04 CF 48", 3) - 8);
    Offsets::MapAddress("TrophySystem::Instance", Offsets::OffsetFromInstruction("48 89 1D ? ? ? ? E8 ? ? ? ? 48 8B 1D ? ? ? ? 48 8D 05 ? ? ? ? 48 8D 55 E0 48", 3));
    Offsets::MapAddress("FactoryManager::Instance", Offsets::OffsetFromInstruction("48 8B 0D ? ? ? ? 48 89 54 24 ? 8B 42 F8 89 44 24 28 8B 42 F4 48 8D 54 24 ? 89 44 24 2C E8 ? ? ? ? 48 85 C0 74 0D 48 8B C8 E8", 3));
    Offsets::MapAddress("NxLogImpl::Instance", Offsets::OffsetFromInstruction("48 8B 0D ? ? ? ? 4C 8D 05 ? ? ? ? 48 8D 15 ? ? ? ? 48 8B 01 FF 50 48 48 8B 06 B2 01 48", 3));

    Offsets::MapSignature("RTTIFactory::RegisterType", "40 55 53 56 48 8D 6C 24 ? 48 81 EC ? ? ? ? 0F B6 42 05 48 8B DA 48 8B");
    Offsets::MapSignature("RTTIFactory::RegisterAllTypes", "40 55 48 8B EC 48 83 EC 70 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 48 89 9C 24");
    Offsets::MapSignature("RTTIFactory::RegisterSymbols", "48 89 4C 24 ? 41 56 48 83 EC 50 48 89 5C 24 ? 48 8D 0D ? ? ? ? 48 89");
    Offsets::MapSignature("StreamingGraphResource::ResolveTypeHashes", "48 89 5C 24 20 56 57 41 54 41 56 41 57 48 83 EC 20 65 48 8B 04 25 58");
    Offsets::MapSignature("GraphProgramInstance::Evaluate", "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 83 EC 30 48 8B 41 40");
    Offsets::MapSignature("NxInitSystems", "48 83 EC 38 48 83 3D ? ? ? ? ? 74 07 32 C0 48 83 C4 38 C3 48 83 3D ? ? ? ? ? 48 89 5C 24 40 48 89");
    // @formatter:on

    RTTIFactory_RegistersSymbols = Offsets::ResolveID<"RTTIFactory::RegisterSymbols", decltype(RTTIFactory_RegistersSymbols)>();
    GraphProgramInstance_Evaluate = Offsets::ResolveID<"GraphProgramInstance::Evaluate", decltype(GraphProgramInstance_Evaluate)>();
    NxInitSystems = Offsets::ResolveID<"NxInitSystems", decltype(NxInitSystems)>();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(reinterpret_cast<PVOID *>(&RTTIFactory_RegistersSymbols), static_cast<PVOID>(RTTIFactory_RegistersSymbols_Hook));
    DetourAttach(reinterpret_cast<PVOID *>(&GraphProgramInstance_Evaluate), static_cast<PVOID>(GraphProgramInstance_Evaluate_Hook));
    DetourAttach(reinterpret_cast<PVOID *>(&NxInitSystems), static_cast<PVOID>(NxInitSystems_Hook));
    DetourTransactionCommit();
}

void Dumper::Detach() {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(reinterpret_cast<PVOID *>(&RTTIFactory_RegistersSymbols), static_cast<PVOID>(RTTIFactory_RegistersSymbols_Hook));
    DetourDetach(reinterpret_cast<PVOID *>(&GraphProgramInstance_Evaluate), static_cast<PVOID>(GraphProgramInstance_Evaluate_Hook));
    DetourDetach(reinterpret_cast<PVOID *>(&NxInitSystems), static_cast<PVOID>(NxInitSystems_Hook));
    DetourTransactionCommit();
}

void Dumper::Dump() {
    auto &factory = FactoryManager::Get();

    puts("Scanning types for unreferenced members...");
    for (auto type: factory.Types())
        ScanType(*type);

    puts("Scanning memory for unreferenced types...");
    ScanMemoryForTypes();

    std::vector<pcRTTI> types{factory.Types().begin(), factory.Types().end()};
    std::sort(types.begin(), types.end(), TypeComparator);

    puts("Exporting types...");
    JsonExporter("dump/hfw").Export(types);
    IdaExporter("dump/hfw").Export(types);

    ExitProcess(0);
}
