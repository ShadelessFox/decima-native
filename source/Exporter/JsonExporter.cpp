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
    if (const auto as_container = inType.AsContainer(); as_container) {
        if (mContainerTypes.contains(as_container->mContainerType->mTypeName))
            return;
        mContainerTypes.emplace(as_container->mContainerType->mTypeName);
    }

    if (const auto as_pointer = inType.AsPointer(); as_pointer) {
        if (mPointerTypes.contains(as_pointer->mPointerType->mTypeName))
            return;
        mPointerTypes.emplace(as_pointer->mPointerType->mTypeName);
    }

    auto name = inType.BaseName();
    auto kind = inType.KindName();
    auto ctx = inCtx;

    JsonNameObject(ctx, name.c_str());
    JsonNameValueStr(ctx, "kind", kind.c_str());

    if (auto as_class = inType.AsCompound(); as_class) {
        JsonNameValueNum(ctx, "version", as_class->mVersion);
        JsonNameValueNum(ctx, "flags", as_class->mFlags);

        if (!as_class->MessageHandlers().empty()) {
            JsonNameArray(ctx, "messages");

            for (const auto &message: as_class->MessageHandlers()) {
                auto message_type = message.mMessage->Name();
                JsonValueStr(ctx, message_type.c_str());
            }

            JsonEndArray(ctx);
        }

        if (!as_class->Bases().empty()) {
            JsonNameArray(ctx, "bases");

            for (const auto &base: as_class->Bases()) {
                auto base_type = base.mType->Name();

                JsonBeginCompactObject(ctx);
                JsonNameValueStr(ctx, "name", base_type.c_str());
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
                JsonNameValueStr(ctx, "type", attr.mType->Name().c_str());
                JsonNameValueNum(ctx, "offset", attr.mOffset);
                JsonNameValueNum(ctx, "flags", attr.mFlags);
                if (attr.mMinValue)
                    JsonNameValueStr(ctx, "min", attr.mMinValue);
                if (attr.mMaxValue)
                    JsonNameValueStr(ctx, "max", attr.mMaxValue);
                if (attr.mGetter && attr.mSetter)
                    JsonNameValueBool(ctx, "property", 1);
                JsonEndCompactObject(ctx);
            }

            JsonEndArray(ctx);
        }
    } else if (auto as_enum = inType.AsEnum(); as_enum) {
        JsonNameValueNum(ctx, "size", as_enum->mSize);
        JsonNameArray(ctx, "values");

        for (auto &value: as_enum->Values()) {
            JsonBeginCompactObject(ctx);
            JsonNameValueNum(ctx, "value", value.mValue);
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
    } else if (auto as_atom = inType.AsAtom(); as_atom) {
        auto base_type = as_atom->mParentType->Name();

        JsonNameValueStr(ctx, "base_type", base_type.c_str());
        // JsonNameValueBool(ctx, "simple", as_atom->mSimple);
    } else if (auto as_container = inType.AsContainer(); as_container) {
        const auto& container = *as_container->mContainerType;
        // JsonNameValueBool(ctx, "array", container.mArray);
        // if (!container.mConstructor)
        //     JsonNameValueNum(ctx, "size", container.mSize / container.mAlignment);
    }

    JsonEndObject(ctx);
}
