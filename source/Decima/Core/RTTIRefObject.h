#pragma once

#include <Windows.h>

#include "Decima/Core/RTTIObject.h"
#include "Decima/PCore/UUID.h"
#include "Util/Offsets.h"

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
