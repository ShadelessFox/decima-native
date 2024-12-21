#include "Dumper.h"

#include <Windows.h>

#include "detours.h"
#include "Pattern16.h"

#include "Util/Offsets.h"
#include "Util/XUtil.h"

#include "Core/RTTI.h"
#include "PCore/MemoryPool.h"

#include "Exporter/JsonExporter.h"
#include "Exporter/IdaExporter.h"
#include "Exporter/SymbolsExporter.h"

#include <set>
#include <array>
#include <print>

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
    return inFirst->Name() < inSecond->Name();
};

static std::set<const RTTI *, decltype(TypeComparator)> AllTypes;

static void ScanType(const RTTI &inType) {
    auto name = inType.Name();
    if (AllTypes.contains(&inType))
        return;

    AllTypes.emplace(&inType);
    std::print("[RTTI] Registered '{}' (total: {})\n", name, AllTypes.size());
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
                compound->mNumFunctions && !IsValidPtr(compound->mFunctions) ||
                compound->mNumMessageHandlers && !IsValidPtr(compound->mMessageHandlers)
            ) {
                continue;
            }
        } else {
            continue;
        }

        std::println("Scanning type at {}", reinterpret_cast<void *>(type));
        ScanType(*type);
    }
}

static void ExportTypes() {
    puts("Scanning memory...\n");
    ScanMemoryForTypes();

    puts("Exporting types...");
    std::vector<const RTTI *> types{AllTypes.cbegin(), AllTypes.cend()};
    JsonExporter("hrzr").Export(types);
    IdaExporter("hrzr").Export(types);
    SymbolsExporter("hrzr_symbols").Export(types);

    ExitProcess(0);
}

static void (*FactoryManager_RegisterType)(void *, const RTTI &);
static void (*Symbols_ExportSymbolGroups)();

static void FactoryManager_RegisterType_Hook(void *inFactory, const RTTI &inType) {
    FactoryManager_RegisterType(inFactory, inType);
    ScanType(inType);
}

static void Symbols_ExportSymbolGroups_Hook() {
    Symbols_ExportSymbolGroups();
    ExportTypes();
}

void Dumper::Attach() {
    auto [moduleBase, moduleEnd] = Offsets::GetModule();
    auto offsetFromInstruction = [&](const char *Signature, uint32_t Add) {
        auto addr = XUtil::FindPattern(moduleBase, moduleEnd - moduleBase, Signature);
        if (!addr)
            return addr;
        auto relOffset = *reinterpret_cast<int32_t *>(addr + Add) + sizeof(int32_t);
        return addr + Add + relOffset - moduleBase;
    };

    // @formatter:off
    Offsets::MapSignature("FactoryManager::RegisterType", "48 89 5C 24 20 55 48 83 EC 20 F6 42 05 01 48 8B DA 48 89 74 24 30 48");
    Offsets::MapSignature("Symbols::ExportSymbolGroups", "41 56 48 83 EC 30 48 89 5C 24 ? 48 8D 0D ? ? ? ? 48 89 6C 24 ? 48 89");
    Offsets::MapAddress("Symbols::sExportedSymbolGroups", offsetFromInstruction("48 8B 3D ? ? ? ? 48 63 05 ? ? ? ? 48 8D 2C C7 48 3B FD 74 45 48 8B", 3) - 8);

    Offsets::MapAddress("MemoryPool::Instance", offsetFromInstruction("48 8B 0D ? ? ? ? 48 85 C0 48 0F 45 C8 48 8B 01 FF 50 08 45 33 C0 48 8D 15", 3));
    Offsets::MapAddress("String::sEmptyBuffer", offsetFromInstruction("48 8D 15 ? ? ? ? 48 3B C2 B9 07 00 00 00 C7 00 01 00 00 00 41 0F 44 C8 89", 3));
    Offsets::MapSignature("String::Buffer::~Buffer", "40 57 48 83 EC 20 83 39 00 48 8B F9 7D 6B 48 89 5C 24 38 48 8D 05");
    Offsets::MapSignature("GGUUID::ToString", "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 8B F1 48 8B FA 48");
    Offsets::MapSignature("StreamingDataSource::MakeId", "48 89 5C 24 08 4C 8B 19 33 C0 F2 49 0F 38 F1 C3 44 8B D0 49 8B C3 F2 4C 0F 38 F1");

    Offsets::MapSignature("Stream::ReadInt32", "48 89 5C 24 18 57 48 83 EC 20 48 8B 01 48 8B FA 41 B8 04 00 00 00 48 8D 54 24 38 48");
    Offsets::MapSignature("Stream::ReadStreamingDataSource", "48 89 5C 24 18 57 48 83 EC 20 48 8B 01 48 8B FA 41 B8 01 00 00 00 48 8D 54 24 30 48");
    // @formatter:on

    FactoryManager_RegisterType = Offsets::ResolveID<"FactoryManager::RegisterType", decltype(FactoryManager_RegisterType)>();
    Symbols_ExportSymbolGroups = Offsets::ResolveID<"Symbols::ExportSymbolGroups", decltype(Symbols_ExportSymbolGroups)>();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(reinterpret_cast<PVOID *>(&FactoryManager_RegisterType), reinterpret_cast<PVOID>(FactoryManager_RegisterType_Hook));
    DetourAttach(reinterpret_cast<PVOID *>(&Symbols_ExportSymbolGroups), reinterpret_cast<PVOID>(Symbols_ExportSymbolGroups_Hook));
    DetourTransactionCommit();
}

void Dumper::Detach() {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(reinterpret_cast<PVOID *>(&FactoryManager_RegisterType), reinterpret_cast<PVOID>(FactoryManager_RegisterType_Hook));
    DetourDetach(reinterpret_cast<PVOID *>(&Symbols_ExportSymbolGroups), reinterpret_cast<PVOID>(Symbols_ExportSymbolGroups_Hook));
    DetourTransactionCommit();
}
