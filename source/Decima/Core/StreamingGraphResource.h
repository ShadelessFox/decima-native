#pragma once

#include "Decima/Core/RTTIRefObject.h"
#include "Decima/PCore/Array.h"

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

    bool IsPacked;
    Array<uint64_t> TypeHashes;
    Array<const RTTI *> TypePtrs;
    Array<uint8_t> TypeTableData;
    uint8_t Unk58[52];
    uint64_t LinkTableID;
    uint32_t LinkTableSize;
    Array<StreamingDataSourceLocator> LocatorTable;
    Array<uint32_t> ArrayTable;
    Array<StreamingSourceSpan> SpanTable;
    uint8_t UnkD0[16];
    Array<uint32_t> SubGroups;
    Array<GGUUID> RootUUIDs;
    Array<uint32_t> RootIndices;
    uint8_t Unk110[80];
    Array<Array<uint32_t>> PackFileOffsets;
    Array<Array<uint32_t>> PackFileLengths;
    Array<StreamingObjectLocator> ObjectLocators;
    uint32_t PackFileUncompressedBlockSize;
    uint32_t PackFileMaxCompressedBlockSize;
    uint8_t Unk198[8];
};

assert_size(StreamingGraphResource, 416);
assert_offset(StreamingGraphResource, IsPacked, 32);
assert_offset(StreamingGraphResource, TypeHashes, 40);
assert_offset(StreamingGraphResource, TypePtrs, 56);
assert_offset(StreamingGraphResource, TypeTableData, 72);
assert_offset(StreamingGraphResource, LinkTableID, 144);
assert_offset(StreamingGraphResource, LinkTableSize, 152);
assert_offset(StreamingGraphResource, LocatorTable, 160);
assert_offset(StreamingGraphResource, ArrayTable, 176);
assert_offset(StreamingGraphResource, SpanTable, 192);
assert_offset(StreamingGraphResource, SubGroups, 224);
assert_offset(StreamingGraphResource, RootUUIDs, 240);
assert_offset(StreamingGraphResource, RootIndices, 256);
assert_offset(StreamingGraphResource, PackFileOffsets, 352);
assert_offset(StreamingGraphResource, PackFileLengths, 368);
assert_offset(StreamingGraphResource, ObjectLocators, 384);
assert_offset(StreamingGraphResource, PackFileUncompressedBlockSize, 400);
assert_offset(StreamingGraphResource, PackFileMaxCompressedBlockSize, 404);
