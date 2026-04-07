#include "AttrExporter.h"

#include <print>

void AttrExporter::Export(const std::span<const RTTI *> &inTypes) {
    for (auto &type: inTypes) {
        Export(*type);
    }
}

void AttrExporter::Export(const RTTI &inType) {
    const auto cls = inType.AsCompound();
    if (!cls)
        return;
    fprintf(mFile, "class : '%s'\n", cls->mTypeName);
    for (auto &attr: cls->OrderedAttrs())
        fprintf(mFile, " attr : '%s' '%s' ('%s')\n", attr.mType->Name().data(), attr.mName, attr.mGroup);
    fprintf(mFile, "\n");
}
