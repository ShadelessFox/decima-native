#pragma once

#include "Array.h"

class MemoryPool {
public:
    static MemoryPool &Instance() {
        return **Offsets::ResolveID<"MemoryPool::Instance", MemoryPool **>();
    }

    MemoryPool() = delete;

    virtual ~MemoryPool() = 0;

    virtual void *Alloc(size_t inSize) = 0;

    virtual void Free(Array<void *> &inPtrs) = 0;

    virtual void Free(void *inPtr) = 0;

    virtual void *Realloc(void *inPtr, size_t inSize) = 0;

    virtual void *AlignedAlloc(size_t inSize) = 0;

    virtual size_t GetSize(void *inPtr) = 0;
};
