#pragma once

#include "Decima/Core/GGUUID.h"

template<typename T>
    requires (std::is_base_of_v<RTTIRefObject, T>)
struct Ref {
public:
    const T &operator*() const { return *mPtr; }

    const T *operator->() const { return mPtr; }

    T &operator*() { return *mPtr; }

    T *operator->() { return mPtr; }

private:
    T *mPtr;
};

template<typename T>
struct UUIDRef {
public:
    GGUUID ObjectUUID;
};