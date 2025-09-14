#pragma once

#include <cstdint>

#include "Decima/Core/Filename.h"
#include "Decima/PCore/Array.h"
#include "Util/Offsets.h"

namespace FileSystem {
    struct FileEntry {
        Filename mFilename;
        uint64_t mFileSize{};
        uint64_t mCreationTime{};
        uint64_t mModificationTime{};
        bool mArchive{};
        bool mCompressed{};
        bool mHidden{};
        bool mReadOnly{};
        bool mSystem{};
    };

    inline void Find(const Filename &inFileSpec, Array<Filename> *outFileList, Array<Filename> *outDirectoryList) {
        Offsets::CallID<"FileSystem::Find", void(*)(const Filename &, void *, void *)>(inFileSpec, outFileList, outDirectoryList);
    }

    void Find(const Filename &inFileSpec, Array<FileEntry> *outFileList, Array<FileEntry> *outDirectoryList);
}
