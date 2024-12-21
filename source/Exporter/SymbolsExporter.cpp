#include "SymbolsExporter.h"


#include "Core/ExportedSymbolGroup.h"
#include "Util/Offsets.h"
#include "Util/json.h"

void SymbolsExporter::Export(const std::span<const RTTI *> &inTypes) {
    const auto &groups = *Offsets::ResolveID<"Symbols::sExportedSymbolGroups", Array<ExportedSymbolGroup *> *>();

    JsonContext ctx{};
    JsonInit(&ctx, mFile);
    JsonBeginObject(&ctx);

    for (const auto &group: groups) {
        Export(*group, &ctx);
    }

    JsonEndObject(&ctx);
}

void SymbolsExporter::Export(const ExportedSymbolGroup &group, JsonContext *ctx) {
    auto group_type = group.GetRTTI().Name();
    JsonNameObject(ctx, group_type.c_str());

    if (group.mAlwaysExport)
        JsonNameValueBool(ctx, "alwaysExported", group.mAlwaysExport);

    if (!group.mMembers.empty()) {
        JsonNameArray(ctx, "members");
        for (const auto &member: group.mMembers) {
            JsonBeginObject(ctx);
            JsonNameValueStr(ctx, "name", member.mName);
            JsonNameValueStr(ctx, "namespace", member.mNamespace);
            JsonNameValueStr(ctx, "kind", SymbolKind_ToString(member.mKind));
            JsonNameValueNum(ctx, "unk20", reinterpret_cast<uint64_t>(member.mUnk20));
            JsonNameValueNum(ctx, "unk28", reinterpret_cast<uint64_t>(member.mUnk28));
            JsonNameArray(ctx, "language");
            for (const auto &language: member.mLanguageInfo) {
                if (language.mTypeName == nullptr)
                    break;

                JsonBeginObject(ctx);
                JsonNameValueStr(ctx, "typename", language.mTypeName);
                JsonNameValueNum(ctx, "unk10", reinterpret_cast<uint64_t>(language.mUnk10));
                JsonNameValueNum(ctx, "unk18", reinterpret_cast<uint64_t>(language.mUnk18));
                JsonNameValueNum(ctx, "unk30", reinterpret_cast<uint64_t>(language.mUnk30));
                JsonNameValueNum(ctx, "unk38", reinterpret_cast<uint64_t>(language.mUnk38));
                JsonNameValueNum(ctx, "unk40", reinterpret_cast<uint64_t>(language.mUnk40));
                JsonNameValueNum(ctx, "unk48", reinterpret_cast<uint64_t>(language.mUnk48));
                JsonNameValueNum(ctx, "unk50", reinterpret_cast<uint64_t>(language.mUnk50));
                JsonNameValueNum(ctx, "unk58", reinterpret_cast<uint64_t>(language.mUnk58));
                JsonNameValueStr(ctx, "unk60", language.mUnk60);
                JsonNameValueStr(ctx, "unk68", language.mUnk68);

                JsonNameCompactArray(ctx, "signature");
                for (const auto &part: language.mSignature) {
                    JsonBeginObject(ctx);
                    JsonNameValueStr(ctx, "name", part.mName.c_str());
                    JsonNameValueStr(ctx, "modifiers", part.mModifiers.c_str());
                    JsonNameValueNum(ctx, "unk10", reinterpret_cast<uint64_t>(part.mUnk10));
                    JsonNameValueNum(ctx, "unk18", reinterpret_cast<uint64_t>(part.mUnk18));
                    JsonNameValueBool(ctx, "unk20", part.mUnk20);
                    JsonEndObject(ctx);
                }
                JsonEndCompactArray(ctx);

                JsonEndObject(ctx);
            }
            JsonEndArray(ctx);
            JsonEndObject(ctx);
        }
        JsonEndArray(ctx);
    }

    if (!group.mDependencies.empty()) {
        JsonNameArray(ctx, "dependencies");
        for (const auto &dependency: group.mDependencies) {
            auto type_name = dependency->Name();
            JsonValueStr(ctx, type_name.c_str());
        }
        JsonEndArray(ctx);
    }

    JsonEndObject(ctx);
}
