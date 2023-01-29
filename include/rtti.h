#ifndef DECIMA_NATIVE_RTTI_H
#define DECIMA_NATIVE_RTTI_H

#include <stdint.h>
#include <stdbool.h>

enum RTTIType {
    RTTIType_Primitive = 0,
    RTTIType_Reference,
    RTTIType_Container,
    RTTIType_Enum,
    RTTIType_Class,
    RTTIType_EnumFlags,
    RTTIType_POD,
};

struct RTTI {
    uint32_t id;
    uint8_t type;

    union {
        uint8_t enum_underlying_type_size;
        uint8_t class_inheritance_count;
    };

    union {
        uint16_t enum_member_count;

        struct {
            uint8_t class_member_count;
            uint8_t lua_function_count;
        };
    };
};

_Static_assert(sizeof(struct RTTI) == 0x8, "sizeof(struct RTTI) == 0x8");

struct RTTIClassSuperclass {
    struct RTTI *type;
    uint32_t offset;
};

_Static_assert(sizeof(struct RTTIClassSuperclass) == 0x10, "sizeof(struct RTTIClassSuperclass) == 0x10");

struct RTTIClassMember {
    struct RTTI *type;
    uint16_t offset;
    uint16_t flags;
    const char *name;
    const void *property_get_fn;
    const void *property_set_fn;
    uint64_t unk_24;
    uint64_t unk_2C;
};

_Static_assert(sizeof(struct RTTIClassMember) == 0x38, "sizeof(struct RTTIClassMember) == 0x38");

struct RTTIClassMessageHandler {
    struct RTTI *type;
    void *callback;
};

_Static_assert(sizeof(struct RTTIClassMessageHandler) == 0x10, "sizeof(struct RTTIClassMessageHandler) == 0x10");

struct RTTIClass {
    struct RTTI base;
    uint8_t message_handler_count;
    uint8_t inherited_message_count;
    uint8_t unk_0A[2];
    uint16_t version;
    uint8_t unk_0E[2];
    uint32_t size;
    uint16_t alignment;
    uint16_t flags;
    const void *constructor_fn;
    const void *destructor_fn;
    const void *deserialize_string_fn;
    const void *serialize_string_fn;
    const char *name;
    uint64_t unk_40;
    uint64_t unk_48;
    uint64_t unk_50;
    struct RTTIClassSuperclass *inheritance_table;
    struct RTTIClassMember *member_table;
    const void *lua_function_table;
    struct RTTIClassMessageHandler *message_handler_table;
    const void *inherited_message_table;
    const void *get_exported_symbols_fn;
};

_Static_assert(sizeof(struct RTTIClass) == 0x88, "sizeof(struct RTTIClass) == 0x88");

struct RTTIContainerData {
    const char *name;
};

struct RTTIContainer {
    struct RTTI base;
    struct RTTI *type;
    struct RTTIContainerData *data;
};

_Static_assert(sizeof(struct RTTIContainer) == 0x18, "sizeof(struct RTTIContainer) == 0x18");

struct RTTIEnumMember {
    int32_t value;
    const char *name;
    const char *alias0;
    const char *alias1;
    const char *alias2;
};

_Static_assert(sizeof(struct RTTIEnumMember) == 0x28, "sizeof(struct RTTIEnumMember) == 0x28");

struct RTTIEnum {
    struct RTTI base;
    uint8_t unk_08[8];
    const char *name;
    struct RTTIEnumMember *member_table;
    uint8_t unk_20[8];
};

_Static_assert(sizeof(struct RTTIEnum) == 0x28, "sizeof(struct RTTIEnum) == 0x28");

struct RTTIPrimitive {
    struct RTTI base;
    uint8_t unk_08[8];
    const char *name;
    struct RTTI *parent_type;
};

const char *RTTIType_ToString(enum RTTIType);

const char *RTTI_Name(struct RTTI *);

const char *RTTI_FullName(struct RTTI *);

_Bool RTTI_AsClass(struct RTTI *, struct RTTIClass **);

_Bool RTTI_AsContainer(struct RTTI *, struct RTTIContainer **);

_Bool RTTI_AsEnum(struct RTTI *, struct RTTIEnum **);

_Bool RTTI_AsPrimitive(struct RTTI *, struct RTTIPrimitive **);


#endif //DECIMA_NATIVE_RTTI_H
