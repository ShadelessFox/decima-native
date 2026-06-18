#include "JsonSymbolExporter.h"

#include "Util/json.h"

#include <Windows.h>

SymbolExporter::SymbolExporter(const std::string &inPath) : mFile(std::fopen(inPath.c_str(), "w")) {
    if (!mFile)
        throw std::runtime_error("Failed to open file for writing: " + inPath);
}

void SymbolExporter::ExportSymbols() {
    JsonContext json{};
    JsonInit(&json, mFile);
    JsonBeginArray(&json);

    for (auto &group: ExportedSymbols::Get().mGroups) {
        ExportGroup(*group, &json);
    }

    JsonEndArray(&json);
}

void SymbolExporter::ExportGroup(const ExportedSymbolGroup &group, JsonContext *json) {
    JsonBeginObject(json);
    JsonNameValueStr(json, "type", group.GetRTTI()->Name());
    JsonNameValueStr(json, "namespace", group.mNamespace ? group.mNamespace : "");

    JsonNameArray(json, "symbols");
    for (auto &symbol: group.mSymbols) {
        ExportSymbol(group, symbol, json);
    }
    JsonEndArray(json);

    JsonEndObject(json);
}

void JsonSymbolExporter::ExportSymbol(const ExportedSymbolGroup &group, const ExportedSymbol &symbol, JsonContext *json) {
    if (symbol.mNamespace && strcmp(group.mNamespace, symbol.mNamespace) != 0)
        __debugbreak();

    auto base = reinterpret_cast<uint64_t>(GetModuleHandleW(nullptr));

    JsonBeginObject(json);
    JsonNameValueStr(json, "name", symbol.mName);
    JsonNameValueStr(json, "kind", std::to_string(symbol.mKind));

    JsonNameArray(json, "language");
    for (const auto &language: symbol.mLanguage) {
        if (!language.mName)
            break;

        JsonBeginObject(json);
        JsonNameValueStr(json, "name", language.mName);

        if (language.mAddress)
            JsonNameValueStr(json, "address", std::format("{:#x}", reinterpret_cast<uint64_t>(language.mAddress) - base));

        JsonNameArray(json, "signature");
        for (const auto &signature: language.mSignature) {
            JsonBeginCompactObject(json);
            JsonNameValueStr(json, "name", signature.mName);
            JsonNameValueStr(json, "modifiers", signature.mModifiers);
            JsonEndCompactObject(json);
        }
        JsonEndArray(json);

        JsonEndObject(json);
    }
    JsonEndArray(json);

    JsonEndObject(json);
}
