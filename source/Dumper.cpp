#include "Dumper.h"

#include <Windows.h>

#include "detours.h"
#include "Pattern16.h"

#include "Offsets.h"
#include "Core/RTTI.h"
#include "Exporter/JsonExporter.h"
#include "Exporter/IdaExporter.h"
#include "PCore/MemoryPool.h"

#include <set>
#include <array>
#include <print>
#include <XUtil.h>

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
    return inFirst->TypeName() < inSecond->TypeName();
};

static std::set<const RTTI *, decltype(TypeComparator)> AllTypes;

static void ScanType(const RTTI &inType) {
    auto name = inType.TypeName();
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

static void (*FactoryManager_RegisterType)(void *, const RTTI &);

static void FactoryManager_RegisterType_Hook(void *inFactory, const RTTI &inType) {
    FactoryManager_RegisterType(inFactory, inType);
    ScanType(inType);

    if (AllTypes.size() == 9057) {
        puts("Scanning memory...\n");
        ScanMemoryForTypes();

        puts("Exporting types...");
        std::vector<const RTTI *> types{AllTypes.cbegin(), AllTypes.cend()};
        JsonExporter("hrzr").Export(types);
        IdaExporter("hrzr").Export(types);

        exit(EXIT_SUCCESS);
    }
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
    Offsets::MapAddress("MemoryPool::Instance", offsetFromInstruction("48 8B 0D ? ? ? ? 48 85 C0 48 0F 45 C8 48 8B 01 FF 50 08 45 33 C0 48 8D 15", 3));
    Offsets::MapAddress("String::sEmptyBuffer", offsetFromInstruction("48 8D 15 ? ? ? ? 48 3B C2 B9 07 00 00 00 C7 00 01 00 00 00 41 0F 44 C8 89", 3));
    Offsets::MapSignature("String::Buffer::~Buffer", "40 57 48 83 EC 20 83 39 00 48 8B F9 7D 6B 48 89 5C 24 38 48 8D 05");
    Offsets::MapSignature("MurmurHashValue::ToString", "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 8B F1 48 8B FA 48");
    // @formatter:on

    FactoryManager_RegisterType = Offsets::ResolveID<"FactoryManager::RegisterType", decltype(FactoryManager_RegisterType)>();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(reinterpret_cast<PVOID *>(&FactoryManager_RegisterType), static_cast<PVOID>(FactoryManager_RegisterType_Hook));
    DetourTransactionCommit();
}

void Dumper::Detach() {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(reinterpret_cast<PVOID *>(&FactoryManager_RegisterType), static_cast<PVOID>(FactoryManager_RegisterType_Hook));
    DetourTransactionCommit();
}
