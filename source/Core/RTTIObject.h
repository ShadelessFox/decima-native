#pragma once

struct RTTI;

class RTTIObject {
public:
    [[nodiscard]] virtual const RTTI *GetRTTI() const = 0;

    virtual ~RTTIObject() = 0;
};
