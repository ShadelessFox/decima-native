#pragma once

#include <string>
#include <span>

#include "Decima/Core/RTTI.h"

class Exporter {
public:
    explicit Exporter(const std::string &inPath);

    virtual ~Exporter();

    virtual void Export(const std::span<const RTTI *> &inTypes) = 0;
protected:
    FILE* mFile{};
};