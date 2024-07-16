#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "rtti.h"
#include "json.h"
#include "scan.h"

#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <search.h>

#include <hashmap.h>
#include <detours.h>
#include <stdlib.h>

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

static void (*RTTIFactory_RegisterAllTypes)();

static char (*RTTIFactory_RegisterType)(void *, struct RTTI *);

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

_Bool APIENTRY DllMain(HINSTANCE handle, DWORD reason, LPVOID reserved) {
    (void) handle;
    (void) reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        switch (MessageBoxA(NULL, "Do you want to dump the RTTI?", "Choose action", MB_YESNOCANCEL | MB_ICONQUESTION)) {
            case IDYES:
                break;
            case IDNO:
                return TRUE;
            case IDCANCEL:
                ExitProcess(0);
        }

        AllocConsole();
        AttachConsole(ATTACH_PARENT_PROCESS);
        freopen("CON", "w", stdout);

        struct Section section;
        if (!FindSection(GetModuleHandleA(NULL), ".text", &section)) {
            perror("Unable to find '.text' section in the executable");
            return FALSE;
        }

        if (!FindPattern(section.start, section.end, "40 55 48 8B EC 48 83 EC 70 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 48 89 9C 24",
                         (void **) &RTTIFactory_RegisterAllTypes)) {
            perror("Unable to find 'RTTIFactory::RegisterAllTypes' function in the executable");
            return FALSE;
        }

        if (!FindPattern(section.start, section.end, "40 55 53 56 48 8D 6C 24 ? 48 81 EC ? ? ? ? 0F B6 42 05 48 8B DA 48 8B",
                         (void **) &RTTIFactory_RegisterType)) {
            perror("Unable to find 'RTTIFactory::RegisterType' function in the executable");
            return FALSE;
        }

        printf("Found RTTIFactory::RegisterAllTypes at %p\n", RTTIFactory_RegisterAllTypes);
        printf("Found RTTIFactory::RegisterType at %p\n", RTTIFactory_RegisterType);

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

    printf("Found mType '%s' (kind: %s, pointer: %p)\n", RTTI_Name(rtti), RTTIKind_Name(rtti->kind), rtti);

    if (RTTI_AsContainer(rtti, &object.container))
        ScanType(object.container->mItemType, registered);
    if (RTTI_AsPointer(rtti, &object.pointer))
        ScanType(object.pointer->mItemType, registered);
    else if (RTTI_AsAtom(rtti, &object.atom))
        ScanType(object.atom->mBaseType, registered);
    else if (RTTI_AsCompound(rtti, &object.compound)) {
        for (int index = 0; index < object.compound->mNumBases; index++)
            ScanType(object.compound->mBases[index].mType, registered);
        for (int index = 0; index < object.compound->mNumAttrs; index++)
            ScanType(object.compound->mAttrs[index].type, registered);
        for (int index = 0; index < object.compound->mNumMessageHandlers; index++)
            ScanType(object.compound->mMessageHandlers[index].mMessage, registered);
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
        JsonNameValueNum(ctx, "mVersion", rtti_class->mVersion);
        JsonNameValueNum(ctx, "mFlags", rtti_class->mFlags);

        if (rtti_class->mNumMessageHandlers) {
            JsonNameArray(ctx, "messages");

            printf("mMessageHandlers (pointer: %p, count: %d)\n", rtti_class->mMessageHandlers,
                   rtti_class->mNumMessageHandlers);
            for (int i = 0; i < rtti_class->mNumMessageHandlers; i++) {
                struct RTTIMessageHandler *handler = &rtti_class->mMessageHandlers[i];
                printf("  message_handler %d: %p '%s'\n", i, handler->mMessage, RTTI_Name(handler->mMessage));
                JsonValueStr(ctx, RTTI_DisplayName(handler->mMessage));
            }

            JsonEndArray(ctx);
        }

        if (rtti_class->mNumBases) {
            JsonNameArray(ctx, "mBases");

            printf("mBases (pointer: %p, count: %d)\n", rtti_class->mBases, rtti_class->mNumBases);
            for (int i = 0; i < rtti_class->mNumBases; i++) {
                struct RTTIBase *base = &rtti_class->mBases[i];
                printf("  base %d: %p '%s'\n", i, base->mType, RTTI_Name(base->mType));
                JsonBeginCompactObject(ctx);
                JsonNameValueStr(ctx, "mTypeName", RTTI_DisplayName(base->mType));
                JsonNameValueNum(ctx, "mOffset", base->mOffset);
                JsonEndCompactObject(ctx);
            }

            JsonEndArray(ctx);
        }

        if (rtti_class->mNumAttrs) {
            JsonNameArray(ctx, "mAttrs");

            printf("mAttrs (pointer: %p, count: %d)\n", rtti_class->mAttrs, rtti_class->mNumAttrs);
            for (int i = 0; i < rtti_class->mNumAttrs; i++) {
                struct RTTIAttr *attr = &rtti_class->mAttrs[i];

                if (attr->type == NULL) {
                    JsonBeginCompactObject(ctx);
                    JsonNameValueStr(ctx, "category", attr->mName);
                    JsonEndCompactObject(ctx);
                    continue;
                }

                printf("  attr %d: %p %s ('%s')\n", i, attr->type, attr->mName, RTTI_Name(attr->type));
                JsonBeginCompactObject(ctx);
                JsonNameValueStr(ctx, "mTypeName", attr->mName);
                JsonNameValueStr(ctx, "mType", RTTI_DisplayName(attr->type));
                JsonNameValueNum(ctx, "mOffset", attr->mOffset);
                JsonNameValueNum(ctx, "mFlags", attr->mFlags);
                if (attr->mMinValue)
                    JsonNameValueStr(ctx, "min", attr->mMinValue);
                if (attr->mMaxValue)
                    JsonNameValueStr(ctx, "max", attr->mMaxValue);
                if (attr->mGetter || attr->mSetter)
                    JsonNameValueBool(ctx, "property", 1);
                JsonEndCompactObject(ctx);
            }

            JsonEndArray(ctx);
        }
    } else if (RTTI_AsEnum(rtti, &rtti_enum)) {
        JsonNameValueNum(ctx, "mSize", rtti_enum->size);
        JsonNameArray(ctx, "values");

        for (int i = 0; i < rtti_enum->num_values; i++) {
            struct RTTIValue *m = &rtti_enum->values[i];

            JsonBeginCompactObject(ctx);
            JsonNameValueNum(ctx, "mValue", m->mValue);
            JsonNameValueStr(ctx, "mTypeName", m->mName);

            if (m->mAliases[0]) {
                JsonNameCompactArray(ctx, "alias");
                for (size_t j = 0; j < 4 && m->mAliases[j]; j++)
                    JsonValueStr(ctx, m->mAliases[j]);
                JsonEndArray(ctx);
            }

            JsonEndCompactObject(ctx);
        }

        JsonEndArray(ctx);
    } else if (RTTI_AsAtom(rtti, &rtti_Atom)) {
        JsonNameValueStr(ctx, "mBaseType", RTTI_DisplayName(rtti_Atom->mBaseType));
    }

    JsonEndObject(ctx);
}

void ExportTypes(FILE *file, struct RTTI **types, size_t count) {
    struct JsonContext ctx;
    JsonInit(&ctx, file);
    JsonBeginObject(&ctx);

    JsonNameCompactObject(&ctx, "$spec");
    JsonNameValueStr(&ctx, "mVersion", "5.0");
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
        fprintf(file, "\n\t// %s %s\n", RTTIKind_Name(type->kind), RTTI_Name(type));
        fprintf(file, "\tset_name(0x%p, \"RTTI_%s\");\n", type, RTTI_Name(type));
        fprintf(file, "\tapply_type(0x%p, \"%s\");\n", type, RTTIKind_IDAName(type->kind));

        if (RTTI_AsCompound(type, &type_compound)) {
            struct RTTIBase *bases = type_compound->mBases;
            if (bases) {
                uint8_t bases_count = type_compound->mNumBases;
                fprintf(file, "\tdel_items(0x%p, DELIT_SIMPLE, %zu);\n", bases, bases_count * sizeof(struct RTTIBase));
                fprintf(file, "\tapply_type(0x%p, \"RTTIBase[%d]\");\n", bases, bases_count);
                fprintf(file, "\tset_name(0x%p, \"%s::sBases\");\n", bases, RTTI_Name(type));
            }
            struct RTTIAttr *attrs = type_compound->mAttrs;
            if (attrs) {
                uint8_t attrs_count = type_compound->mNumAttrs;
                fprintf(file, "\tdel_items(0x%p, DELIT_SIMPLE, %zu);\n", attrs, attrs_count * sizeof(struct RTTIAttr));
                fprintf(file, "\tset_name(0x%p, \"%s::sAttrs\");\n", attrs, RTTI_Name(type));
                fprintf(file, "\tapply_type(0x%p, \"RTTIAttr[%d]\");\n", attrs, attrs_count);
            }
            struct RTTIMessageHandler *messages = type_compound->mMessageHandlers;
            if (messages) {
                uint8_t messages_count = type_compound->mNumMessageHandlers;
                fprintf(file, "\tdel_items(0x%p, DELIT_SIMPLE, %zu);\n", messages, messages_count * sizeof(struct RTTIMessageHandler));
                fprintf(file, "\tset_name(0x%p, \"%s::sMessageHandlers\");\n", messages, RTTI_Name(type));
                fprintf(file, "\tapply_type(0x%p, \"RTTIMessageHandler[%d]\");\n", messages, messages_count);

                for (uint8_t i = 0; i < messages_count; ++i) {
                    struct RTTIMessageHandler *message = &messages[i];
                    if (strcmp(RTTI_Name(message->mMessage), "MsgReadBinary") == 0) {
                        fprintf(file, "\tset_name(0x%p, \"%s::OnReadBinary\");\n", message->mHandler, RTTI_Name(type));
                        fprintf(file, "\tapply_type(0x%p, \"__int64 __fastcall f(void* this, MsgReadBinary* msg)\");\n", message->mHandler);
                    }
                }
            }
            struct RTTIMessageOrderEntry* message_order_entries = type_compound->mMessageOrderEntries;
            if (message_order_entries) {
                uint8_t entry_count = type_compound->mNumMessageOrderEntries;
                fprintf(file, "\tdel_items(0x%p, DELIT_SIMPLE, %zu);\n", message_order_entries, entry_count * sizeof(struct RTTIMessageOrderEntry));
                fprintf(file, "\tset_name(0x%p, \"%s::sInheritedMessageHandlers\");\n", message_order_entries, RTTI_Name(type));
                fprintf(file, "\tapply_type(0x%p, \"RTTIInheritedMessageHandler[%d]\");\n", message_order_entries, entry_count);
            }
            if (type_compound->mGetExportedSymbols) {
                fprintf(file, "\tset_name(0x%p, \"%s::GetExportedSymbols\");\n", type_compound->mGetExportedSymbols, RTTI_Name(type));
            }
        } else if (RTTI_AsEnum(type, &type_enum)) {
            if (type_enum->values) {
                fprintf(file, "\tdel_items(0x%p, DELIT_SIMPLE, %zu);\n", type_enum->values,
                        type_enum->num_values * sizeof(struct RTTIValue));
                fprintf(file, "\tset_name(0x%p, \"%s::sValues\");\n", type_enum->values, RTTI_Name(type));
                fprintf(file, "\tapply_type(0x%p, \"RTTIValue[%d]\");", type_enum->values, type_enum->num_values);
            }
        } else if (RTTI_AsContainer(type, &type_container)) {
            const char *container_name;
            if (strcmp(type_container->mContainerType->mTypeName, "Array") == 0) {
                // Arrays share the same info
                container_name = type_container->mContainerType->mTypeName;
            } else {
                container_name = type_container->mTypeName;
            }
            fprintf(file, "\tset_name(0x%p, \"%s::sInfo\");\n", type_container->mContainerType, container_name);
            fprintf(file, "\tapply_type(0x%p, \"RTTIContainerData\");\n", type_container->mContainerType);
        } else if (RTTI_AsPointer(type, &type_pointer)) {
            fprintf(file, "\tset_name(0x%p, \"%s::sInfo\");\n", type_pointer->mPointerType, type_pointer->mPointerType->mTypeName);
            fprintf(file, "\tapply_type(0x%p, \"RTTIPointerData\");\n", type_pointer->mPointerType);
        }
    }

    fputs("}", file);
}
