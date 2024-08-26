#include "Exporter.h"

Exporter::Exporter(const std::string &inPath) {
    fopen_s(&mFile, inPath.c_str(), "w");
}

Exporter::~Exporter() {
    fflush(mFile);
    fclose(mFile);
    mFile = nullptr;
}
