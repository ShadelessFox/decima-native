#pragma once

#include "Exporter.h"

#include <set>

class IdaExporter final : public Exporter {
public:
    explicit IdaExporter(const std::string &inPath) : Exporter(inPath + ".idc") {
    }

    void Export(const std::span<const RTTI *> &inTypes) override;

private:
    void ExportDeclarations(const RTTI &inType);
    void ExportFunctions(const RTTI &inType);

    std::set<const RTTIContainer::Data *> mContainerTypes;
    std::set<const RTTIPointer::Data *> mPointerTypes;
};
