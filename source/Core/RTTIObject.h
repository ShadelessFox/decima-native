#pragma once

struct RTTI;

class RTTIObject {
public:
    virtual const RTTI *GetRTTI() const = 0;

    virtual ~RTTIObject() = 0;
};