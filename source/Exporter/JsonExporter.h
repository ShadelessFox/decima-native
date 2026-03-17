#pragma once

#include "Exporter.h"
#include "Util/json.h"

class JsonExporter final : public Exporter {
public:
    explicit JsonExporter(const std::string &inPath) : Exporter(inPath + ".json") {
    }

    void Export(const std::span<const RTTI *> &inTypes) override;

private:
    void Export(const RTTI &inType, JsonContext *inCtx);
};