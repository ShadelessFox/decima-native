#include "Dumper.h"

#include <Windows.h>

#include "detours.h"
#include "Pattern16.h"

#include "Util/Offsets.h"
#include "Util/XUtil.h"

#include "Core/RTTI.h"
#include "Core/GGUUID.h"
#include "PCore/MemoryPool.h"
#include "PCore/StreamingGraphResource.h"

#include "Exporter/JsonExporter.h"
#include "Exporter/IdaExporter.h"

#include <set>
#include <array>
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
                // compound->mNumFunctions && !IsValidPtr(compound->mFunctions) ||
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

static bool (*RTTIFactory_RegisterType)(void *, const RTTI &);

static void (*RTTIFactory_RegisterAllTypes)();

static void (*StreamingGraphResource_ResolveTypeHashes)(StreamingGraphResource &);

static bool RTTIFactory_RegisterType_Hook(void *inFactory, const RTTI &inType) {
    if (RTTIFactory_RegisterType(inFactory, inType)) {
        ScanType(inType);
        return true;
    }
    return false;
}

static void RTTIFactory_RegisterAllTypes_Hook() {
    RTTIFactory_RegisterAllTypes();

    puts("Scanning memory...\n");
    ScanMemoryForTypes();

    puts("Exporting types...");
    std::vector<const RTTI *> types{AllTypes.cbegin(), AllTypes.cend()};
    JsonExporter("dump/hfw").Export(types);
    IdaExporter("dump/hfw").Export(types);

    exit(EXIT_SUCCESS);
}

static void StreamingGraphResource_ResolveTypeHashes_Hook(StreamingGraphResource &graph) {
    StreamingGraphResource_ResolveTypeHashes(graph);
}

void Dumper::Attach() {
    // @formatter:off
    Offsets::MapSignature("RTTIFactory::RegisterType", "40 55 53 56 48 8D 6C 24 ? 48 81 EC ? ? ? ? 0F B6 42 05 48 8B DA 48 8B");
    Offsets::MapSignature("RTTIFactory::RegisterAllTypes", "40 55 48 8B EC 48 83 EC 70 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 48 89 9C 24");
    Offsets::MapSignature("StreamingGraphResource::ResolveTypeHashes", "48 89 5C 24 20 56 57 41 54 41 56 41 57 48 83 EC 20 65 48 8B 04 25 58");
    // @formatter:on

    RTTIFactory_RegisterType = Offsets::ResolveID<"RTTIFactory::RegisterType", decltype(RTTIFactory_RegisterType)>();
    RTTIFactory_RegisterAllTypes = Offsets::ResolveID<"RTTIFactory::RegisterAllTypes", decltype(RTTIFactory_RegisterAllTypes)>();
    StreamingGraphResource_ResolveTypeHashes = Offsets::ResolveID<"StreamingGraphResource::ResolveTypeHashes", decltype(StreamingGraphResource_ResolveTypeHashes)>();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    // DetourAttach(reinterpret_cast<PVOID *>(&RTTIFactory_RegisterType), static_cast<PVOID>(RTTIFactory_RegisterType_Hook));
    // DetourAttach(reinterpret_cast<PVOID *>(&RTTIFactory_RegisterAllTypes), static_cast<PVOID>(RTTIFactory_RegisterAllTypes_Hook));
    DetourAttach(reinterpret_cast<PVOID *>(&StreamingGraphResource_ResolveTypeHashes), static_cast<PVOID>(StreamingGraphResource_ResolveTypeHashes_Hook));
    DetourTransactionCommit();
}

void Dumper::Detach() {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    // DetourDetach(reinterpret_cast<PVOID *>(&RTTIFactory_RegisterType), static_cast<PVOID>(RTTIFactory_RegisterType_Hook));
    // DetourDetach(reinterpret_cast<PVOID *>(&RTTIFactory_RegisterAllTypes), static_cast<PVOID>(RTTIFactory_RegisterAllTypes_Hook));
    DetourDetach(reinterpret_cast<PVOID *>(&StreamingGraphResource_ResolveTypeHashes), static_cast<PVOID>(StreamingGraphResource_ResolveTypeHashes_Hook));
    DetourTransactionCommit();
}
