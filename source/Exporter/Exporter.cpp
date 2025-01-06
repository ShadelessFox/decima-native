#include "Exporter.h"

#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

Exporter::Exporter(const std::string &inPath) {
    fs::create_directories(fs::path(inPath).parent_path());

    if (fopen_s(&mFile, inPath.c_str(), "w") != 0) {
        throw std::runtime_error("Failed to open file");
    }
}

Exporter::~Exporter() {
    fflush(mFile);
    fclose(mFile);
    mFile = nullptr;
}
