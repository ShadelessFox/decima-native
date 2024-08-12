#pragma once

#include <cstdint>
#include <span>
#include <system_error>

#include "PCore/String.h"
#include "Core/RTTIObject.h"

#define ASSERT_SIZE(_Type, _Size) static_assert(sizeof(_Type) == (_Size), "sizeof(" # _Type ") == " # _Size)

using pFromStringFunction = bool (*)(const String &inString, void *outData);
using pToStringFunction = bool (*)(const void *inData, String &outString);

enum class RTTIKind : std::uint8_t {
    Atom = 0,
    Pointer = 1,
    Container = 2,
    Enum = 3,
    Compound = 4,
    EnumFlags = 5,
    POD = 6
};

#pragma pack(push, 1)

struct RTTIAtom;
struct RTTIClass;
struct RTTIEnum;
struct RTTIPointer;
struct RTTIContainer;

struct RTTI {
    std::uint32_t mId;
    RTTIKind mKind;
    std::uint8_t mFactoryFlags;

    [[nodiscard]] String GetName() const;

    [[nodiscard]] String ToString(const void *value) const;

    [[nodiscard]] const RTTIAtom* AsAtom() const {
        return mKind == RTTIKind::Atom ? reinterpret_cast<const RTTIAtom*>(this) : nullptr;
    }

    [[nodiscard]] const RTTIClass* AsClass() const {
        return mKind == RTTIKind::Compound ? reinterpret_cast<const RTTIClass*>(this) : nullptr;
    }

    [[nodiscard]] const RTTIEnum* AsEnum() const {
        return mKind == RTTIKind::Enum || mKind == RTTIKind::EnumFlags ? reinterpret_cast<const RTTIEnum*>(this) : nullptr;
    }

    [[nodiscard]] const RTTIPointer* AsPointer() const {
        return mKind == RTTIKind::Pointer ? reinterpret_cast<const RTTIPointer*>(this) : nullptr;
    }

    [[nodiscard]] const RTTIContainer* AsContainer() const {
        return mKind == RTTIKind::Container ? reinterpret_cast<const RTTIContainer*>(this) : nullptr;
    }
};

#pragma pack(pop)

ASSERT_SIZE(RTTI, 0x6);

struct RTTIAtom : RTTI {
    uint16_t mSize;
    uint8_t mAlignment;
    uint8_t mSimple;
    const char *mName;
    const RTTIAtom *mBaseType;
    pFromStringFunction mFromString;
    pToStringFunction mToString;
    const void *mCopy;
    const void *mEquals;
    const void *mConstructor;
    const void *mDestructor;
    const void *mAssignWithEndian;
    const void *mAssign;
    const void *mGetSize;
    const void *mCompareStrings;
    const RTTI *mRepresentationType;
};

ASSERT_SIZE(RTTIAtom, 0x78);

struct RTTIValue {
    uint32_t mValue;
    const char *mName;
    const char *mAliases[3];
};

ASSERT_SIZE(RTTIValue, 0x28);

struct RTTIEnum : RTTI {
    uint8_t mSize;
    uint16_t mNumValues;
    uint8_t mAlignment;
    const char *mTypeName;
    const RTTIValue *mValues;
    const RTTI *mRepresentationType;
};

ASSERT_SIZE(RTTIEnum, 0x28);

struct RTTIBase {
    const RTTI *mType;
    uint64_t mOffset;
};

ASSERT_SIZE(RTTIBase, 0x10);

struct RTTIAttr {
    using pGetterFunction = void (*)(const void *inObject, void *inValue);
    using pSetterFunction = void (*)(void *inObject, const void *inValue);

    const RTTI *mType;
    uint16_t mOffset;
    uint16_t mFlags;
    const char *mName;
    pGetterFunction mGetter;
    pSetterFunction mSetter;
    const char *mMinValue;
    const char *mMaxValue;

    template<typename T>
    bool Get(const RTTIObject &inObject, T &outValue) const {
        if (mType == nullptr)
            return false;
        auto offset = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(&inObject) + mOffset);
        if (mGetter != nullptr) {
            mGetter(&inObject, &outValue);
        } else {
            outValue = reinterpret_cast<std::add_const_t<T> *>(offset);
        }
        return true;
    }

    template<typename T>
    T* GetRef(const RTTIObject &inObject) const {
        if (mType == nullptr)
            throw std::runtime_error("Can't obtain a reference to a category marker");
        if (mGetter != nullptr)
            throw std::runtime_error("Can't obtain a reference to a property");
        auto offset = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(&inObject) + mOffset);
        return reinterpret_cast<T *>(offset);
    }

    template<typename T>
    void Set(RTTIObject &inObject, const T &inValue) {
        if (mType == nullptr)
            return;
        auto offset = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(&inObject) + mOffset);
        if (mSetter != nullptr) {
            mSetter(&inObject, inValue);
        } else {
            *reinterpret_cast<T>(offset) = inValue;
        }
    }
};

ASSERT_SIZE(RTTIAttr, 0x38);

struct RTTIMessageHandler {
    const RTTI *mMessage;
    const void *mHandler;
};

ASSERT_SIZE(RTTIMessageHandler, 0x10);

struct RTTIMessageOrderEntry {
    uint32_t mBefore;
    const RTTI *mMessage;
    const RTTI *mCompound;
};

ASSERT_SIZE(RTTIMessageOrderEntry, 0x18);

struct RTTIClass : RTTI {
    uint8_t mNumBases;
    uint8_t mNumAttrs;
    uint8_t mNumFunctions;
    uint8_t mNumMessageHandlers;
    uint8_t mNumMessageOrderEntries;
    uint8_t _mPad0B[3];
    uint16_t mVersion;
    uint8_t _mPad10[4];
    uint32_t mSize;
    uint16_t mAlignment;
    uint16_t mFlags;
    const void *mConstructor;
    const void *mDestructor;
    pFromStringFunction mFromString;
    pToStringFunction mToString;
    const char *mTypeName;
    uint32_t mTypeNameCrc;
    const RTTI *mNextType;
    const RTTI *mPrevType;
    const RTTIBase *mBases;
    const RTTIAttr *mAttrs;
    const void *mFunctions;
    const RTTIMessageHandler *mMessageHandlers;
    const RTTIMessageOrderEntry *mMessageOrderEntries;
    const void *mGetExportedSymbols;
    const RTTI *mRepresentationType;

    [[nodiscard]] auto GetBases() const { return std::span{mBases, mNumBases}; }

    [[nodiscard]] auto GetAttrs() const { return std::span{mAttrs, mNumAttrs}; }
};

ASSERT_SIZE(RTTIClass, 0x98);

struct RTTIPointerInfo {
    const char *mName;
    uint32_t mSize;
    uint32_t mAlignment;
    const void *mConstructor;
    const void *mDestructor;
    const void *mGetter;
    const void *mSetter;
    const void *mCopier;
};

ASSERT_SIZE(RTTIPointerInfo, 0x38);

struct RTTIPointer : RTTI {
    const RTTI *mItemType;
    const RTTIPointerInfo *mPointerType;
};

ASSERT_SIZE(RTTIPointer, 0x18);

struct RTTIContainerInfo {
    const char *mName;
    uint16_t mSize;
    uint8_t mAlignment;
    const void *mConstructor;
    const void *mDestructor;
    const void *mResize;
    const void *mInsert;
    const void *mRemove;
    const void *mGetSize;
    const void *mGetItem;
    const void *mUnk48;
    const void *mUnk50;
    const void *mUnk58;
    const void *mUnk60;
    const void *mUnk68;
    const void *mUnk70;
    const void *mUnk78;
    const void *mUnk80;
    const void *mUnk88;
    const void *mUnk90;
    const void *mToString;
    const void *mFromString;
    const void *mUnkA8;
    const void *mUnkB0;
    const void *mUnkB8;
};

ASSERT_SIZE(RTTIContainerInfo, 0xC0);

struct RTTIContainer : RTTI {
    const RTTI *mItemType;
    const RTTIContainerInfo *mContainerInfo;
};

ASSERT_SIZE(RTTIContainer, 0x18);