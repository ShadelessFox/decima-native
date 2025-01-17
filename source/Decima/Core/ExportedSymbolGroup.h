#pragma once

#include "RTTI.h"
#include "RTTIObject.h"
#include "Decima/PCore/Array.h"
#include "Decima/PCore/HashMap.h"
#include "Decima/PCore/String.h"
#include "Util/Typedefs.h"

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
        Unk10 = 10
    };

    struct Signature {
        pcTChar mName;
        pcTChar mModifiers;
        pRTTI mType;
        pVoid mUnk18;
        uint8_t mUnk20;
    };

    struct Language {
        pVoid mAddress;
        pcTChar mName;
        pVoid mUnk10;
        pVoid mUnk18;
        Array<Signature> mSignature;
        pVoid mUnk30;
        pVoid mUnk38;
    };

    Kind mKind;
    pcRTTI mType;
    pcTChar mNamespace;
    pcTChar mName;
    pVoid mUnk20;
    uint8_t mUnk28;
    Language mLanguage[2];
};

assert_size(ExportedSymbol, 0xB0);
assert_size(ExportedSymbol::Kind, 0x1);
assert_size(ExportedSymbol::Signature, 0x28);
assert_size(ExportedSymbol::Language, 0x40);

class ExportedSymbolGroup : public RTTIObject {
public:
    ExportedSymbolGroup() = delete;

    virtual void RegisterSymbols() = 0;

public:
    uint32_t mExportMask;
    pcTChar mNamespace;
    Array<ExportedSymbol> mSymbols;
    Array<pcRTTI> mDependencies;
};

assert_size(ExportedSymbolGroup, 0x38);

struct ExportedSymbols {
    Array<ExportedSymbolGroup *> mGroups;
    Array<pcRTTI> mDependenciesUnk1;
    Array<pcRTTI> mDependenciesUnk2;
    HashMap<ExportedSymbol *, uint32_t> mAllSymbols;
    HashMap<String, ExportedSymbol *> mTypeSymbols;

    static const ExportedSymbols &Get() {
        return *Offsets::ResolveID<"RTTIFactory::sExportedSymbols", ExportedSymbols *>();
    }
};