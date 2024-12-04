#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>

class RTTIObject;

class RTTIRefObject;

#define ASSERT_SIZE(_Type, _Size) static_assert(sizeof(_Type) == (_Size), "sizeof(" # _Type ") == " # _Size)

enum class RTTIKind : uint8_t {
    Atom,
    Pointer,
    Container,
    Enum,
    Compound,
    EnumFlags,
    POD
};

enum RTTIFlags : uint8_t {
    RTTIFactory_Registered = 0x2,
    FactoryManager_Registered = 0x4
};

struct RTTIAtom;
struct RTTICompound;
struct RTTIEnum;
struct RTTIPointer;
struct RTTIContainer;

#pragma pack(push, 1)

struct RTTI {
    int32_t mId;
    RTTIKind mKind;
    RTTIFlags mFactoryFlags;

    [[nodiscard]] std::string BaseName() const;

    [[nodiscard]] std::string Name() const;

    [[nodiscard]] std::string KindName() const;

    [[nodiscard]] const RTTIAtom *AsAtom() const {
        return mKind == RTTIKind::Atom ? reinterpret_cast<const RTTIAtom *>(this) : nullptr;
    }

    [[nodiscard]] const RTTICompound *AsCompound() const {
        return mKind == RTTIKind::Compound ? reinterpret_cast<const RTTICompound *>(this) : nullptr;
    }

    [[nodiscard]] const RTTIEnum *AsEnum() const {
        return mKind == RTTIKind::Enum || mKind == RTTIKind::EnumFlags
                   ? reinterpret_cast<const RTTIEnum *>(this)
                   : nullptr;
    }

    [[nodiscard]] const RTTIPointer *AsPointer() const {
        return mKind == RTTIKind::Pointer ? reinterpret_cast<const RTTIPointer *>(this) : nullptr;
    }

    [[nodiscard]] const RTTIContainer *AsContainer() const {
        return mKind == RTTIKind::Container ? reinterpret_cast<const RTTIContainer *>(this) : nullptr;
    }
};

#pragma pack(pop)

ASSERT_SIZE(RTTI, 0x6);

struct RTTIAtom : RTTI {
    uint16_t mSize;
    uint8_t mAlignment;
    uint8_t mSimple;
    const char *mTypeName;
    const RTTIAtom *mParentType;
    const void *mFromString;
    const void *mToString;
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
    std::array<const char*, 3> mAliases;
};

ASSERT_SIZE(RTTIValue, 0x28);

struct RTTIEnum : RTTI {
    uint8_t mSize;
    uint16_t mNumValues;
    uint8_t mAlignment;
    const char *mTypeName;
    const RTTIValue *mValues;
    const RTTI *mRepresentationType;

    [[nodiscard]] auto Values() const { return std::span{mValues, mNumValues}; }
};

ASSERT_SIZE(RTTIEnum, 0x28);

struct RTTIBase {
    const RTTICompound *mType;
    uint32_t mOffset;
};

ASSERT_SIZE(RTTIBase, 0x10);

struct RTTIAttr {
    const RTTI *mType;
    uint16_t mOffset;
    uint16_t mFlags;
    const char *mName;
    const void *mGetter;
    const void *mSetter;
    const char *mMinValue;
    const char *mMaxValue;
};

ASSERT_SIZE(RTTIAttr, 0x38);

struct RTTIOrderedAttr : RTTIAttr {
    const RTTICompound *mParent;
    const char *mCategory;
};

ASSERT_SIZE(RTTIOrderedAttr, 0x48);

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

struct RTTIFunction {
    char mReturnType;
    const char* mName;
    const char* mArguments;
    const void* mFunction;
};

ASSERT_SIZE(RTTIFunction, 0x20);

struct RTTICompound : RTTI {
    uint8_t mNumBases;
    uint8_t mNumAttrs;
    uint8_t mNumFunctions;
    uint8_t mNumMessageHandlers;
    uint8_t mNumMessageOrderEntries;
    uint8_t _mPad0B[3];
    uint16_t mVersion;
    uint32_t mSize;
    uint16_t mAlignment;
    uint16_t mFlags;
    const void *mConstructor;
    const void *mDestructor;
    const void *mFromString;
    const void *mToString;
    const char *mTypeName;
    uint32_t mTypeNameCrc;
    const RTTI *mNextType;
    const RTTI *mPrevType;
    const RTTIBase *mBases;
    const RTTIAttr *mAttrs;
    const RTTIFunction *mFunctions;
    const RTTIMessageHandler *mMessageHandlers;
    const RTTIMessageOrderEntry *mMessageOrderEntries;
    const void *mGetExportedSymbols;
    const RTTI *mRepresentationType;
    const RTTIOrderedAttr *mOrderedAttrs;
    uint32_t mNumOrderedAttrs;
    RTTIMessageHandler mMsgReadBinary;
    uint32_t mMsgReadBinaryOffset;
    void *mUnkB8;

    [[nodiscard]] auto Bases() const { return std::span{mBases, mNumBases}; }

    [[nodiscard]] auto Attrs() const { return std::span{mAttrs, mNumAttrs}; }

    [[nodiscard]] auto Functions() const { return std::span{mFunctions, mNumFunctions}; }

    [[nodiscard]] auto MessageHandlers() const { return std::span{mMessageHandlers, mNumMessageHandlers}; }
};

ASSERT_SIZE(RTTICompound, 0xC0);

struct RTTIPointer : RTTI {
    struct Data {
        using pGetFunction = const RTTIRefObject *(*)(const RTTIPointer &inType, const void *inObject);
        using pSetFunction = void (*)(const RTTIPointer &inType, void *inObject, const RTTIRefObject *inValue);

        const char *mTypeName;
        uint32_t mSize;
        uint32_t mAlignment;
        const void *mConstructor;
        const void *mDestructor;
        pGetFunction mGetter;
        pSetFunction mSetter;
        const void *mCopier;
    };

    const RTTI *mItemType;
    const Data *mPointerType;
};

ASSERT_SIZE(RTTIPointer, 0x18);
ASSERT_SIZE(RTTIPointer::Data, 0x38);

struct RTTIContainer : RTTI {
    struct Data {
        const char *mTypeName;
        uint16_t mSize;
        uint8_t mAlignment;
        uint8_t mArray;
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

    const RTTI *mItemType;
    const Data *mContainerType;
};

ASSERT_SIZE(RTTIContainer, 0x18);
ASSERT_SIZE(RTTIContainer::Data, 0xC0);

struct RTTIPod : RTTI {
    uint32_t mSize;
};

ASSERT_SIZE(RTTIPod, 0x0C);
