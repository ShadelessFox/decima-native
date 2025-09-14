#pragma once

#include "RTTI.h"

class RTTIObject {
public:
    virtual const RTTIClass *GetRTTI() const = 0;

    virtual ~RTTIObject() = 0;

    template<typename T>
    T &Get(std::string_view inName) {
        return GetRTTI()->GetAttrRefUnsafe<T>(*this, inName);
    }

    template<typename T>
    const T &Get(std::string_view inName) const {
        return GetRTTI()->GetAttrRefUnsafe<T>(*this, inName);
    }

    bool Set(std::string_view inName, const String& inValue) {
        const RTTIAttr* attr;
        auto& object = GetRTTI()->GetAttrRefUnsafe<uintptr_t>(*this, inName, &attr);
        return attr->mType->FromString(reinterpret_cast<void *>(&object), inValue);
    }
};