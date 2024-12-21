#pragma once

#include "Exporter.h"

class ExportedSymbolGroup;
struct JsonContext;

class SymbolsExporter final : public Exporter {
public:
    explicit SymbolsExporter(const std::string &inPath) : Exporter(inPath + ".json") {
    }

    void Export(const std::span<const RTTI *> &inTypes) override;
private:
    static void Export(const ExportedSymbolGroup& group, JsonContext* ctx);
};
