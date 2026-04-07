#include "JsonExporter.h"

#include <print>

using namespace std::string_view_literals;

void JsonExporter::Export(const std::span<const RTTI *> &inTypes) {
    JsonContext ctx{};
    JsonInit(&ctx, mFile);
    JsonBeginObject(&ctx);

    for (auto &type: inTypes) {
        Export(*type, &ctx);
    }

    JsonEndObject(&ctx);
}

void JsonExporter::Export(const RTTI &inType, JsonContext *inCtx) {
    auto name = inType.Name();
    auto kind = inType.KindName();
    auto ctx = inCtx;

    JsonNameObject(ctx, name);
    JsonNameValueStr(ctx, "kind"sv, kind);

    if (auto as_class = inType.AsCompound()) {
        JsonNameValueNum(ctx, "version"sv, as_class->mVersion);
        JsonNameValueNum(ctx, "flags"sv, as_class->mFlags);

        if (!as_class->MessageHandlers().empty()) {
            JsonNameArray(ctx, "messages"sv);

            for (const auto &message: as_class->MessageHandlers()) {
                JsonValueStr(ctx, message.mMessage->Name());
            }

            JsonEndArray(ctx);
        }

        if (!as_class->Bases().empty()) {
            JsonNameArray(ctx, "bases"sv);

            for (const auto &base: as_class->Bases()) {
                JsonBeginCompactObject(ctx);
                JsonNameValueStr(ctx, "type"sv, base.mType->Name());
                JsonNameValueNum(ctx, "offset"sv, base.mOffset);
                JsonEndCompactObject(ctx);
            }

            JsonEndArray(ctx);
        }

        if (!as_class->Attrs().empty()) {
            JsonNameArray(ctx, "attrs"sv);

            for (auto &attr: as_class->Attrs()) {
                if (attr.mType == nullptr) {
                    JsonBeginCompactObject(ctx);
                    JsonNameValueStr(ctx, "category"sv, attr.mName);
                    JsonEndCompactObject(ctx);
                    continue;
                }

                JsonBeginCompactObject(ctx);
                JsonNameValueStr(ctx, "name"sv, attr.mName);
                JsonNameValueStr(ctx, "type"sv, attr.mType->Name());
                JsonNameValueNum(ctx, "offset"sv, attr.mOffset);
                JsonNameValueNum(ctx, "flags"sv, attr.mFlags);
                if (attr.mMinValue)
                    JsonNameValueStr(ctx, "min"sv, attr.mMinValue);
                if (attr.mMaxValue)
                    JsonNameValueStr(ctx, "max"sv, attr.mMaxValue);
                if (attr.mGetter || attr.mSetter)
                    JsonNameValueBool(ctx, "property"sv, 1);
                JsonEndCompactObject(ctx);
            }

            JsonEndArray(ctx);
        }
    } else if (auto as_enum = inType.AsEnum()) {
        JsonNameValueNum(ctx, "size"sv, as_enum->mSize);
        JsonNameArray(ctx, "values");

        for (auto &value: as_enum->Values()) {
            JsonBeginCompactObject(ctx);
            JsonNameValueNum(ctx, "value"sv, static_cast<int>(value.mValue));
            JsonNameValueStr(ctx, "name"sv, value.mName);

            if (value.mAliases[0]) {
                JsonNameCompactArray(ctx, "alias");
                for (size_t j = 0; j < value.mAliases.size() && value.mAliases[j]; j++)
                    JsonValueStr(ctx, value.mAliases[j]);
                JsonEndArray(ctx);
            }

            JsonEndCompactObject(ctx);
        }

        JsonEndArray(ctx);
    } else if (auto as_bitset = inType.AsBitSet()) {
        JsonNameValueNum(ctx, "size"sv, as_bitset->mSize);
        JsonNameValueStr(ctx, "type"sv, as_bitset->mRepresentationType->Name());
    } else if (auto as_atom = inType.AsAtom()) {
        JsonNameValueStr(ctx, "base_type"sv, as_atom->mParentType->Name());
    } else if (auto as_container = inType.AsContainer()) {
        JsonNameValueStr(ctx, "type"sv, as_container->mContainerType->mTypeName);
        JsonNameValueStr(ctx, "item_type"sv, as_container->mItemType->Name());
    } else if (auto as_pointer = inType.AsPointer()) {
        JsonNameValueStr(ctx, "type"sv, as_pointer->mPointerType->mTypeName);
        JsonNameValueStr(ctx, "item_type"sv, as_pointer->mItemType->Name());
    }

    JsonEndObject(ctx);
}
