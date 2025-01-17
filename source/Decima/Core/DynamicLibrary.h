#pragma once

#include "Decima/Core/Resource.h"

class VirtualRTTIRegistrationListener {
public:
    virtual ~VirtualRTTIRegistrationListener() = 0;
};

assert_size(VirtualRTTIRegistrationListener, 0x8);

class DynamicLibrary : public Resource, public VirtualRTTIRegistrationListener {
public:
    String Filename;
    Array<uint8_t> Data;
};

// assert_size(DynamicLibrary, 0x88);
