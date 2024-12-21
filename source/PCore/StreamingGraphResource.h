#pragma once

#include "Core/RTTIRefObject.h"
#include "PCore/Array.h"

class StreamingDataSourceLocator final {
public:
    uint64_t Data;
};

class StreamingSourceSpan final {
public:
    uint32_t FileIndexAndIsPatch;
    uint32_t Length;
    uint32_t Offset;
};

class StreamingObjectLocator final {
public:
    GGUUID ObjectUUID;
    uint16_t TypeIndex;
    uint16_t Reserved;
    uint32_t FileIndex;
    uint32_t Offset;
    uint32_t Length;
};

class StreamingGraphResource : public RTTIRefObject {
public:
    StreamingGraphResource() = delete;

    bool IsPacked; // 32
    Array<uint64_t> TypeHashes; // 40
    Array<const RTTI *> TypePtrs; // 56
    Array<uint8_t> TypeTableData; // 72
    uint8_t Unk58[52]; // 88
    uint64_t LinkTableID; // 144
    uint32_t LinkTableSize; // 152
    Array<StreamingDataSourceLocator> LocatorTable; // 160
    Array<uint32_t> ArrayTable; // 176
    Array<StreamingSourceSpan> SpanTable; // 192
    uint8_t UnkD0[16]; // 208
    Array<uint32_t> SubGroups; // 224
    Array<GGUUID> RootUUIDs; // 240
    Array<uint32_t> RootIndices; // 256
    uint8_t Unk110[80]; // 272
    Array<Array<uint32_t>> PackFileOffsets; // 352
    Array<Array<uint32_t>> PackFileLengths; // 368
    Array<StreamingObjectLocator> ObjectLocators; // 384
    uint32_t PackFileUncompressedBlockSize; // 400
    uint32_t PackFileMaxCompressedBlockSize; // 404
    uint8_t Unk198[8]; // 408
};
