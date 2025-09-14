#pragma once

#include "Decima/PCore/Array.h"
#include "Decima/PCore/HashMap.h"
#include "Decima/PCore/String.h"
#include "Decima/Core/RTTIObject.h"
#include "Util/Offsets.h"

#include <cstdint>

struct ExportedSymbol {
    enum class Kind : uint32_t {
        Atom = 0x0,
        Enum = 0x1,
        Class = 0x02,
        Struct = 0x03,
        Typedef = 0x04,
        Function = 0x5,
        Variable = 0x6,
        Container = 0x7,
        Reference = 0x8
    };

    struct TypeInfo {
        RTTI *mType;
        String mTypeName;
        RTTIRefObject *mObject;
    };

    struct Signature {
        String mName;
        String mModifiers;
        TypeInfo mType;
    };

    struct Language {
        void *mAddress;
        const char *mName;
        const char *mHeaderName;
        const char *mSourceName;
        Array<Signature> mSignature;
        uint64_t mToStringFn;
        uint64_t mConstructorFn;
    };

    Kind mKind;
    RTTI *mType;
    const char *mNamespace;
    const char *mName;
    RTTI *mRepresentationType;
    Language mLanguage[3];
};

class ExportedSymbolGroup : public RTTIObject {
public:
    ExportedSymbolGroup() = delete;

    virtual void RegisterSymbols() = 0;

public:
    uint32_t mExportMask;
    const char *mNamespace;
    Array<ExportedSymbol> mSymbols;
    Array<RTTI *> mDependencies;
};

struct ExportedSymbols {
    Array<ExportedSymbolGroup *> mGroups;
    Array<RTTICompound *> field_10;
    Array<RTTICompound *> field_20;
    Array<RTTICompound *> field_30;
    HashMap<uint32_t, const ExportedSymbol *> mAllSymbols;
    HashMap<String, const ExportedSymbol *> mTypeSymbols;

    static const ExportedSymbols &Get() {
        return *Offsets::ResolveID<"ExportedSymbols::Instance", ExportedSymbols *>();
    }
};
