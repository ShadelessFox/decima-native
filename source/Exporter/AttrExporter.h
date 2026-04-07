#pragma once

#include "Exporter.h"
#include "Util/json.h"

class AttrExporter final : public Exporter {
public:
    explicit AttrExporter(const std::string &inPath) : Exporter(inPath + ".attr") {
    }

    void Export(const std::span<const RTTI *> &inTypes) override;

private:
    void Export(const RTTI &inType);
};