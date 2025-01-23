#pragma once

#include "Nixxes/NxD3DDriver.h"

#include "Util/Assert.h"
#include "Util/Offsets.h"

#include <d3d12.h>

class INxD3D {
public:
    virtual ~INxD3D() = 0;
};

class NxD3DImpl : public INxD3D {
public:
    static NxD3DImpl *Instance() {
        return *Offsets::ResolveID<"NxD3DImpl::Instance", NxD3DImpl **>();
    }

    // @formatter:off
    virtual void Initialize() = 0;
    virtual void INxD3D_Unk10() = 0;
    virtual void INxD3D_Unk18() = 0;
    virtual void INxD3D_Unk20() = 0;
    virtual void INxD3D_Unk28() = 0;
    virtual void INxD3D_Unk30() = 0;
    virtual void INxD3D_Unk38() = 0;
    virtual void INxD3D_Unk40() = 0;
    virtual void INxD3D_Unk48() = 0;
    virtual void Present() = 0;
    virtual void INxD3D_Unk58() = 0;
    virtual void INxD3D_Unk60() = 0;
    virtual void INxD3D_Unk68() = 0;
    virtual void INxD3D_Unk70() = 0;
    virtual void INxD3D_Unk78() = 0;
    virtual void INxD3D_Unk80() = 0;
    virtual void INxD3D_Unk88() = 0;
    virtual void INxD3D_Unk90() = 0;
    virtual void INxD3D_Unk98() = 0;
    virtual void INxD3D_UnkA0() = 0;
    virtual void INxD3D_UnkA8() = 0;
    virtual void INxD3D_UnkB0() = 0;
    virtual void INxD3D_UnkB8() = 0;
    virtual void INxD3D_UnkC0() = 0;
    virtual void SetCanCreateDx11_1_Device() = 0;
    virtual void GetBackBufferCount() = 0;
    virtual void INxD3D_UnkD8() = 0;
    virtual void INxD3D_UnkE0() = 0;
    virtual void INxD3D_UnkE8() = 0;
    virtual void INxD3D_UnkF0() = 0;
    virtual void INxD3D_UnkF8() = 0;
    virtual void INxD3D_Unk100() = 0;
    virtual ID3D12CommandQueue *GetCommandQueue(uint32_t index) = 0;
    virtual void INxD3D_Unk110() = 0;
    virtual void INxD3D_Unk118() = 0;
    virtual void INxD3D_Unk120() = 0;
    virtual void INxD3D_Unk128() = 0;
    virtual void INxD3D_Unk130() = 0;
    virtual void INxD3D_Unk138() = 0;
    virtual void INxD3D_Unk140() = 0;
    virtual void INxD3D_Unk148() = 0;
    virtual void INxD3D_Unk150() = 0;
    virtual void INxD3D_Unk158() = 0;
    virtual void INxD3D_Unk160() = 0;
    virtual void INxD3D_Unk168() = 0;
    virtual void INxD3D_Unk170() = 0;
    virtual void INxD3D_Unk178() = 0;
    virtual void INxD3D_Unk180() = 0;
    virtual void CanCreateDx11_1_Device() = 0;
    virtual void INxD3D_Unk190() = 0;
    virtual void INxD3D_Unk198() = 0;
    virtual void INxD3D_Unk1A0() = 0;
    virtual void INxD3D_Unk1A8() = 0;
    virtual void INxD3D_Unk1B0() = 0;
    // @formatter:on

    uint8_t Unk8[0x178];
    NxD3D12Driver *Driver;
    uint8_t Unk188[0x78];
};

assert_size(NxD3DImpl, 0x200);
assert_offset(NxD3DImpl, Driver, 0x180);
