#include "memory.h"
#include "rtti.h"
#include "hashmap.h"
#include "json.h"

#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static const uint8_t type_pattern[] = {0xff, 0xff, 0xff, 0xff};

static void ScanTypes(struct hashmap *result);

static void RegisterTypes(struct hashmap *registered, struct hashmap *scanned);

static void ExportTypes(FILE *file, struct hashmap *types);

static uint64_t RTTITypeHash(const void *item, uint64_t seed0, uint64_t seed1) {
    const char *name = RTTI_Name(*(struct RTTI **) item);
    return hashmap_sip(name, strlen(name), seed0, seed1);
}

static int RTTITypeCompare(const void *a, const void *b, void *data) {
    (void) data;
    return strcmp(RTTI_Name(*(struct RTTI **) a), RTTI_Name(*(struct RTTI **) b));
}

static int RTTITypeOrder(struct RTTI *rtti) {
    switch (rtti->type) {
        case RTTIType_Class:
            return 0;
        case RTTIType_Enum:
            return 1;
        case RTTIType_EnumFlags:
            return 2;
        case RTTIType_Primitive:
            return 3;
        default:
            return 4;
    }
}

static int RTTITypeCompareLexical(const void *a, const void *b) {
    struct RTTI *a_rtti = *(struct RTTI **) a;
    struct RTTI *b_rtti = *(struct RTTI **) b;

    int order_a = RTTITypeOrder(a_rtti);
    int order_b = RTTITypeOrder(b_rtti);

    if (order_a != order_b)
        return order_a - order_b;

    return strcmp(RTTI_Name(a_rtti), RTTI_Name(b_rtti));
}

static struct hashmap *scanned_types;
static struct hashmap *registered_types;
static struct Section rdata, data;

BOOL APIENTRY DllMain(HINSTANCE handle, DWORD reason, LPVOID reserved) {
    (void) handle;
    (void) reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        HMODULE module = GetModuleHandleA(NULL);
        if (!FindSection(module, ".rdata", &rdata) || !FindSection(module, ".data", &data))
            return FALSE;

        AllocConsole();
        AttachConsole(ATTACH_PARENT_PROCESS);
        freopen("CON", "w", stdout);

        scanned_types = hashmap_new(sizeof(struct RTTI *), 0, 0, 0, RTTITypeHash, RTTITypeCompare, NULL, NULL);
        registered_types = hashmap_new(sizeof(struct RTTI *), 0, 0, 0, RTTITypeHash, RTTITypeCompare, NULL, NULL);

        ScanTypes(scanned_types);
        printf("Types scanned: %zu\n", hashmap_count(scanned_types));
    }

    if (reason == DLL_THREAD_ATTACH) {
        ScanTypes(scanned_types);
        printf("Types scanned: %zu\n", hashmap_count(scanned_types));
    }

    if (reason == DLL_PROCESS_DETACH) {
        ScanTypes(scanned_types);
        printf("Types scanned: %zu\n", hashmap_count(scanned_types));

        RegisterTypes(registered_types, scanned_types);
        printf("Types registered: %zu\n", hashmap_count(registered_types));

        FILE *file;
        fopen_s(&file, "types.json", "w");
        ExportTypes(file, registered_types);
        fclose(file);

        FreeConsole();
    }

    return TRUE;
}

void ScanTypes(struct hashmap *result) {
    void *current = rdata.start;
    struct RTTI *rtti;

    while (FindPattern(&current, data.end, (void **) &rtti, type_pattern, sizeof(type_pattern))) {
        struct RTTIClass *rtti_class;
        struct RTTIEnum *rtti_enum;
        struct RTTIPrimitive *rtti_primitive;

        if (RTTI_AsClass(rtti, &rtti_class)) {
            if (!WithinSection(&rdata, rtti_class->name))
                continue;

            if (rtti_class->base.class_member_count > 0
                ? !WithinSection(&data, rtti_class->member_table) : rtti_class->member_table != NULL)
                continue;

            if (rtti_class->base.class_inheritance_count > 0
                ? !WithinSection(&data, rtti_class->inheritance_table) : rtti_class->inheritance_table != NULL)
                continue;

            hashmap_set(result, &rtti);
        }

        if (RTTI_AsEnum(rtti, &rtti_enum)) {
            if (!WithinSection(&rdata, rtti_enum->name))
                continue;

            hashmap_set(result, &rtti);
        }

        if (RTTI_AsPrimitive(rtti, &rtti_primitive)) {
            if (!WithinSection(&rdata, rtti_primitive->name) || !WithinSection(&data, rtti_primitive->parent_type))
                continue;

            hashmap_set(result, &rtti);
        }
    }
}

static void RegisterType(struct RTTI *rtti, struct hashmap *registered, const struct hashmap *scanned) {
    struct RTTIContainer *rtti_container;
    struct RTTIPrimitive *rtti_primitive;
    struct RTTIClass *rtti_class;

    if (rtti == NULL || hashmap_get(registered, &rtti) != NULL)
        return;

    hashmap_set(registered, &rtti);

    if (RTTI_AsContainer(rtti, &rtti_container))
        RegisterType(rtti_container->type, registered, scanned);
    else if (RTTI_AsPrimitive(rtti, &rtti_primitive))
        RegisterType(rtti_primitive->parent_type, registered, scanned);
    else if (RTTI_AsClass(rtti, &rtti_class)) {
        for (int index = 0; index < rtti_class->base.class_inheritance_count; index++)
            RegisterType(rtti_class->inheritance_table[index].type, registered, scanned);
        for (int index = 0; index < rtti_class->base.class_member_count; index++)
            RegisterType(rtti_class->member_table[index].type, registered, scanned);
        for (int index = 0; index < rtti_class->message_handler_count; index++)
            RegisterType(rtti_class->message_handler_table[index].type, registered, scanned);
    }
}

void RegisterTypes(struct hashmap *registered, struct hashmap *scanned) {
    size_t index = 0;
    struct RTTI **item;

    while (hashmap_iter(scanned, &index, (void *) &item)) {
        RegisterType(*item, registered, scanned);
    }
}

static void ExportType(struct JsonContext *ctx, struct RTTI *rtti) {
    struct RTTIClass *rtti_class;
    struct RTTIEnum *rtti_enum;
    struct RTTIPrimitive *rtti_primitive;

    if (rtti->type == RTTIType_Reference || rtti->type == RTTIType_Container || rtti->type == RTTIType_POD)
        return;

    JsonNameObject(ctx, RTTI_FullName(rtti));
    JsonNameValueStr(ctx, "type", RTTIType_ToString(rtti->type));

    if (RTTI_AsClass(rtti, &rtti_class)) {
        JsonNameValueNum(ctx, "version", rtti_class->version);
        JsonNameValueNum(ctx, "flags", rtti_class->flags);

        if (rtti_class->message_handler_count) {
            JsonNameArray(ctx, "messages");

            for (int i = 0; i < rtti_class->message_handler_count; i++) {
                struct RTTIClassMessageHandler *handler = &rtti_class->message_handler_table[i];

                JsonValueStr(ctx, RTTI_FullName(handler->type));
            }

            JsonEndArray(ctx);
        }

        if (rtti_class->base.class_inheritance_count) {
            JsonNameArray(ctx, "extends");

            for (int i = 0; i < rtti_class->base.class_inheritance_count; i++) {
                struct RTTIClassSuperclass *superclass = &rtti_class->inheritance_table[i];

                JsonBeginCompactObject(ctx);
                JsonNameValueStr(ctx, "name", RTTI_FullName(superclass->type));
                JsonNameValueNum(ctx, "offset", superclass->offset);
                JsonEndCompactObject(ctx);
            }

            JsonEndArray(ctx);
        }

        if (rtti_class->base.class_member_count) {
            JsonNameArray(ctx, "fields");

            struct RTTIClassMember *category = NULL;

            for (int i = 0; i < rtti_class->base.class_member_count; i++) {
                struct RTTIClassMember *member = &rtti_class->member_table[i];

                if (member->type == NULL) {
                    category = member;
                    continue;
                }

                JsonBeginCompactObject(ctx);
                JsonNameValueStr(ctx, "name", member->name);
                JsonNameValueStr(ctx, "type", RTTI_FullName(member->type));
                JsonNameValueStr(ctx, "category", category ? category->name : "");
                JsonNameValueNum(ctx, "offset", member->offset);
                JsonNameValueNum(ctx, "flags", member->flags);
                JsonNameValueBool(ctx, "is_property", member->property_get_fn || member->property_set_fn);
                JsonEndCompactObject(ctx);
            }

            JsonEndArray(ctx);
        }
    } else if (RTTI_AsEnum(rtti, &rtti_enum)) {
        JsonNameValueNum(ctx, "size", rtti_enum->base.enum_underlying_type_size);
        JsonNameArray(ctx, "values");

        for (int i = 0; i < rtti_enum->base.enum_member_count; i++) {
            struct RTTIEnumMember *m = &rtti_enum->member_table[i];

            JsonBeginCompactObject(ctx);
            JsonNameValueNum(ctx, "value", m->value);
            JsonNameValueStr(ctx, "name", m->name);
            if (m->alias0) JsonNameValueStr(ctx, "alias1", m->alias0);
            if (m->alias1) JsonNameValueStr(ctx, "alias2", m->alias1);
            if (m->alias2) JsonNameValueStr(ctx, "alias3", m->alias2);
            JsonEndCompactObject(ctx);
        }

        JsonEndArray(ctx);
    } else if (RTTI_AsPrimitive(rtti, &rtti_primitive)) {
        JsonNameValueStr(ctx, "parent_type", RTTI_FullName(rtti_primitive->parent_type));
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
    JsonNameValueNum(&ctx, "version", 3);
    JsonEndCompactObject(&ctx);

    size_t count = hashmap_count(types);
    struct RTTI **sorted = calloc(count, sizeof(struct RTTI *));

    for (size_t cur = 0; hashmap_iter(types, &index, (void *) &item); cur++) {
        sorted[cur] = *item;
    }

    qsort(sorted, count, sizeof(struct RTTI *), RTTITypeCompareLexical);

    for (index = 0; index < count; index++) {
        ExportType(&ctx, sorted[index]);
    }

    free(sorted);

    JsonEndObject(&ctx);
}
