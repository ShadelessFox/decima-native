#include "RTTI.h"

#include <format>
#include <Offsets.h>

[[nodiscard]] std::string RTTI::BaseName() const {
    switch (mKind) {
        case RTTIKind::Atom:
            return reinterpret_cast<const RTTIAtom *>(this)->mTypeName;
        case RTTIKind::Pointer:
        case RTTIKind::Container:
            return reinterpret_cast<const RTTIContainer *>(this)->mContainerType->mTypeName;
        case RTTIKind::Enum:
        case RTTIKind::EnumFlags:
            return reinterpret_cast<const RTTIEnum *>(this)->mTypeName;
        case RTTIKind::Compound:
            return reinterpret_cast<const RTTICompound *>(this)->mTypeName;
        default:
            throw std::runtime_error("Unreachable code");
    }
}

[[nodiscard]] std::string RTTI::Name() const {
    switch (mKind) {
        case RTTIKind::Atom:
            return reinterpret_cast<const RTTIAtom *>(this)->mTypeName;
        case RTTIKind::Pointer:
        case RTTIKind::Container: {
            const auto container = reinterpret_cast<const RTTIContainer *>(this);
            return std::format("{}<{}>", container->mContainerType->mTypeName, container->mItemType->Name());
        }
        case RTTIKind::Enum:
        case RTTIKind::EnumFlags:
            return reinterpret_cast<const RTTIEnum *>(this)->mTypeName;
        case RTTIKind::Compound:
            return reinterpret_cast<const RTTICompound *>(this)->mTypeName;
        case RTTIKind::POD:
            return std::format("POD{}", reinterpret_cast<const RTTIPod *>(this)->mSize);
        default:
            throw std::runtime_error("Unreachable code");
    }
}

[[nodiscard]] std::string RTTI::KindName() const {
    switch (mKind) {
        case RTTIKind::Atom:
            return "primitive";
        case RTTIKind::Pointer:
            return "pointer";
        case RTTIKind::Container:
            return "container";
        case RTTIKind::Enum:
            return "enum";
        case RTTIKind::Compound:
            return "class";
        case RTTIKind::EnumFlags:
            return "enum flags";
        case RTTIKind::POD:
            return "pod";
        default:
            throw std::runtime_error("Unreachable code");
    }
}