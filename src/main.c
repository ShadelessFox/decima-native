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
    const char *name = RTTI_Name(*(struct RTTI **) item);
    return hashmap_sip(name, strlen(name), seed0, seed1);
}

static int RTTI_Compare(const void *a, const void *b, void *data) {
    (void) data;
    return strcmp(RTTI_Name(*(struct RTTI **) a), RTTI_Name(*(struct RTTI **) b));
}

static int RTTIKind_Order(struct RTTI *rtti) {
    switch (rtti->kind) {
        case RTTIKind_Compound:
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

static void ScanType(struct RTTI *, struct hashmap *);

static void ExportTypes(FILE *file, struct RTTI **types, size_t count);

static void ExportIda(FILE *file, struct RTTI **types, size_t count);

static struct hashmap *g_all_types;

// 40 55 48 8B EC 48 83 EC 70 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 48 89 9C 24
static void (*RTTIFactory_RegisterAllTypes)() = (void (*)()) 0x7FF6096EF540;

// 40 55 53 56 48 8D 6C 24 ? 48 81 EC ? ? ? ? 0F B6 42 05 48 8B DA 48 8B
static char (*RTTIFactory_RegisterType)(void *, struct RTTI *) = (char (*)(void *, struct RTTI *)) 0x7FF608BC06F0;

static char RTTIFactory_RegisterType_Hook(void *a1, struct RTTI *type) {
    printf("RTTIFactory::RegisterType: '%s' (kind: %s, pointer: %p)\n", RTTI_Name(type), RTTIKind_Name(type->kind), type);
    ScanType(type, g_all_types);
    return RTTIFactory_RegisterType(a1, type);
}

static void RTTIFactory_RegisterAllTypes_Hook() {
    RTTIFactory_RegisterAllTypes();

    size_t count = hashmap_count(g_all_types);
    struct RTTI **item;
    struct RTTI **sorted = calloc(count, sizeof(struct RTTI *));

    for (size_t cur = 0, idx = 0; hashmap_iter(g_all_types, &idx, (void *) &item); cur++)
        sorted[cur] = *item;

    qsort(sorted, count, sizeof(struct RTTI *), RTTIKind_CompareLexical);

    FILE *file;

    fopen_s(&file, "hfw_types.json", "w");
    ExportTypes(file, sorted, count);
    fclose(file);

    fopen_s(&file, "hfw_ggrtti.idc", "w");
    ExportIda(file, sorted, count);
    fclose(file);

    free(sorted);

    ExitProcess(0);
}

BOOL APIENTRY DllMain(HINSTANCE handle, DWORD reason, LPVOID reserved) {
    (void) handle;
    (void) reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        AllocConsole();
        AttachConsole(ATTACH_PARENT_PROCESS);
        freopen("CON", "w", stdout);

        g_all_types = hashmap_new(sizeof(struct RTTI *), 0, 0, 0, RTTI_Hash, RTTI_Compare, NULL, NULL);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((PVOID *) &RTTIFactory_RegisterAllTypes, RTTIFactory_RegisterAllTypes_Hook);
        DetourAttach((PVOID *) &RTTIFactory_RegisterType, RTTIFactory_RegisterType_Hook);
        DetourTransactionCommit();
    } else if (reason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach((PVOID *) &RTTIFactory_RegisterAllTypes, RTTIFactory_RegisterAllTypes_Hook);
        DetourDetach((PVOID *) &RTTIFactory_RegisterType, RTTIFactory_RegisterType_Hook);
        DetourTransactionCommit();

        hashmap_free(g_all_types);
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

    printf("Found type '%s' (kind: %s, pointer: %p)\n", RTTI_Name(rtti), RTTIKind_Name(rtti->kind), rtti);

    if (RTTI_AsContainer(rtti, &object.container))
        ScanType(object.container->item_type, registered);
    if (RTTI_AsPointer(rtti, &object.pointer))
        ScanType(object.pointer->item_type, registered);
    else if (RTTI_AsAtom(rtti, &object.atom))
        ScanType(object.atom->base_type, registered);
    else if (RTTI_AsCompound(rtti, &object.compound)) {
        for (int index = 0; index < object.compound->base.bases_count; index++)
            ScanType(object.compound->bases[index].type, registered);
        for (int index = 0; index < object.compound->base.attrs_count; index++)
            ScanType(object.compound->attrs[index].type, registered);
        for (int index = 0; index < object.compound->message_handlers_count; index++)
            ScanType(object.compound->message_handlers[index].message, registered);
    }
}

static void ExportType(struct JsonContext *ctx, struct RTTI *rtti) {
    struct RTTICompound *rtti_class;
    struct RTTIEnum *rtti_enum;
    struct RTTIAtom *rtti_Atom;

    if (rtti->kind == RTTIKind_Pointer || rtti->kind == RTTIKind_Container || rtti->kind == RTTIKind_POD)
        return;

    JsonNameObject(ctx, RTTI_DisplayName(rtti));
    JsonNameValueStr(ctx, "kind", RTTIKind_Name(rtti->kind));

    if (RTTI_AsCompound(rtti, &rtti_class)) {
        JsonNameValueNum(ctx, "version", rtti_class->version);
        JsonNameValueNum(ctx, "flags", rtti_class->flags);

        if (rtti_class->message_handlers_count) {
            JsonNameArray(ctx, "messages");

            printf("message_handlers (pointer: %p, count: %d)\n", rtti_class->message_handlers,
                   rtti_class->message_handlers_count);
            for (int i = 0; i < rtti_class->message_handlers_count; i++) {
                struct RTTIMessageHandler *handler = &rtti_class->message_handlers[i];
                printf("  message_handler %d: %p '%s'\n", i, handler->message, RTTI_Name(handler->message));
                JsonValueStr(ctx, RTTI_DisplayName(handler->message));
            }

            JsonEndArray(ctx);
        }

        if (rtti_class->base.bases_count) {
            JsonNameArray(ctx, "bases");

            printf("bases (pointer: %p, count: %d)\n", rtti_class->bases, rtti_class->base.bases_count);
            for (int i = 0; i < rtti_class->base.bases_count; i++) {
                struct RTTIBase *base = &rtti_class->bases[i];
                printf("  base %d: %p '%s'\n", i, base->type, RTTI_Name(base->type));
                JsonBeginCompactObject(ctx);
                JsonNameValueStr(ctx, "name", RTTI_DisplayName(base->type));
                JsonNameValueNum(ctx, "offset", base->offset);
                JsonEndCompactObject(ctx);
            }

            JsonEndArray(ctx);
        }

        if (rtti_class->base.attrs_count) {
            struct RTTIAttr *category = NULL;

            JsonNameArray(ctx, "attrs");

            printf("attrs (pointer: %p, count: %d)\n", rtti_class->attrs, rtti_class->base.attrs_count);
            for (int i = 0; i < rtti_class->base.attrs_count; i++) {
                struct RTTIAttr *attr = &rtti_class->attrs[i];

                if (attr->type == NULL) {
                    category = attr;
                    continue;
                }

                printf("  attr %d: %p %s ('%s')\n", i, attr->type, attr->name, RTTI_Name(attr->type));
                JsonBeginCompactObject(ctx);
                JsonNameValueStr(ctx, "name", attr->name);
                JsonNameValueStr(ctx, "type", RTTI_DisplayName(attr->type));
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
        JsonNameValueStr(ctx, "base_type", RTTI_DisplayName(rtti_Atom->base_type));
    }

    JsonEndObject(ctx);
}

void ExportTypes(FILE *file, struct RTTI **types, size_t count) {
    struct JsonContext ctx;
    JsonInit(&ctx, file);
    JsonBeginObject(&ctx);

    JsonNameCompactObject(&ctx, "$spec");
    JsonNameValueNum(&ctx, "version", 4);
    JsonEndCompactObject(&ctx);

    for (size_t index = 0; index < count; index++) {
        ExportType(&ctx, types[index]);
    }

    JsonEndObject(&ctx);
}

static const char *RTTIKind_IDAName(enum RTTIKind kind) {
    switch (kind) {
        case RTTIKind_Atom:
            return "RTTIAtom";
        case RTTIKind_Pointer:
            return "RTTIPointer";
        case RTTIKind_Container:
            return "RTTIContainer";
        case RTTIKind_Enum:
        case RTTIKind_EnumFlags:
            return "RTTIEnum";
        case RTTIKind_Compound:
            return "RTTICompound";
        case RTTIKind_POD:
            return "RTTIPod";
        default:
            assert(0 && "Unexpected RTTIKind");
            return NULL;
    }
}

void ExportIda(FILE *file, struct RTTI **types, size_t count) {
    fputs("#include <idc.idc>\n\nstatic main()\n{", file);

    struct RTTICompound *type_compound;
    struct RTTIEnum *type_enum;
    struct RTTIContainer *type_container;
    struct RTTIPointer *type_pointer;

    for (size_t index = 0; index < count; index++) {
        struct RTTI *type = types[index];
        fprintf(file, "\n\tset_name(0x%p, \"RTTI_%s\");\n", type, RTTI_Name(type));
        fprintf(file, "\tapply_type(0x%p, \"%s\");\n", type, RTTIKind_IDAName(type->kind));

        if (RTTI_AsCompound(type, &type_compound)) {
            struct RTTIBase *bases = type_compound->bases;
            if (bases) {
                uint8_t bases_count = type->bases_count;
                fprintf(file, "\tdel_items(0x%p, DELIT_SIMPLE, %zu);\n", bases, bases_count * sizeof(struct RTTIBase));
                fprintf(file, "\tapply_type(0x%p, \"RTTIBase[%d]\");\n", bases, bases_count);
                fprintf(file, "\tset_name(0x%p, \"%s::sBases\");\n", bases, RTTI_Name(type));
            }
            struct RTTIAttr *attrs = type_compound->attrs;
            if (attrs) {
                uint8_t attrs_count = type->attrs_count;
                fprintf(file, "\tdel_items(0x%p, DELIT_SIMPLE, %zu);\n", attrs, attrs_count * sizeof(struct RTTIAttr));
                fprintf(file, "\tset_name(0x%p, \"%s::sAttrs\");\n", attrs, RTTI_Name(type));
                fprintf(file, "\tapply_type(0x%p, \"RTTIAttr[%d]\");\n", attrs, attrs_count);
            }
            struct RTTIMessageHandler *messages = type_compound->message_handlers;
            if (messages) {
                uint8_t messages_count = type_compound->message_handlers_count;
                fprintf(file, "\tdel_items(0x%p, DELIT_SIMPLE, %zu);\n", messages, messages_count * sizeof(struct RTTIMessageHandler));
                fprintf(file, "\tset_name(0x%p, \"%s::sMessageHandlers\");\n", messages, RTTI_Name(type));
                fprintf(file, "\tapply_type(0x%p, \"RTTIMessageHandler[%d]\");\n", messages, messages_count);

                for (uint8_t i = 0; i < messages_count; ++i) {
                    struct RTTIMessageHandler* message = &messages[i];
                    if (strcmp(RTTI_Name(message->message), "MsgReadBinary") == 0) {
                        fprintf(file, "\tset_name(0x%p, \"%s::OnReadBinary\");\n", message->handler, RTTI_Name(type));
                        fprintf(file, "\tapply_type(0x%p, \"__int64 __fastcall f(void* this, MsgReadBinary* msg)\");\n", message->handler);
                    }
                }
            }
        } else if (RTTI_AsEnum(type, &type_enum)) {
            if (type_enum->values) {
                fprintf(file, "\tdel_items(0x%p, DELIT_SIMPLE, %zu);\n", type_enum->values, type_enum->num_values * sizeof(struct RTTIValue));
                fprintf(file, "\tset_name(0x%p, \"%s::sValues\");\n", type_enum->values, RTTI_Name(type));
                fprintf(file, "\tapply_type(0x%p, \"RTTIValue[%d]\");", type_enum->values, type_enum->num_values);
            }
        } else if (RTTI_AsContainer(type, &type_container)) {
            fprintf(file, "\tset_name(0x%p, \"%s::sInfo\");\n", type_container->container_type, RTTI_Name(type));
            fprintf(file, "\tapply_type(0x%p, \"RTTIContainerInfo\");\n", type_container->container_type);
        } else if (RTTI_AsPointer(type, &type_pointer)) {
            fprintf(file, "\tset_name(0x%p, \"%s::sInfo\");\n", type_pointer->pointer_type, RTTI_Name(type));
            fprintf(file, "\tapply_type(0x%p, \"RTTIPointerInfo\");\n", type_pointer->pointer_type);
        }
    }

    fputs("}", file);
}
