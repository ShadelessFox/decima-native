#include "rtti.h"
#include "json.h"

#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <search.h>

#include <hashmap.h>
#include <detours.h>

static uint64_t RTTI_Hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const char *name = RTTI_FullName(*(struct RTTI **) item);
    return hashmap_sip(name, strlen(name), seed0, seed1);
}

static int RTTI_Compare(const void *a, const void *b, void *data) {
    (void) data;
    return strcmp(RTTI_FullName(*(struct RTTI **) a), RTTI_FullName(*(struct RTTI **) b));
}

static int RTTIKind_Order(struct RTTI *rtti) {
    switch (rtti->kind) {
        case RTTIKind_Class:
            return 0;
        case RTTIKind_Enum:
            return 1;
        case RTTIKind_EnumFlags:
            return 2;
        case RTTIKind_Atom:
            return 3;
        default:
            return 4;
    }
}

static int RTTIKind_CompareLexical(const void *a, const void *b) {
    struct RTTI *a_rtti = *(struct RTTI **) a;
    struct RTTI *b_rtti = *(struct RTTI **) b;

    int order_a = RTTIKind_Order(a_rtti);
    int order_b = RTTIKind_Order(b_rtti);

    if (order_a != order_b)
        return order_a - order_b;

    return strcmp(RTTI_Name(a_rtti), RTTI_Name(b_rtti));
}

struct RegisteredType {
    uint64_t hash;
    struct RTTI *type;
};

struct Array_RegisteredType {
    struct RegisteredType *data;
    uint32_t count;
    uint32_t capacity;
};

static_assert(sizeof(struct RegisteredType) == 0x10, "sizeof(struct RegisteredType) == 0x10");

struct FactoryManager {
    uintptr_t vtbl;
    struct Array_RegisteredType types;
};

static void ExportTypes(FILE *, struct hashmap *);

static void ScanTypes(struct hashmap *, const struct Array_RegisteredType *);

static void ScanType(struct RTTI *, struct hashmap *);

static struct hashmap *all_types;

// 48 89 6B 58 FF 15 ? ? ? ? 40 38 2D ? ? ? ? 48 89 6B 68
static struct FactoryManager **s_factory_manager = (struct FactoryManager **) 0x7FF782550620;

// 40 55 48 8B EC 48 83 EC 70 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 48 89 9C 24
static void (*RTTIFactory_RegisterAllTypes)() = (void (*)()) 0x7FF780BCF300;

// 40 55 53 56 48 8D 6C 24 ? 48 81 EC ? ? ? ? 0F B6 42 05 48 8B DA 48 8B
static char (*RTTIFactory_RegisterType)(void *, struct RTTI *) = (char (*)(void *, struct RTTI *)) 0x7FF7800A07B0;

static char RTTIFactory_RegisterType_Hook(void *a1, struct RTTI *type) {
    printf("RTTIFactory::RegisterType: '%s' (pointer: %p)\n", RTTI_FullName(type), type);
    ScanType(type, all_types);
    return RTTIFactory_RegisterType(a1, type);
}

static void RTTIFactory_RegisterAllTypes_Hook() {
    RTTIFactory_RegisterAllTypes();

    FILE *file;
    fopen_s(&file, "hfw_types.json", "w");
    ExportTypes(file, all_types);
    fclose(file);

    puts("Saved to 'hfw_types.json'");
}

BOOL APIENTRY DllMain(HINSTANCE handle, DWORD reason, LPVOID reserved) {
    (void) handle;
    (void) reserved;

    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    if (reason == DLL_PROCESS_ATTACH) {
        AllocConsole();
        AttachConsole(ATTACH_PARENT_PROCESS);
        freopen("CON", "w", stdout);

        all_types = hashmap_new(sizeof(struct RTTI *), 0, 0, 0, RTTI_Hash, RTTI_Compare, NULL, NULL);

        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourAttach((PVOID *) &RTTIFactory_RegisterAllTypes, RTTIFactory_RegisterAllTypes_Hook);
        DetourAttach((PVOID *) &RTTIFactory_RegisterType, RTTIFactory_RegisterType_Hook);
        DetourTransactionCommit();
    } else if (reason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach((PVOID *) &RTTIFactory_RegisterAllTypes, RTTIFactory_RegisterAllTypes_Hook);
        DetourDetach((PVOID *) &RTTIFactory_RegisterType, RTTIFactory_RegisterType_Hook);
        DetourTransactionCommit();

        hashmap_free(all_types);
    }

    return TRUE;
}

static void ScanType(struct RTTI *rtti, struct hashmap *registered) {
    union {
        struct RTTIContainer *container;
        struct RTTIPointer *pointer;
        struct RTTIAtom *atom;
        struct RTTICompound *compound;
    } object;

    if (rtti == NULL || hashmap_get(registered, &rtti) != NULL)
        return;

    hashmap_set(registered, &rtti);

    printf("Found type '%s' (kind: %s, pointer: %p)\n", RTTI_FullName(rtti), RTTIKind_ToString(rtti->kind), rtti);

    if (RTTI_AsContainer(rtti, &object.container))
        ScanType(object.container->item_type, registered);
    if (RTTI_AsPointer(rtti, &object.pointer))
        ScanType(object.pointer->item_type, registered);
    else if (RTTI_AsAtom(rtti, &object.atom))
        ScanType(object.atom->base_type, registered);
    else if (RTTI_AsCompound(rtti, &object.compound)) {
        for (int index = 0; index < object.compound->base.class_num_bases; index++)
            ScanType(object.compound->bases[index].type, registered);
        for (int index = 0; index < object.compound->base.class_num_attrs; index++)
            ScanType(object.compound->attrs[index].type, registered);
        for (int index = 0; index < object.compound->num_message_handlers; index++)
            ScanType(object.compound->message_handlers[index].message, registered);
    }
}

static void ScanTypes(struct hashmap *registered, const struct Array_RegisteredType *types) {
    for (uint32_t index = 0; index < types->count; index++) {
        struct RegisteredType type = types->data[index];
        printf("[%d/%d] ", index, types->count);
        if (type.type == NULL) {
            puts("Skipping NULL type");
        } else {
            ScanType(type.type, registered);
            puts("");
        }
    }
}

static void ExportType(struct JsonContext *ctx, struct RTTI *rtti) {
    struct RTTICompound *rtti_class;
    struct RTTIEnum *rtti_enum;
    struct RTTIAtom *rtti_Atom;

    if (rtti->kind == RTTIKind_Pointer || rtti->kind == RTTIKind_Container || rtti->kind == RTTIKind_POD)
        return;

    JsonNameObject(ctx, RTTI_FullName(rtti));
    JsonNameValueStr(ctx, "kind", RTTIKind_ToString(rtti->kind));

    if (RTTI_AsCompound(rtti, &rtti_class)) {
        JsonNameValueNum(ctx, "version", rtti_class->version);
        JsonNameValueNum(ctx, "flags", rtti_class->flags);

        if (rtti_class->num_message_handlers) {
            JsonNameArray(ctx, "messages");

            printf("message_handlers (pointer: %p, count: %d)\n", rtti_class->message_handlers,
                   rtti_class->num_message_handlers);
            for (int i = 0; i < rtti_class->num_message_handlers; i++) {
                struct RTTIMessageHandler *handler = &rtti_class->message_handlers[i];
                printf("  message_handler %d: %p '%s'\n", i, handler->message, RTTI_FullName(handler->message));
                JsonValueStr(ctx, RTTI_FullName(handler->message));
            }

            JsonEndArray(ctx);
        }

        if (rtti_class->base.class_num_bases) {
            JsonNameArray(ctx, "bases");

            printf("bases (pointer: %p, count: %d)\n", rtti_class->bases, rtti_class->base.class_num_bases);
            for (int i = 0; i < rtti_class->base.class_num_bases; i++) {
                struct RTTIBase *base = &rtti_class->bases[i];
                printf("  base %d: %p '%s'\n", i, base->type, RTTI_FullName(base->type));
                JsonBeginCompactObject(ctx);
                JsonNameValueStr(ctx, "name", RTTI_FullName(base->type));
                JsonNameValueNum(ctx, "offset", base->offset);
                JsonEndCompactObject(ctx);
            }

            JsonEndArray(ctx);
        }

        if (rtti_class->base.class_num_attrs) {
            struct RTTIAttr *category = NULL;

            JsonNameArray(ctx, "attrs");

            printf("attrs (pointer: %p, count: %d)\n", rtti_class->attrs, rtti_class->base.class_num_attrs);
            for (int i = 0; i < rtti_class->base.class_num_attrs; i++) {
                struct RTTIAttr *attr = &rtti_class->attrs[i];

                if (attr->type == NULL) {
                    category = attr;
                    continue;
                }

                printf("  attr %d: %p %s ('%s')\n", i, attr->type, attr->name, RTTI_FullName(attr->type));
                JsonBeginCompactObject(ctx);
                JsonNameValueStr(ctx, "name", attr->name);
                JsonNameValueStr(ctx, "type", RTTI_FullName(attr->type));
                JsonNameValueStr(ctx, "category", category ? category->name : "");
                JsonNameValueNum(ctx, "offset", attr->offset);
                JsonNameValueNum(ctx, "flags", attr->flags);
                JsonNameValueBool(ctx, "property", attr->property_get_fn || attr->property_set_fn);
                JsonEndCompactObject(ctx);
            }

            JsonEndArray(ctx);
        }
    } else if (RTTI_AsEnum(rtti, &rtti_enum)) {
        JsonNameValueNum(ctx, "size", rtti_enum->base.enum_size);
        JsonNameArray(ctx, "values");

        for (int i = 0; i < rtti_enum->num_values; i++) {
            struct RTTIValue *m = &rtti_enum->values[i];

            JsonBeginCompactObject(ctx);
            JsonNameValueStr(ctx, "name", m->name);
            JsonNameValueNum(ctx, "value", m->value);
            JsonEndCompactObject(ctx);
        }

        JsonEndArray(ctx);
    } else if (RTTI_AsAtom(rtti, &rtti_Atom)) {
        JsonNameValueStr(ctx, "base_type", RTTI_FullName(rtti_Atom->base_type));
    }

    JsonEndObject(ctx);
}

void ExportTypes(FILE *file, struct hashmap *types) {
    size_t index = 0;
    struct RTTI **item;

    struct JsonContext ctx;
    JsonInit(&ctx, file);
    JsonBeginObject(&ctx);

    JsonNameCompactObject(&ctx, "$spec");
    JsonNameValueNum(&ctx, "version", 4);
    JsonEndCompactObject(&ctx);

    puts("Sorting types");

    size_t count = hashmap_count(types);
    struct RTTI **sorted = calloc(count, sizeof(struct RTTI *));

    for (size_t cur = 0; hashmap_iter(types, &index, (void *) &item); cur++) {
        sorted[cur] = *item;
    }

    qsort(sorted, count, sizeof(struct RTTI *), RTTIKind_CompareLexical);

    puts("Exporting types");

    for (index = 0; index < count; index++) {
        ExportType(&ctx, sorted[index]);
    }

    free(sorted);

    JsonEndObject(&ctx);
}
