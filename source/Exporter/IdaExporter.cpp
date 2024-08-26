#include "IdaExporter.h"

#include <format>
#include <cassert>

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
            const auto container = reinterpret_cast<const RTTIContainer &>(inType);
            return std::format("{}_{}", container.mContainerType->mTypeName, IDATypeName(*container.mItemType));
        }
        case RTTIKind::Enum:
        case RTTIKind::EnumFlags:
            return reinterpret_cast<const RTTIEnum &>(inType).mTypeName;
        case RTTIKind::Compound:
            return reinterpret_cast<const RTTICompound &>(inType).mTypeName;
        case RTTIKind::POD:
            return std::format("POD{}", reinterpret_cast<const RTTIPod &>(inType).mSize);
        default:
            assert("Unreachable");
    }
}

void IdaExporter::Export(const std::span<const RTTI *> &inTypes) {
    fputs("#include <idc.idc>\n\nstatic main()\n{", mFile);

    for (const auto type: inTypes) {
        Export(*type);
    }

    fputs("}", mFile);
}

void IdaExporter::Export(const RTTI &inType) {
    constexpr auto rebase = [=](auto inPtr) {
        constexpr uintptr_t Base = 0;
        return reinterpret_cast<uintptr_t>(inPtr) - Base;
    };

    const auto type_name = IDATypeName(inType);
    const auto kind_name = IdaKindName(inType);

    fprintf(mFile, "\n\t// %s %s\n", inType.KindName().c_str(), inType.TypeName().c_str());
    fprintf(mFile, "\tset_name(%#llx, \"RTTI_%s\");\n", rebase(&inType), type_name.c_str());
    fprintf(mFile, "\tapply_type(%#llx, \"%s\");\n", rebase(&inType), kind_name.c_str());

    if (const auto as_class = inType.AsCompound()) {
        if (const auto bases = as_class->mBases) {
            const auto bases_count = as_class->mNumBases;
            fprintf(mFile, "\tdel_items(%#llx, DELIT_SIMPLE, %zu);\n", rebase(bases), bases_count * sizeof(RTTIBase));
            fprintf(mFile, "\tapply_type(%#llx, \"RTTIBase[%d]\");\n", rebase(bases), bases_count);
            fprintf(mFile, "\tset_name(%#llx, \"%s::sBases\");\n", rebase(bases), type_name.c_str());
        }

        if (const auto attrs = as_class->mAttrs) {
            const auto attrs_count = as_class->mNumAttrs;
            fprintf(mFile, "\tdel_items(%#llx, DELIT_SIMPLE, %zu);\n", rebase(attrs), attrs_count * sizeof(RTTIAttr));
            fprintf(mFile, "\tset_name(%#llx, \"%s::sAttrs\");\n", rebase(attrs), type_name.c_str());
            fprintf(mFile, "\tapply_type(%#llx, \"RTTIAttr[%d]\");\n", rebase(attrs), attrs_count);
        }

        if (const auto messages = as_class->mMessageHandlers) {
            const auto messages_count = as_class->mNumMessageHandlers;
            fprintf(mFile, "\tdel_items(%#llx, DELIT_SIMPLE, %zu);\n", rebase(messages), messages_count * sizeof(RTTIMessageHandler));
            fprintf(mFile, "\tset_name(%#llx, \"%s::sMessageHandlers\");\n", rebase(messages), type_name.c_str());
            fprintf(mFile, "\tapply_type(%#llx, \"RTTIMessageHandler[%d]\");\n", rebase(messages), messages_count);

            for (const auto& message_handler : as_class->MessageHandlers()) {
                const auto message_name = IDATypeName(*message_handler.mMessage);
                fprintf(mFile, "\tset_name(%#llx, \"%s::On%s\");\n", rebase(message_handler.mHandler), type_name.c_str(), message_name.c_str() + 3);
                fprintf(mFile, "\tapply_type(%#llx, \"__int64 __fastcall f(void* this, %s* ioMsg)\");\n", rebase(message_handler.mHandler), message_name.c_str());
            }
        }

        if (const auto message_order_entries = as_class->mMessageOrderEntries) {
            const auto message_order_entries_count = as_class->mNumMessageHandlers;
            fprintf(mFile, "\tdel_items(%#llx, DELIT_SIMPLE, %zu);\n", rebase(message_order_entries), message_order_entries_count * sizeof(RTTIMessageOrderEntry));
            fprintf(mFile, "\tset_name(%#llx, \"%s::sInheritedMessageHandlers\");\n", rebase(message_order_entries), type_name.c_str());
            fprintf(mFile, "\tapply_type(%#llx, \"RTTIInheritedMessageHandler[%d]\");\n", rebase(message_order_entries), message_order_entries_count);
        }

        if (const auto functions = as_class->mFunctions) {
            const auto functions_count = as_class->mNumFunctions;
            fprintf(mFile, "\tdel_items(%#llx, DELIT_SIMPLE, %zu);\n", rebase(functions), functions_count * sizeof(RTTIFunction));
            fprintf(mFile, "\tset_name(%#llx, \"%s::sFunctions\");\n", rebase(functions), type_name.c_str());
            fprintf(mFile, "\tapply_type(%#llx, \"RTTIFunction[%d]\");\n", rebase(functions), functions_count);

            for (const auto& function : as_class->Functions()) {
                fprintf(mFile, "\tset_name(%#llx, \"%s::%s\");\n", rebase(function.mFunction), type_name.c_str(), function.mName);
            }
        }

        if (as_class->mGetExportedSymbols) {
            fprintf(mFile, "\tset_name(%#llx, \"%s::GetExportedSymbols\");\n", rebase(as_class->mGetExportedSymbols), type_name.c_str());
        }
    }

    if (const auto as_enum = inType.AsEnum()) {
        if (const auto values = as_enum->mValues) {
            fprintf(mFile, "\tdel_items(%#llx, DELIT_SIMPLE, %zu);\n", rebase(values), as_enum->mNumValues * sizeof(RTTIValue));
            fprintf(mFile, "\tset_name(%#llx, \"%s::sValues\");\n", rebase(values), type_name.c_str());
            fprintf(mFile, "\tapply_type(%#llx, \"RTTIValue[%d]\");\n", rebase(values), as_enum->mNumValues);
        }
    }

    if (const auto as_container = inType.AsContainer(); as_container && !mContainerTypes.contains(as_container->mContainerType)) {
        mContainerTypes.emplace(as_container->mContainerType);
        fprintf(mFile, "\tset_name(%#llx, \"%s::sInfo\");\n", rebase(as_container->mContainerType), type_name.c_str());
        fprintf(mFile, "\tapply_type(%#llx, \"RTTIContainer::Data\");\n", rebase(as_container->mContainerType));
    }

    if (const auto as_pointer = inType.AsPointer(); as_pointer && !mPointerTypes.contains(as_pointer->mPointerType)) {
        mPointerTypes.emplace(as_pointer->mPointerType);
        fprintf(mFile, "\tset_name(%#llx, \"%s::sInfo\");\n", rebase(as_pointer->mPointerType), type_name.c_str());
        fprintf(mFile, "\tapply_type(%#llx, \"RTTIPointer::Data\");\n", rebase(as_pointer->mPointerType));
    }
}
