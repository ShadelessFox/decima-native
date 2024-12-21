#pragma once

#include "Core/RTTI.h"
#include "PCore/Array.h"
#include "PCore/String.h"

enum class SymbolKind : uint32_t {
    Atom = 0,
    Enum = 1,
    Class = 2,
    Struct = 3,
    Typedef = 4,
    Function = 5,
    Variable = 6,
    Container = 7,
    SourceFile = 8
};

[[nodiscard]] const char *SymbolKind_ToString(SymbolKind value);

struct SymbolSignaturePart {
    String mName;
    String mModifiers;
    void *mUnk10;
    void *mUnk18;
    bool mUnk20;
};

struct SymbolLanguageInfo {
    const RTTI *mType;
    const char *mTypeName;
    void *mUnk10;
    void *mUnk18;
    Array<SymbolSignaturePart> mSignature;
    void *mUnk30;
    void *mUnk38;
    void *mUnk40;
    void *mUnk48;
    void *mUnk50;
    void *mUnk58;
    const char *mUnk60;
    const char *mUnk68;
};

struct ExportedSymbolMember {
    SymbolKind mKind;
    const RTTI &mType;
    const char *mNamespace;
    const char *mName;
    void *mUnk20;
    void *mUnk28;
    std::array<SymbolLanguageInfo, 3> mLanguageInfo;
};

class ExportedSymbolGroup {
public:
    ExportedSymbolGroup() = delete;

    [[nodiscard]] virtual const RTTI &GetRTTI() const = 0;

    virtual ~ExportedSymbolGroup() = 0;

    virtual void RegisterSymbols() = 0;

    bool mAlwaysExport;
    const char *mNamespace;
    Array<ExportedSymbolMember> mMembers;
    Array<const RTTI *> mDependencies;
};
