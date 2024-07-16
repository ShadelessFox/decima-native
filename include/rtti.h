#ifndef DECIMA_NATIVE_RTTI_H
#define DECIMA_NATIVE_RTTI_H

#ifdef RTTI_STANDALONE

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#endif

#ifdef RTTI_STANDALONE
enum RTTIKind {
#else
    enum RTTIKind : __int8 {
#endif
    RTTIKind_Atom = 0,
    RTTIKind_Pointer = 1,
    RTTIKind_Container = 2,
    RTTIKind_Enum = 3,
    RTTIKind_Compound = 4,
    RTTIKind_EnumFlags = 5,
    RTTIKind_POD = 6,
    RTTIKind_EnumBitSet = 7,
};

#pragma pack(push, 1)
struct RTTI {
    uint32_t id;
#ifdef RTTI_STANDALONE
    uint8_t kind;
#else
    enum RTTIKind kind;
#endif
    uint8_t rtti_factory_flags;
};
#pragma pack(pop)

struct RTTIBase {
    struct RTTI *mType;
    uint32_t mOffset;
};

struct RTTIAttr {
    struct RTTI *type;
    uint16_t mOffset;
    uint16_t mFlags;
    const char *mName;
    const void *mGetter;
    const void *mSetter;
    const char *mMinValue;
    const char *mMaxValue;
};

struct RTTIMessageHandler {
    struct RTTI *mMessage;
    void *mHandler;
};

struct RTTIMessageOrderEntry {
    uint32_t mBefore;
    struct RTTI *mMessage;
    struct RTTI *mCompound;
};

struct RTTICompound {
    struct RTTI base;
    uint8_t mNumBases;
    uint8_t mNumAttrs;
    uint8_t mNumMessageHandlers;
    uint8_t mNumMessageOrderEntries;
    uint8_t unk_09;
    uint32_t mVersion;
    uint32_t mSize;
    uint16_t mAlignment;
    uint16_t mFlags;
    void *mConstructor;
    void *mDestructor;
    void *mFromString;
    void *mFromStringSlice;
    void *mToString;
    const char *mTypeName;
    struct RTTI *previous_type;
    struct RTTI *next_type;
    struct RTTIBase *mBases;
    struct RTTIAttr *mAttrs;
    struct RTTIMessageHandler *mMessageHandlers;
    struct RTTIMessageOrderEntry *mMessageOrderEntries;
    void *mGetExportedSymbols;

    struct RTTI *representation_type;
    uintptr_t unk_88;
    uintptr_t unk_90;
    uintptr_t unk_98;
    void *on_read_msg_binary_fn;
    uint32_t vtable_offset;
    uint32_t unk_AC;
};

struct RTTIContainerData {
    const char *mTypeName;
    uint16_t mSize;
    uint8_t mAlignment;
    uint8_t unk_0B[5];
    void *constructor_fn;
    void *destructor_fn;
    void *unk_20;
    void *unk_28;
    void *unk_30;
    void *get_num_items_fn;
    void *unk_40;
    void *unk_48;
    void *unk_50;
    void *unk_58;
    void *unk_60;
    void *unk_68;
    void *unk_70;
};

struct RTTIContainer {
    struct RTTI base;
    uint16_t unk_06;
    struct RTTI *mItemType;
    struct RTTIContainerData *mContainerType;
    const char *mTypeName;
};

struct RTTIPointerData {
    const char *mTypeName;
    uint32_t mSize;
    uint8_t mAlignment;
    uint8_t unk_0D[3];
    void *constructor_fn;
    void *destructor_fn;
    void *getter_fn;
    void *setter_fn;
    void *copier_fn;
};

struct RTTIPointer {
    struct RTTI base;
    uint8_t unk_06[2];
    struct RTTI *mItemType;
    struct RTTIPointerData *mPointerType;
    const char *mTypeName;
};

struct RTTIValue {
    uint64_t mValue;
    const char *mName;
    const char *mAliases[4];
};

struct RTTIEnum {
    struct RTTI base;
    uint8_t size;
    uint8_t alignment;
    uint16_t num_values;
    uint8_t unk_0A[6];
    const char *type_name;
    struct RTTIValue *values;
    struct RTTI *representation_type;
};

struct RTTIAtom {
    struct RTTI base;
    uint16_t mSize;
    uint8_t mAlignment;
    uint8_t mSimple; ///< A mSimple atom is one with a fixed memory layout, such as int32, bool, etc.
    uint8_t unk_08[6];
    const char *mTypeName;
    struct RTTI *mBaseType;
    void *from_string_fn;
    void *to_string_fn;
    void *unk_30;
    void *assign_fn;
    void *equals_fn;
    void *constructor_fn;
    void *destructor_fn;
    void *swap_endian_fn;
    void *try_assign;
    void *get_size_in_memory_fn;
    void *compare_strings_fn;
    struct RTTI *representation_type;
};


#ifdef RTTI_STANDALONE

static_assert(sizeof(struct RTTI) == 0x6, "sizeof(struct RTTI) == 0x5");
static_assert(sizeof(struct RTTIBase) == 0x10, "sizeof(struct RTTIBase) == 0x10");
static_assert(sizeof(struct RTTIAttr) == 0x38, "sizeof(struct RTTIAttr) == 0x38");
static_assert(sizeof(struct RTTIMessageHandler) == 0x10, "sizeof(struct RTTIMessageHandler) == 0x10");
static_assert(sizeof(struct RTTICompound) == 0xB0, "sizeof(struct RTTICompound) == 0xB0");
static_assert(sizeof(struct RTTIContainerData) == 0x78, "sizeof(struct RTTIContainerData) == 0x78");
static_assert(sizeof(struct RTTIContainer) == 0x20, "sizeof(struct RTTIContainer) == 0x20");
static_assert(sizeof(struct RTTIPointerData) == 0x38, "sizeof(struct RTTIPointerData) == 0x38");
static_assert(sizeof(struct RTTIPointer) == 0x20, "sizeof(struct RTTIPointer) == 0x20");
static_assert(sizeof(struct RTTIValue) == 0x30, "sizeof(struct RTTIValue) == 0x30");
static_assert(sizeof(struct RTTIEnum) == 0x28, "sizeof(struct RTTIEnum) == 0x28");
static_assert(sizeof(struct RTTIAtom) == 0x80, "sizeof(struct RTTIAtom) == 0x80");

const char *RTTIKind_Name(enum RTTIKind);

const char *RTTI_Name(struct RTTI *);

const char *RTTI_DisplayName(struct RTTI *);

_Bool RTTI_AsCompound(struct RTTI *, struct RTTICompound **);

_Bool RTTI_AsContainer(struct RTTI *, struct RTTIContainer **);

_Bool RTTI_AsPointer(struct RTTI *, struct RTTIPointer **);

_Bool RTTI_AsEnum(struct RTTI *, struct RTTIEnum **);

_Bool RTTI_AsAtom(struct RTTI *, struct RTTIAtom **);

#endif

#endif //DECIMA_NATIVE_RTTI_H
