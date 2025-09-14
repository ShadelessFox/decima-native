#include "IdaExporter.h"

#include "Decima/Core/ExportedSymbolGroup.h"
#include "Decima/Core/FactoryManager.h"

#include <format>

[[nodiscard]] static std::string IdaKindName(const RTTI &inType) {
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
        case RTTIKind::Compound:
            return "RTTICompound";
        case RTTIKind::POD:
            return "RTTIPod";
        default:
            throw std::runtime_error("Unreachable");
    }
}

[[nodiscard]] static std::string IDATypeName(const RTTI &inType) {
    switch (inType.mKind) {
        case RTTIKind::Atom:
            return reinterpret_cast<const RTTIAtom &>(inType).mTypeName;
        case RTTIKind::Pointer:
        case RTTIKind::Container: {
            const auto &container = reinterpret_cast<const RTTIContainer &>(inType);
            return std::format("{}_{}", container.mContainerType->mTypeName, IDATypeName(*container.mItemType));
        }
        case RTTIKind::Enum:
        case RTTIKind::EnumFlags:
            return reinterpret_cast<const RTTIEnum &>(inType).mTypeName;
        case RTTIKind::Compound:
            return reinterpret_cast<const RTTICompound &>(inType).mTypeName;
        default:
            throw std::runtime_error("Unreachable");
    }
}

void IdaExporter::Export(const std::span<const RTTI *> &inTypes) {
    fputs(R"(#include <idc.idc>

static main() {
    auto NAME_FLAGS = SN_FORCE | SN_DELTAIL | SN_NOWARN;
)", mFile);

    for (const auto type: inTypes) {
        ExportDeclarations(*type);
    }

    fputs("\n", mFile);

    // for (const auto type: inTypes) {
    //     ExportFunctions(*type);
    // }


    const auto &symbols = ExportedSymbols::Get();

    fprintf(mFile, "\n\t// Exported symbols\n");
    for (const auto &group: symbols.mGroups) {
        ExportSymbols(*group);
    }

    fprintf(mFile, "\n\t// Symbol Hashes\n");
    for (const auto &[hash, symbol]: symbols.mAllSymbols) {
        fprintf(mFile, "\t// %s hash=0x%08x address=0x%p\n", symbol->mName, hash, symbol->mLanguage[0].mAddress);
    }

    fputs("}", mFile);
}

void IdaExporter::ExportDeclarations(const RTTI &inType) {
    const auto type_name = IDATypeName(inType);
    const auto kind_name = IdaKindName(inType);

    fprintf(mFile, "\n\t// %s %s\n", IdaKindName(inType).c_str(), IDATypeName(inType).c_str());
    fprintf(mFile, "\tset_name(0x%p, \"RTTI_%s\", NAME_FLAGS);\n", &inType, type_name.c_str());
    fprintf(mFile, "\tapply_type(0x%p, \"%s\");\n", &inType, kind_name.c_str());

    if (const auto as_compound = inType.AsCompound()) {
        if (const auto bases = as_compound->mBases) {
            const auto bases_count = as_compound->mNumBases;
            fprintf(mFile, "\tdel_items(0x%p, DELIT_SIMPLE, %zu);\n", bases, bases_count * sizeof(RTTIBase));
            fprintf(mFile, "\tapply_type(0x%p, \"RTTIBase[%d]\");\n", bases, bases_count);
            fprintf(mFile, "\tset_name(0x%p, \"%s::sBases\", NAME_FLAGS);\n", bases, type_name.c_str());
        }

        if (const auto attrs = as_compound->mAttrs) {
            const auto attrs_count = as_compound->mNumAttrs;
            fprintf(mFile, "\tdel_items(0x%p, DELIT_SIMPLE, %zu);\n", attrs, attrs_count * sizeof(RTTIAttr));
            fprintf(mFile, "\tapply_type(0x%p, \"RTTIAttr[%d]\");\n", attrs, attrs_count);
            fprintf(mFile, "\tset_name(0x%p, \"%s::sAttrs\", NAME_FLAGS);\n", attrs, type_name.c_str());
        }

        if (const auto message_handlers = as_compound->mMessageHandlers) {
            const auto message_handlers_count = as_compound->mNumMessageHandlers;
            fprintf(mFile, "\tdel_items(0x%p, DELIT_SIMPLE, %zu);\n", message_handlers,
                    message_handlers_count * sizeof(RTTIMessageHandler));
            fprintf(mFile, "\tapply_type(0x%p, \"RTTIMessageHandler[%d]\");\n", message_handlers, message_handlers_count);
            fprintf(mFile, "\tset_name(0x%p, \"%s::sMessageHandlers\", NAME_FLAGS);\n", message_handlers, type_name.c_str());
        }

        if (const auto message_order_entries = as_compound->mMessageOrderEntries) {
            const auto message_order_entries_count = as_compound->mNumMessageHandlers;
            fprintf(mFile, "\tdel_items(0x%p, DELIT_SIMPLE, %zu);\n", message_order_entries,
                    message_order_entries_count * sizeof(RTTIMessageOrderEntry));
            fprintf(mFile, "\tapply_type(0x%p, \"RTTIMessageOrderEntry[%d]\");\n", message_order_entries, message_order_entries_count);
            fprintf(mFile, "\tset_name(0x%p, \"%s::sMessageOrderEntries\", NAME_FLAGS);\n", message_order_entries, type_name.c_str());
        }
    }

    if (const auto as_enum = inType.AsEnum()) {
        if (const auto values = as_enum->mValues) {
            fprintf(mFile, "\tset_name(0x%p, \"%s::sValues\", NAME_FLAGS);\n", values, type_name.c_str());
            fprintf(mFile, "\tdel_items(0x%p, DELIT_SIMPLE, %zu);\n", values, as_enum->mNumValues * sizeof(RTTIValue));
            fprintf(mFile, "\tapply_type(0x%p, \"RTTIValue[%d]\");\n", values, as_enum->mNumValues);
        }
    }

    // Pointer and container types are shared
    // if (const auto as_container = inType.AsContainer()) {
    //     fprintf(mFile, "\tset_name(0x%p, \"%s::sInfo\", NAME_FLAGS);\n", as_container->mContainerType, type_name.c_str());
    //     fprintf(mFile, "\tapply_type(0x%p, \"RTTIContainer::Data\");\n", as_container->mContainerType);
    // }
    //
    // if (const auto as_pointer = inType.AsPointer()) {
    //     fprintf(mFile, "\tset_name(0x%p, \"%s::sInfo\", NAME_FLAGS);\n", as_pointer->mPointerType, type_name.c_str());
    //     fprintf(mFile, "\tapply_type(0x%p, \"RTTIPointer::Data\");\n", as_pointer->mPointerType);
    // }
}

void IdaExporter::ExportFunctions(const RTTI &inType) {
    const auto type_name = IDATypeName(inType);

    if (const auto as_class = inType.AsCompound()) {
        if (as_class->mConstructor) {
            const auto pointer = as_class->mConstructor;
            fprintf(mFile, "\tif (is_unique_function(0x%p, 0x%p)) {\n", &inType, pointer);
            fprintf(mFile, "\t\tapply_type(0x%p, \"void* __fastcall f(RTTI* inType, void* inObject)\");\n", pointer);
            fprintf(mFile, "\t\tset_name(0x%p, \"%s::Constructor\", NAME_FLAGS);\n", pointer, type_name.c_str());
            fprintf(mFile, "\t}\n");
        }

        if (as_class->mDestructor) {
            const auto pointer = as_class->mDestructor;
            fprintf(mFile, "\tif (is_unique_function(0x%p, 0x%p)) {\n", &inType, pointer);
            fprintf(mFile, "\t\tapply_type(0x%p, \"void __fastcall f(RTTI* inType, void* inObject)\");\n", pointer);
            fprintf(mFile, "\t\tset_name(0x%p, \"%s::Destructor\", NAME_FLAGS);\n", pointer, type_name.c_str());
            fprintf(mFile, "\t}\n");
        }

        if (as_class->mGetExportedSymbols) {
            const auto pointer = as_class->mGetExportedSymbols;
            fprintf(mFile, "\tif (is_unique_function(0x%p, 0x%p)) {\n", &inType, pointer);
            fprintf(mFile, "\t\tapply_type(0x%p, \"const RTTI* __fastcall f()\");\n", pointer);
            fprintf(mFile, "\t\tset_name(0x%p, \"%s::GetExportedSymbols\", NAME_FLAGS);\n", pointer, type_name.c_str());
            fprintf(mFile, "\t}\n");
        }

        for (const auto &attr: as_class->Attrs()) {
            if (attr.mType == nullptr)
                continue;
            if (attr.mGetter) {
                fprintf(mFile, "\tif (is_unique_function(0x%p, 0x%p)) {\n", as_class->mAttrs, attr.mGetter);
                fprintf(mFile, "\t\tapply_type(0x%p, \"void* __fastcall f(void* this)\");\n", attr.mGetter);
                fprintf(mFile, "\t\tset_name(0x%p, \"%s::Get%s\", NAME_FLAGS);\n", attr.mGetter, type_name.c_str(), attr.mName);
                fprintf(mFile, "\t}\n");
            }
            if (attr.mSetter) {
                fprintf(mFile, "\tif (is_unique_function(0x%p, 0x%p)) {\n", as_class->mAttrs, attr.mSetter);
                fprintf(mFile, "\t\tapply_type(0x%p, \"void __fastcall f(void* this, void* inValue)\");\n", attr.mSetter);
                fprintf(mFile, "\t\tset_name(0x%p, \"%s::Set%s\", NAME_FLAGS);\n", attr.mSetter, type_name.c_str(), attr.mName);
                fprintf(mFile, "\t}\n");
            }
        }

        for (const auto &message_handler: as_class->MessageHandlers()) {
            const auto msg_name = IDATypeName(*message_handler.mMessage);
            const auto msg_handler = message_handler.mHandler;
            fprintf(mFile, "\tif (is_unique_function(0x%p, 0x%p)) {\n", as_class->mMessageHandlers, msg_handler);
            fprintf(mFile, "\t\tapply_type(0x%p, \"__int64 __fastcall f(void* this, %s* ioMsg)\");\n", msg_handler, msg_name.c_str());
            fprintf(mFile, "\t\tset_name(0x%p, \"%s::On%s\", NAME_FLAGS);\n", msg_handler, type_name.c_str(), msg_name.c_str() + 3);
            fprintf(mFile, "\t}\n");
        }
    }
}

void IdaExporter::ExportSymbols(const ExportedSymbolGroup &inGroup) {
    constexpr auto format_signature = [](auto signature) {
        std::string buffer;
        for (const auto &part: signature) {
            std::format_to(std::back_inserter(buffer), "{}{}{}", buffer.empty() ? "" : ", ", part.mName, part.mModifiers);
        }
        return buffer;
    };

    for (const auto &symbol: inGroup.mSymbols) {
        switch (symbol.mKind) {
            case ExportedSymbol::Kind::Atom:
                break;
            case ExportedSymbol::Kind::Enum:
                break;
            case ExportedSymbol::Kind::Class:
                break;
            case ExportedSymbol::Kind::Struct:
                break;
            case ExportedSymbol::Kind::Typedef:
                break;
            case ExportedSymbol::Kind::Function:
                break;
            case ExportedSymbol::Kind::Variable:
                break;
            case ExportedSymbol::Kind::Container:
                break;
            case ExportedSymbol::Kind::Reference:
                break;
            default:
                throw std::runtime_error("Unexpected symbol type");
        }

        if (symbol.mKind != ExportedSymbol::Kind::Variable && symbol.mKind != ExportedSymbol::Kind::Function)
            continue;

        // Process only the first language entry. Others often contain duplicates. Not sure what their purpose is
        for (const auto &language: std::span{symbol.mLanguage, 1}) {
            if (!language.mName)
                break;
            if (symbol.mNamespace) {
                fprintf(mFile, "\tset_name(0x%p, \"%s::%s\", NAME_FLAGS);\n", language.mAddress, symbol.mNamespace, language.mName);
            } else {
                fprintf(mFile, "\tset_name(0x%p, \"%s\", NAME_FLAGS);\n", language.mAddress, language.mName);
            }

            const auto &signature = language.mSignature;
            if (symbol.mKind == ExportedSymbol::Kind::Function) {
                const auto return_type = format_signature(std::span{signature.begin(), 1});
                const auto parameter_types = format_signature(std::span{signature.begin() + 1, signature.end()});

                fprintf(mFile, "\tset_func_cmt(0x%p, \"%s ", language.mAddress, return_type.c_str());
                if (symbol.mNamespace)
                    fprintf(mFile, "%s::", symbol.mNamespace);
                fprintf(mFile, "%s(", language.mName);
                fprintf(mFile, "%s", parameter_types.c_str());
                fprintf(mFile, ");\", 1);\n");
            } else if (symbol.mKind == ExportedSymbol::Kind::Variable) {
                const auto variable_type = format_signature(std::span{language.mSignature});

                fprintf(mFile, "\tupdate_extra_cmt(0x%p, E_PREV, \"; %s\");\n", language.mAddress, variable_type.c_str());
            }
        }
    }
}
