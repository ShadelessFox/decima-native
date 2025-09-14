#include "Exporter.h"

#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

Exporter::Exporter(const std::string &inPath) {
    auto path = fs::absolute(fs::path(inPath));
    auto parent = path.parent_path();
    fs::create_directories(parent);

    if (fopen_s(&mFile, path.string().c_str(), "w") != 0) {
        throw std::runtime_error("Failed to open file");
    }
}

Exporter::~Exporter() {
    fflush(mFile);
    fclose(mFile);
    mFile = nullptr;
}