#pragma once

#include "Util/Assert.h"

struct RTTI;

class RTTIObject {
public:
    [[nodiscard]] virtual const RTTI *GetRTTI() const = 0;

    virtual ~RTTIObject() = 0;
};

assert_size(RTTIObject, 8);
