#pragma once

#include "Decima/Core/RTTI.h"
#include "Decima/PCore/HashMap.h"

class FactoryManager {
public:
    static FactoryManager &Get() {
        return **Offsets::ResolveID<"FactoryManager::Instance", FactoryManager **>();
    }

    FactoryManager() = delete;

    virtual ~FactoryManager() = 0;

    virtual void Register(const RTTI &) = 0;

    virtual void Unregister(const RTTI &) = 0;

    auto &Types() const { return mAllTypes; }

public:
    HashSet<RTTI *> mAllTypes;
    HashMap<uint32_t, RTTI *> mPodTypes;
    // ...
};
