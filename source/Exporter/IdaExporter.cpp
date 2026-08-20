#include "IdaExporter.h"

#include "Util/Offsets.h"
#include "Decima/Core/ExportedSymbolGroup.h"

#include <format>
#include <print>
#include <cassert>

#include <Windows.h>

constexpr auto rebase = [](auto inPtr) {
    static auto Base = reinterpret_cast<uint64_t>(GetModuleHandleW(nullptr));
    return reinterpret_cast<uintptr_t>(inPtr) - Base;
};

[[nodiscard]] static std::string IdaKindName(const RTTI& inType) {
    switch (inType.mKind) {
        case RTTIKind::Atom:
            return "RTTIAtom";
        case RTTIKind::Pointer:
            return "RTTIPointer";
        case RTTIKind::Container:
            return "RTTIContainer";
        case RTTIKind::Enum:
        case RTTIKind::EnumFlags:
            return "RTTIEnum";
        case RTTIKind::BitSet:
            return "RTTIBitSet";
        case RTTIKind::Compound:
            return "RTTICompound";
        case RTTIKind::POD:
            return "RTTIPod";
        default:
            assert("Unreachable");
    }
}

[[nodiscard]] static std::string IDATypeName(const RTTI &inType) {
    switch (inType.mKind) {
        case RTTIKind::Atom:
            return reinterpret_cast<const RTTIAtom &>(inType).mTypeName;
        case RTTIKind::Pointer:
        case RTTIKind::Container: {
            const auto& container = reinterpret_cast<const RTTIContainer &>(inType);
            return std::format("{}_{}", container.mContainerType->mTypeName, IDATypeName(*container.mItemType));
        }
        case RTTIKind::Enum:
        case RTTIKind::EnumFlags:
            return reinterpret_cast<const RTTIEnum &>(inType).mTypeName;
        case RTTIKind::BitSet:
            return reinterpret_cast<const RTTIBitSet &>(inType).mTypeName;
        case RTTIKind::Compound:
            return reinterpret_cast<const RTTICompound &>(inType).mTypeName;
        case RTTIKind::POD:
            return std::format("POD{}", reinterpret_cast<const RTTIPod &>(inType).mSize);
        default:
            assert("Unreachable");
    }
}

void IdaExporter::Export(const std::span<const RTTI *> &inTypes) {
    fputs(R"(#include <idc.idc>

// Check if a function is unique by ensuring it's only referenced by [inType],
// while also allowing references from the ".pdata" and ".rdata" segments.
// Any other references make [inFunction] not unique.
static is_unique_function(inType, inFunction) {
    if (1)
        return 0;
    auto ref = get_first_dref_to(inFunction);
    while (ref != BADADDR) {
        auto seg = get_segm_name(ref);
        if (ref != inType && seg != ".pdata" && seg != ".rdata")
            return 0;
        ref = get_next_dref_to(inFunction, ref);
    }
    return 1;
}

static main() {
    auto NAME_FLAGS = SN_FORCE | SN_DELTAIL | SN_NOWARN;
)", mFile);

    for (const auto type: inTypes) {
        ExportDeclarations(*type);
    }

    fputs("\n", mFile);

    for (const auto type: inTypes) {
        ExportFunctions(*type);
    }

    if constexpr (false) {
        fprintf(mFile, "\t// Exported symbols\n");

        const auto& symbols = ExportedSymbols::Get();
        for (const auto& group : symbols.mGroups) {
            ExportSymbols(*group);
        }

        fprintf(mFile, "\t// Symbol Hashes\n");

        for (const auto& [symbol, hash] : symbols.mAllSymbols) {
            fprintf(mFile, "\t // %s hash=%#08x address=%#llx\n", symbol->mName, hash, rebase(symbol->mLanguage[0].mAddress));
        }
    }

    fputs("}", mFile);
}

void IdaExporter::ExportDeclarations(const RTTI &inType) {
    const auto type_name = IDATypeName(inType);
    const auto kind_name = IdaKindName(inType);

    fprintf(mFile, "\n\t// %s %s\n", inType.KindName().data(), inType.Name().data());
    fprintf(mFile, "\tset_name(%#llx, \"RTTI_%s\", NAME_FLAGS);\n", rebase(&inType), type_name.c_str());
    fprintf(mFile, "\tapply_type(%#llx, \"%s\");\n", rebase(&inType), kind_name.c_str());

    if (const auto as_class = inType.AsCompound()) {
        if (const auto bases = as_class->mBases) {
            const auto bases_count = as_class->mNumBases;
            fprintf(mFile, "\tdel_items(%#llx, DELIT_SIMPLE, %zu);\n", rebase(bases), bases_count * sizeof(RTTIBase));
            fprintf(mFile, "\tapply_type(%#llx, \"RTTIBase[%d]\");\n", rebase(bases), bases_count);
            fprintf(mFile, "\tset_name(%#llx, \"%s::sBases\", NAME_FLAGS);\n", rebase(bases), type_name.c_str());
        }

        if (const auto attrs = as_class->mAttrs) {
            const auto attrs_count = as_class->mNumAttrs;
            fprintf(mFile, "\tdel_items(%#llx, DELIT_SIMPLE, %zu);\n", rebase(attrs), attrs_count * sizeof(RTTIAttr));
            fprintf(mFile, "\tapply_type(%#llx, \"RTTIAttr[%d]\");\n", rebase(attrs), attrs_count);
            fprintf(mFile, "\tset_name(%#llx, \"%s::sAttrs\", NAME_FLAGS);\n", rebase(attrs), type_name.c_str());
        }

        if (const auto message_handlers = as_class->mMessageHandlers) {
            const auto message_handlers_count = as_class->mNumMessageHandlers;
            fprintf(mFile, "\tdel_items(%#llx, DELIT_SIMPLE, %zu);\n", rebase(message_handlers), message_handlers_count * sizeof(RTTIMessageHandler));
            fprintf(mFile, "\tapply_type(%#llx, \"RTTIMessageHandler[%d]\");\n", rebase(message_handlers), message_handlers_count);
            fprintf(mFile, "\tset_name(%#llx, \"%s::sMessageHandlers\", NAME_FLAGS);\n", rebase(message_handlers), type_name.c_str());
        }

        if (const auto message_order_entries = as_class->mMessageOrderEntries) {
            const auto message_order_entries_count = as_class->mNumMessageHandlers;
            fprintf(mFile, "\tdel_items(%#llx, DELIT_SIMPLE, %zu);\n", rebase(message_order_entries), message_order_entries_count * sizeof(RTTIMessageOrderEntry));
            fprintf(mFile, "\tapply_type(%#llx, \"RTTIMessageOrderEntry[%d]\");\n", rebase(message_order_entries), message_order_entries_count);
            fprintf(mFile, "\tset_name(%#llx, \"%s::sMessageOrderEntries\", NAME_FLAGS);\n", rebase(message_order_entries), type_name.c_str());
        }
    }

    if (const auto as_enum = inType.AsEnum()) {
        if (const auto values = as_enum->mValues) {
            fprintf(mFile, "\tdel_items(%#llx, DELIT_SIMPLE, %zu);\n", rebase(values), as_enum->mNumValues * sizeof(RTTIValue));
            fprintf(mFile, "\tapply_type(%#llx, \"RTTIValue[%d]\");\n", rebase(values), as_enum->mNumValues);
            fprintf(mFile, "\tset_name(%#llx, \"%s::sValues\", NAME_FLAGS);\n", rebase(values), type_name.c_str());
        }
    }

    if (const auto as_container = inType.AsContainer(); as_container && !mContainerTypes.contains(as_container->mContainerType)) {
        mContainerTypes.emplace(as_container->mContainerType);
        fprintf(mFile, "\tapply_type(%#llx, \"RTTIContainer::Data\");\n", rebase(as_container->mContainerType));
        fprintf(mFile, "\tset_name(%#llx, \"%s::sInfo\", NAME_FLAGS);\n", rebase(as_container->mContainerType), type_name.c_str());
    }

    if (const auto as_pointer = inType.AsPointer(); as_pointer && !mPointerTypes.contains(as_pointer->mPointerType)) {
        mPointerTypes.emplace(as_pointer->mPointerType);
        fprintf(mFile, "\tapply_type(%#llx, \"RTTIPointer::Data\");\n", rebase(as_pointer->mPointerType));
        fprintf(mFile, "\tset_name(%#llx, \"%s::sInfo\", NAME_FLAGS);\n", rebase(as_pointer->mPointerType), type_name.c_str());
    }
}

void IdaExporter::ExportFunctions(const RTTI &inType) {
    const auto type_name = IDATypeName(inType);

    if (const auto as_class = inType.AsCompound()) {
        if (as_class->mConstructor) {
            const auto pointer = rebase(as_class->mConstructor);
            fprintf(mFile, "\tif (is_unique_function(%#llx, %#llx)) {\n", rebase(&inType), pointer);
            fprintf(mFile, "\t\tapply_type(%#llx, \"void* __fastcall f(RTTI* inType, void* inObject)\");\n", pointer);
            fprintf(mFile, "\t\tset_name(%#llx, \"%s::Constructor\", NAME_FLAGS);\n", pointer, type_name.c_str());
            fprintf(mFile, "\t}\n");
        }

        if (as_class->mDestructor) {
            const auto pointer = rebase(as_class->mDestructor);
            fprintf(mFile, "\tif (is_unique_function(%#llx, %#llx)) {\n", rebase(&inType), pointer);
            fprintf(mFile, "\t\tapply_type(%#llx, \"void __fastcall f(RTTI* inType, void* inObject)\");\n", pointer);
            fprintf(mFile, "\t\tset_name(%#llx, \"%s::Destructor\", NAME_FLAGS);\n", pointer, type_name.c_str());
            fprintf(mFile, "\t}\n");
        }

        if (as_class->mGetExportedSymbols) {
            const auto pointer = rebase(as_class->mGetExportedSymbols);
            fprintf(mFile, "\tif (is_unique_function(%#llx, %#llx)) {\n", rebase(&inType), pointer);
            fprintf(mFile, "\t\tapply_type(%#llx, \"const RTTI* __fastcall f()\");\n", pointer);
            fprintf(mFile, "\t\tset_name(%#llx, \"%s::GetExportedSymbols\", NAME_FLAGS);\n", pointer, type_name.c_str());
            fprintf(mFile, "\t}\n");
        }

        for (const auto &attr: as_class->Attrs()) {
            if (attr.mType == nullptr)
                continue;
            if (attr.mGetter) {
                fprintf(mFile, "\tif (is_unique_function(%#llx, %#llx)) {\n", rebase(as_class->mAttrs), rebase(attr.mGetter));
                fprintf(mFile, "\t\tapply_type(%#llx, \"void* __fastcall f(void* this)\");\n", rebase(attr.mGetter));
                fprintf(mFile, "\t\tset_name(%#llx, \"%s::Get%s\", NAME_FLAGS);\n", rebase(attr.mGetter), type_name.c_str(), attr.mName);
                fprintf(mFile, "\t}\n");
            }
            if (attr.mSetter) {
                fprintf(mFile, "\tif (is_unique_function(%#llx, %#llx)) {\n", rebase(as_class->mAttrs), rebase(attr.mSetter));
                fprintf(mFile, "\t\tapply_type(%#llx, \"void __fastcall f(void* this, void* inValue)\");\n", rebase(attr.mSetter));
                fprintf(mFile, "\t\tset_name(%#llx, \"%s::Set%s\", NAME_FLAGS);\n", rebase(attr.mSetter), type_name.c_str(), attr.mName);
                fprintf(mFile, "\t}\n");
            }
        }

        for (const auto& message_handler : as_class->MessageHandlers()) {
            const auto msg_name = IDATypeName(*message_handler.mMessage);
            const auto msg_handler = rebase(message_handler.mHandler);
            fprintf(mFile, "\tif (is_unique_function(%#llx, %#llx)) {\n", rebase(as_class->mMessageHandlers), msg_handler);
            fprintf(mFile, "\t\tapply_type(%#llx, \"__int64 __fastcall f(void* this, %s* ioMsg)\");\n", msg_handler, msg_name.c_str());
            fprintf(mFile, "\t\tset_name(%#llx, \"%s::On%s\", NAME_FLAGS);\n", msg_handler, type_name.c_str(), msg_name.c_str() + 3);
            fprintf(mFile, "\t}\n");
        }
    }
}

void IdaExporter::ExportSymbols(const ExportedSymbolGroup &inGroup) {
    for (const auto &symbol: inGroup.mSymbols) {
        if (symbol.mKind != ExportedSymbol::Kind::Variable && symbol.mKind != ExportedSymbol::Kind::Function)
            continue;
        for (const auto &language: symbol.mLanguage) {
            if (!language.mName)
                break;
            if (symbol.mNamespace) {
                fprintf(mFile, "\tset_name(%#llx, \"%s::%s\", NAME_FLAGS);\n", rebase(language.mAddress), symbol.mNamespace, language.mName);
            } else {
                fprintf(mFile, "\tset_name(%#llx, \"%s\", NAME_FLAGS);\n", rebase(language.mAddress), language.mName);
            }
            if (symbol.mKind == ExportedSymbol::Kind::Function) {
                const auto &signature = language.mSignature;
                fprintf(mFile, "\tset_func_cmt(%#llx, \"%s%s", rebase(language.mAddress), signature[0].mName, strlen(signature[0].mModifiers) ? signature[0].mModifiers : " ");
                if (symbol.mNamespace)
                    fprintf(mFile, "%s::", symbol.mNamespace);
                fprintf(mFile, "%s(", language.mName);
                for (auto i = 1; i < signature.size(); i++) {
                    if (i > 1)
                        fprintf(mFile, ", ");
                    fprintf(mFile, "%s%s", signature[i].mName, signature[i].mModifiers);
                }
                fprintf(mFile, ");\", 1);\n");
            }
        }
    }
}


