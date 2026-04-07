#include "RTTI.h"

#include <format>

const RTTIAtom *RTTI::AsAtom() const {
    switch (mKind) {
        case RTTIKind::Atom:
            return reinterpret_cast<const RTTIAtom *>(this);
        default:
            return nullptr;
    }
}

const RTTICompound *RTTI::AsCompound() const {
    switch (mKind) {
        case RTTIKind::Compound:
            return reinterpret_cast<const RTTICompound *>(this);
        default:
            return nullptr;
    }
}

const RTTIEnum *RTTI::AsEnum() const {
    switch (mKind) {
        case RTTIKind::Enum:
        case RTTIKind::EnumFlags:
            return reinterpret_cast<const RTTIEnum *>(this);
        default:
            return nullptr;
    }
}

const RTTIBitSet *RTTI::AsBitSet() const {
    switch (mKind) {
        case RTTIKind::BitSet:
            return reinterpret_cast<const RTTIBitSet *>(this);
        default:
            return nullptr;
    }
}

const RTTIPointer *RTTI::AsPointer() const {
    switch (mKind) {
        case RTTIKind::Pointer:
            return reinterpret_cast<const RTTIPointer *>(this);
        default:
            return nullptr;
    }
}

const RTTIContainer *RTTI::AsContainer() const {
    switch (mKind) {
        case RTTIKind::Container:
            return reinterpret_cast<const RTTIContainer *>(this);
        default:
            return nullptr;
    }
}

const RTTIPod *RTTI::AsPOD() const {
    switch (mKind) {
        case RTTIKind::POD:
            return reinterpret_cast<const RTTIPod *>(this);
        default:
            return nullptr;
    }
}

[[nodiscard]] std::string_view RTTI::Name() const {
    switch (mKind) {
        case RTTIKind::Atom:
            return reinterpret_cast<const RTTIAtom *>(this)->mTypeName;
        case RTTIKind::Pointer:
        case RTTIKind::Container:
            return reinterpret_cast<const RTTIContainer *>(this)->mTypeName;
        case RTTIKind::Enum:
        case RTTIKind::EnumFlags:
            return reinterpret_cast<const RTTIEnum *>(this)->mTypeName;
        case RTTIKind::BitSet:
            return reinterpret_cast<const RTTIBitSet *>(this)->mTypeName;
        case RTTIKind::Compound:
            return reinterpret_cast<const RTTICompound *>(this)->mTypeName;
        case RTTIKind::POD:
            return reinterpret_cast<const RTTIPod *>(this)->mTypeName;
        default:
            throw std::runtime_error("Unreachable code");
    }
}

[[nodiscard]] std::string_view RTTI::KindName() const {
    switch (mKind) {
        case RTTIKind::Atom:
            return "atom";
        case RTTIKind::Pointer:
            return "pointer";
        case RTTIKind::Container:
            return "container";
        case RTTIKind::Enum:
            return "enum";
        case RTTIKind::Compound:
            return "compound";
        case RTTIKind::EnumFlags:
            return "enum flags";
        case RTTIKind::BitSet:
            return "enum bitset";
        case RTTIKind::POD:
            return "pod";
        default:
            throw std::runtime_error("Unreachable code");
    }
}

bool RTTI::IsKindOf(std::string_view inName) const {
    if (Name() == inName)
        return true;
    if (mKind == RTTIKind::Compound) {
        for (const auto &base: reinterpret_cast<const RTTICompound *>(this)->Bases())
            if (base.mType->IsKindOf(inName))
                return true;
    }
    return false;
}
