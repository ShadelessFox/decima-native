#pragma once

#include "RTTI.h"
#include "RTTIObject.h"
#include "Decima/PCore/Array.h"
#include "Decima/PCore/HashMap.h"
#include "Decima/PCore/String.h"
#include "Util/Typedefs.h"

#include <string>

struct ExportedSymbol {
    enum class Kind : uint8_t {
        Atom = 0,
        Enum = 1,
        Class = 2,
        Struct = 3,
        Typedef = 4,
        Function = 5,
        Variable = 6,
        Container = 7,
        Reference = 8,
        Pointer = 9,
        Source = 10
    };

    struct Signature {
        const char *mName;
        const char *mModifiers;
        RTTI *mType;
        void *mUnk18;
        void *mUnk20;
        void *mUnk28;
    };

    struct Language {
        void *mAddress;
        const char *mName;
        void *mUnk10;
        void *mUnk18;
        Array<Signature> mSignature;
        void *mUnk30;
        void *mUnk38;
    };

    Kind mKind;
    const RTTI *mType;
    const char *mNamespace;
    const char *mName;
    void *mUnk20;
    uint8_t mUnk28;
    Language mLanguage[2];
};

assert_size(ExportedSymbol, 0xB0);
assert_size(ExportedSymbol::Kind, 0x1);
assert_size(ExportedSymbol::Signature, 0x30);
assert_size(ExportedSymbol::Language, 0x40);

class ExportedSymbolGroup : public RTTIObject {
public:
    ExportedSymbolGroup() = delete;

    virtual void RegisterSymbols() = 0;

public:
    uint32_t mExportMask;
    const char *mNamespace;
    Array<ExportedSymbol> mSymbols;
    Array<const RTTI *> mDependencies;
};

assert_size(ExportedSymbolGroup, 0x38);

struct ExportedSymbols {
    Array<ExportedSymbolGroup *> mGroups;
    Array<const RTTI *> mDependenciesUnk1;
    Array<const RTTI *> mDependenciesUnk2;
    HashMap<ExportedSymbol *, uint32_t> mAllSymbols;
    HashMap<String, ExportedSymbol *> mTypeSymbols;

    static const ExportedSymbols &Get() {
        return *Offsets::ResolveID<"RTTIFactory::sExportedSymbols", ExportedSymbols *>();
    }
};

namespace std {
    inline std::string to_string(ExportedSymbol::Kind value) {
        switch (value) {
            case ExportedSymbol::Kind::Atom: return "Atom";
            case ExportedSymbol::Kind::Enum: return "Enum";
            case ExportedSymbol::Kind::Class: return "Class";
            case ExportedSymbol::Kind::Struct: return "Struct";
            case ExportedSymbol::Kind::Typedef: return "Typedef";
            case ExportedSymbol::Kind::Function: return "Function";
            case ExportedSymbol::Kind::Variable: return "Variable";
            case ExportedSymbol::Kind::Container: return "Container";
            case ExportedSymbol::Kind::Reference: return "Reference";
            case ExportedSymbol::Kind::Pointer: return "Pointer";
            case ExportedSymbol::Kind::Source: return "Source";
            default: return "Unknown";
        }
    }
}
