#pragma once

#include "Util/Assert.h"

class SharedLock final {
public:
private:
    SRWLOCK mLock{};
};

assert_size(SharedLock, 0x8);

template<typename T>
class SharedLockProtected final {
public:
    auto operator->() { return &mValue; }

    auto operator->() const { return &mValue; }

private:
    SRWLOCK mLock{};
    T mValue{};
};
