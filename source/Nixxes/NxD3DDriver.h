#pragma once

#include "Util/Assert.h"

#include <d3d12.h>

class INxD3DDriver {
public:
    virtual ~INxD3DDriver() = 0;
};

class NxD3D12Driver : public INxD3DDriver {
public:
    uint8_t Unk8[0x30];
    ID3D12Device *Device;
    uint8_t Unk40[0xD8];
};

assert_size(NxD3D12Driver, 0x118);
assert_offset(NxD3D12Driver, Device, 0x38);
