#include "rtti.hpp"

std::string_view RTTI::GetName() const {
    switch (mKind) {
        case RTTIKind::Atom:
            return "<Atom>";
        case RTTIKind::Pointer:
            return "<Pointer>";
        case RTTIKind::Container:
            return "<Container>";
        case RTTIKind::Enum:
            return "<Enum>";
        case RTTIKind::Compound:
            return reinterpret_cast<const RTTIClass*>(this)->mTypeName;
        case RTTIKind::EnumFlags:
            return "<EnumFlags>";
        case RTTIKind::POD:
            return "<POD>";
    }
}