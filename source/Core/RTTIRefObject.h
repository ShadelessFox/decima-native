#pragma once

#include "RTTIObject.h"
#include "GGUUID.h"

class RTTIRefObject : public RTTIObject {
public:
    virtual void RTTIRefObject_Unk01() = 0;

    virtual void RTTIRefObject_Unk02() = 0;

public:
    uint32_t mRefCount;
    GGUUID mObjectUUID;
};
