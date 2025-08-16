#pragma once

#include "Decima/Core/RTTIRefObject.h"

#include <Util/Assert.h>

class Module : public RTTIRefObject {
public:
    uint32_t mPauseRequests;
};

assert_size(Module, 0x28);
