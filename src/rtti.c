#include "rtti.h"

const char *RTTIKind_Name(enum RTTIKind kind) {
    switch (kind) {
        case RTTIKind_Atom:
            return "primitive";
        case RTTIKind_Pointer:
            return "reference";
        case RTTIKind_Container:
            return "container";
        case RTTIKind_Enum:
            return "enum";
        case RTTIKind_Class:
            return "class";
        case RTTIKind_EnumFlags:
            return "enum flags";
        case RTTIKind_POD:
            return "pod";
        default:
            assert(0 && "Unexpected RTTIKind");
            return NULL;
    }
}

const char *RTTI_Name(struct RTTI *rtti) {
    if (rtti->kind == RTTIKind_Class)
        return ((struct RTTICompound *) rtti)->type_name;
    else if (rtti->kind == RTTIKind_Enum || rtti->kind == RTTIKind_EnumFlags)
        return ((struct RTTIEnum *) rtti)->type_name;
    else if (rtti->kind == RTTIKind_Atom)
        return ((struct RTTIAtom *) rtti)->type_name;
    else if (rtti->kind == RTTIKind_Container || rtti->kind == RTTIKind_Pointer)
        return ((struct RTTIContainer *) rtti)->container_type->type_name;
    assert(0 && "Unexpected RTTIKind");
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
        ptr = RTTI_FullNameInternal(container->item_type, ptr);
        *ptr++ = '>';
    }

    return ptr;
}

const char *RTTI_FullName(struct RTTI *rtti) {
    static char buffer[4096];
    *RTTI_FullNameInternal(rtti, buffer) = '\0';
    return buffer;
}

_Bool RTTI_AsCompound(struct RTTI *rtti, struct RTTICompound **result) {
    if (rtti->kind == RTTIKind_Class) {
        *result = (struct RTTICompound *) rtti;
        return true;
    }

    return false;
}

_Bool RTTI_AsContainer(struct RTTI *rtti, struct RTTIContainer **result) {
    if (rtti->kind == RTTIKind_Container || rtti->kind == RTTIKind_Pointer) {
        *result = (struct RTTIContainer *) rtti;
        return true;
    }

    return false;
}

_Bool RTTI_AsPointer(struct RTTI *rtti, struct RTTIPointer **result) {
    if (rtti->kind == RTTIKind_Pointer) {
        *result = (struct RTTIPointer *) rtti;
        return true;
    }

    return false;
}

_Bool RTTI_AsEnum(struct RTTI *rtti, struct RTTIEnum **result) {
    if (rtti->kind == RTTIKind_Enum || rtti->kind == RTTIKind_EnumFlags) {
        *result = (struct RTTIEnum *) rtti;
        return true;
    }

    return false;
}

_Bool RTTI_AsAtom(struct RTTI *rtti, struct RTTIAtom **result) {
    if (rtti->kind == RTTIKind_Atom) {
        *result = (struct RTTIAtom *) rtti;
        return true;
    }

    return false;
}
