#pragma once

#include "RTTIObject.h"
#include "GGUUID.h"

class RTTIRefObject : public RTTIObject {
public:
    virtual void RTTIRefObject_Unk01() = 0;

    virtual void RTTIRefObject_Unk02() = 0;

public:
    uint32_t mRefCount;
    uint32_t mPad0C;
    GGUUID mObjectUUID;
};

assert_size(RTTIRefObject, 32);
assert_offset(RTTIRefObject, mRefCount, 8);
assert_offset(RTTIRefObject, mObjectUUID, 16);