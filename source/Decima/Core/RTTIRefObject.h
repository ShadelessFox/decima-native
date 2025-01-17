#pragma once

#include "RTTIObject.h"
#include "GGUUID.h"

class RTTIRefObject : public RTTIObject {
public:
    virtual void RTTIRefObject_Unk01() = 0;

    virtual void RTTIRefObject_Unk02() = 0;

public:
    GGUUID ObjectUUID;
    uint32_t RefCount;
};

assert_size(RTTIRefObject, 32);