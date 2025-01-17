#pragma once

class WeakPtrTarget {
public:
    virtual ~WeakPtrTarget() = 0;
};

class WeakPtrRTTITarget : public WeakPtrTarget {
public:
    pVoid WeakPtrList;
};

assert_size(WeakPtrRTTITarget, 0x10);
