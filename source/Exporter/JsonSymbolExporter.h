#pragma once

#include "Decima/Core/ExportedSymbolGroup.h"
#include "Util/json.h"

#include <string>
#include <cstdio>

class SymbolExporter {
public:
    explicit SymbolExporter(const std::string &inPath);

    virtual ~SymbolExporter() = default;

    void ExportSymbols();

protected:
    virtual void ExportGroup(const ExportedSymbolGroup &group, JsonContext *json);

    virtual void ExportSymbol(const ExportedSymbolGroup &group, const ExportedSymbol &symbol, JsonContext *json) = 0;

private:
    FILE *mFile{nullptr};
};

class JsonSymbolExporter : public SymbolExporter {
public:
    explicit JsonSymbolExporter(const std::string &inPath) : SymbolExporter(inPath) {
    }

    ~JsonSymbolExporter() override = default;

protected:
    void ExportSymbol(const ExportedSymbolGroup &group, const ExportedSymbol &symbol, JsonContext *json) override;
};
