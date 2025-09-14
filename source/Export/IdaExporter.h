#pragma once

#include "Exporter.h"

#include <set>

class ExportedSymbolGroup;

class IdaExporter final : public Exporter {
public:
    explicit IdaExporter(const std::string &inPath) : Exporter(inPath + ".idc") {
    }

    void Export(const std::span<const RTTI *> &) override;

private:
    void ExportDeclarations(const RTTI &);

    void ExportFunctions(const RTTI &);

    void ExportSymbols(const ExportedSymbolGroup &);
};
