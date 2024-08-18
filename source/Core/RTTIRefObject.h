#pragma once

#include "Core/RTTIObject.h"
#include "PCore/UUID.h"
#include "Offsets.h"

class RTTIRefObject : public RTTIObject {
public:
    GGUUID mObjectUUID;

    void IncrementRef() {
        _InterlockedExchangeAdd(&mRefCount, 1);
    }

    void DecrementRef() {
        Offsets::CallID<"RTTIRefObject::DecrementRef", void (*)(RTTIRefObject*)>(this);
    }
private:
    uint32_t mRefCount;
};
