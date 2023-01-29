#include "rtti.h"

const char *RTTIType_ToString(enum RTTIType self) {
    switch (self) {
        case RTTIType_Primitive:
            return "primitive";
        case RTTIType_Reference:
            return "reference";
        case RTTIType_Container:
            return "container";
        case RTTIType_Enum:
            return "enum";
        case RTTIType_Class:
            return "class";
        case RTTIType_EnumFlags:
            return "enum flags";
        case RTTIType_POD:
            return "pod";
        default:
            return NULL;
    }
}

const char *RTTI_Name(struct RTTI *rtti) {
    if (rtti->type == RTTIType_Class)
        return ((struct RTTIClass *) rtti)->name;
    else if (rtti->type == RTTIType_Enum || rtti->type == RTTIType_EnumFlags)
        return ((struct RTTIEnum *) rtti)->name;
    else if (rtti->type == RTTIType_Primitive)
        return ((struct RTTIPrimitive *) rtti)->name;
    else if (rtti->type == RTTIType_Container || rtti->type == RTTIType_Reference)
        return ((struct RTTIContainer *) rtti)->data->name;
    return NULL;
}

static char *RTTI_FullNameInternal(struct RTTI *rtti, char *ptr) {
    struct RTTIContainer *container;
    const char *name = RTTI_Name(rtti);

    while (*name) {
        *ptr++ = *name++;
    }

    if (RTTI_AsContainer(rtti, &container)) {
        *ptr++ = '<';
        ptr = RTTI_FullNameInternal(container->type, ptr);
        *ptr++ = '>';
    }

    return ptr;
}

const char *RTTI_FullName(struct RTTI *rtti) {
    static char buffer[4096];
    *RTTI_FullNameInternal(rtti, buffer) = '\0';
    return buffer;
}

_Bool RTTI_AsClass(struct RTTI *rtti, struct RTTIClass **result) {
    if (rtti->type == RTTIType_Class) {
        *result = (struct RTTIClass *) rtti;
        return true;
    }

    return false;
}

_Bool RTTI_AsContainer(struct RTTI *rtti, struct RTTIContainer **result) {
    if (rtti->type == RTTIType_Container || rtti->type == RTTIType_Reference) {
        *result = (struct RTTIContainer *) rtti;
        return true;
    }

    return false;
}

_Bool RTTI_AsEnum(struct RTTI *rtti, struct RTTIEnum **result) {
    if (rtti->type == RTTIType_Enum || rtti->type == RTTIType_EnumFlags) {
        *result = (struct RTTIEnum *) rtti;
        return true;
    }

    return false;
}

_Bool RTTI_AsPrimitive(struct RTTI *rtti, struct RTTIPrimitive **result) {
    if (rtti->type == RTTIType_Primitive) {
        *result = (struct RTTIPrimitive *) rtti;
        return true;
    }

    return false;
}
