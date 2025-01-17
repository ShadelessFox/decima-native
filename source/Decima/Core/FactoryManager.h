#pragma once

#include "Decima/Core/RTTI.h"
#include "Decima/PCore/HashMap.h"
#include "Decima/PCore/Lock.h"

#include "Util/Noncopyable.h"

struct RTTI;

class FactoryManager {
    DECIMA_MAKE_NONCOPYABLE(FactoryManager);
    DECIMA_MAKE_NONMOVABLE(FactoryManager);

public:
    static FactoryManager &Get() {
        return **Offsets::ResolveID<"FactoryManager::Instance", FactoryManager **>();
    }

    FactoryManager() = delete;
    virtual ~FactoryManager() = 0;

    virtual void Register(const RTTI &) = 0;

    virtual void Unregister(const RTTI &) = 0;

    [[nodiscard]] const RTTI& Find(const std::string_view& inName) const {
        auto result = std::find_if(mTypes.begin(), mTypes.end(), [&](auto& inType) { return inType->Name() == inName; });
        if (result == mTypes.end())
            throw std::invalid_argument(std::format("Unknown RTTI type '{}'", inName));
        return **result;
    }

    const auto& Types() const { return mTypes; }

private:
    HashSet<pcRTTI> mTypes;
    HashMap<RTTIPod*, uint32_t> mPodTypes;
    SharedLockProtected<HashSet<pcRTTI>> mUnk28;
    HashMap<RTTIPointer::Data *, HashMap<pcRTTI, pcRTTI>> mPointerTypes;
    HashMap<pcRTTI, pVoid> mUnk50;
    SharedLockProtected<Array<pVoid>> mUnk60;
};

assert_size(FactoryManager, 0x78);