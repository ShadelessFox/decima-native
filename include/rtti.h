#ifndef DECIMA_NATIVE_RTTI_H
#define DECIMA_NATIVE_RTTI_H

#ifdef RTTI_STANDALONE

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#endif

#include "rtti_types.h"

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
    struct RTTI *type;
    uint32_t offset;
};

struct RTTIAttr {
    struct RTTI *type;
    uint16_t offset;
    uint16_t flags;
    const char *name;
    const void *property_get_fn;
    const void *property_set_fn;
    const char *min_value;
    const char *max_value;
};

struct RTTIMessageHandler {
    struct RTTI *message;
    void *handler;
};

struct RTTICompound {
    struct RTTI base;
    uint8_t bases_count;
    uint8_t attrs_count;
    uint8_t message_handlers_count;
    uint8_t unk_08[2];
    uint32_t version;
    uint32_t size;
    uint16_t alignment;
    uint16_t flags;

    void (*mConstructor)(struct RTTICompound *, struct RTTIObject *);
    void (*mDestructor)(struct RTTICompound *, struct RTTIObject *);
    _Bool (*mFromString)(struct RTTIObject *, STRING_DATA);
    _Bool (*mFromStringSlice)(struct RTTIObject *, struct StringSlice *);
    _Bool (*mToString)(struct RTTIObject *, STRING_DATA);

    STRING_DATA type_name;
    struct RTTI *previous_type;
    struct RTTI *next_type;
    struct RTTIBase *bases;
    struct RTTIAttr *attrs;
    struct RTTIMessageHandler *message_handlers;
    uintptr_t unk_70;
    struct RTTICompound *(*mGetExportedSymbols)();
    struct RTTI *representation_type;
    uintptr_t unk_88;
    uintptr_t unk_90;
    uintptr_t unk_98;
    void *on_read_msg_binary_fn;
    uint32_t vtable_offset;
    uint32_t unk_AC;
};

struct RTTIContainerData {
    STRING_DATA type_name;
    uint16_t size;
    uint8_t alignment;
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
    struct RTTI *item_type;
    struct RTTIContainerData *container_type;
    STRING_DATA type_name;
};

struct RTTIPointerData {
    STRING_DATA type_name;
    uint32_t size;
    uint8_t alignment;
    uint8_t unk_0D[3];
    void *constructor_fn;
    void *destructor_fn;
    void *getter_fn;
    void *setter_fn;
    void *copier_fn;
};

struct RTTIPointer {
    struct RTTI base;
    uint16_t unk_06;
    struct RTTI *item_type;
    struct RTTIPointerData *pointer_type;
    STRING_DATA type_name;
};

struct RTTIValue {
    uint64_t value;
    const char *name;
    const char *aliases[4];
};

struct RTTIEnum {
    struct RTTI base;
    uint8_t size;
    uint8_t alignment;
    uint16_t num_values;
    uint8_t unk_0A[6];
    STRING_DATA type_name;
    struct RTTIValue *values;
    struct RTTI *representation_type;
};

struct RTTIAtom {
    struct RTTI base;
    uint16_t size;
    uint8_t alignment;
    uint8_t simple; ///< A simple atom is one with a fixed memory layout, such as int32, bool, etc.
    uint8_t unk_08[6];
    STRING_DATA type_name;
    struct RTTI *base_type;
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
