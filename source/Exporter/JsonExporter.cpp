#include "JsonExporter.h"

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

    JsonNameObject(ctx, name.data());
    JsonNameValueStr(ctx, "kind", kind.data());

    if (auto as_class = inType.AsCompound()) {
        JsonNameValueNum(ctx, "version", as_class->mVersion);
        JsonNameValueNum(ctx, "flags", as_class->mFlags);

        if (!as_class->MessageHandlers().empty()) {
            JsonNameArray(ctx, "messages");

            for (const auto &message: as_class->MessageHandlers()) {
                JsonValueStr(ctx, message.mMessage->Name().data());
            }

            JsonEndArray(ctx);
        }

        if (!as_class->Bases().empty()) {
            JsonNameArray(ctx, "bases");

            for (const auto &base: as_class->Bases()) {
                JsonBeginCompactObject(ctx);
                JsonNameValueStr(ctx, "type", base.mType->Name().data());
                JsonNameValueNum(ctx, "offset", base.mOffset);
                JsonEndCompactObject(ctx);
            }

            JsonEndArray(ctx);
        }

        if (!as_class->Attrs().empty()) {
            JsonNameArray(ctx, "attrs");

            for (auto &attr: as_class->Attrs()) {
                if (attr.mType == nullptr) {
                    JsonBeginCompactObject(ctx);
                    JsonNameValueStr(ctx, "category", attr.mName);
                    JsonEndCompactObject(ctx);
                    continue;
                }

                JsonBeginCompactObject(ctx);
                JsonNameValueStr(ctx, "name", attr.mName);
                JsonNameValueStr(ctx, "type", attr.mType->Name().data());
                JsonNameValueNum(ctx, "offset", attr.mOffset);
                JsonNameValueNum(ctx, "flags", attr.mFlags);
                if (attr.mMinValue)
                    JsonNameValueStr(ctx, "min", attr.mMinValue);
                if (attr.mMaxValue)
                    JsonNameValueStr(ctx, "max", attr.mMaxValue);
                if (attr.mGetter || attr.mSetter)
                    JsonNameValueBool(ctx, "property", 1);
                JsonEndCompactObject(ctx);
            }

            JsonEndArray(ctx);
        }
    } else if (auto as_enum = inType.AsEnum()) {
        JsonNameValueNum(ctx, "size", as_enum->mSize);
        JsonNameArray(ctx, "values");

        for (auto &value: as_enum->Values()) {
            JsonBeginCompactObject(ctx);
            JsonNameValueNum(ctx, "value", static_cast<int>(value.mValue));
            JsonNameValueStr(ctx, "name", value.mName);

            if (value.mAliases[0]) {
                JsonNameCompactArray(ctx, "alias");
                for (size_t j = 0; j < value.mAliases.size() && value.mAliases[j]; j++)
                    JsonValueStr(ctx, value.mAliases[j]);
                JsonEndArray(ctx);
            }

            JsonEndCompactObject(ctx);
        }

        JsonEndArray(ctx);
    } else if (auto as_atom = inType.AsAtom()) {
        JsonNameValueStr(ctx, "base_type", as_atom->mParentType->Name().data());
    } else if (auto as_container = inType.AsContainer()) {
        JsonNameValueStr(ctx, "type", as_container->mContainerType->mTypeName);
        JsonNameValueStr(ctx, "item_type", as_container->mItemType->Name().data());
    } else if (auto as_pointer = inType.AsPointer()) {
        JsonNameValueStr(ctx, "type", as_pointer->mPointerType->mTypeName);
        JsonNameValueStr(ctx, "item_type", as_pointer->mItemType->Name().data());
    }

    JsonEndObject(ctx);
}
