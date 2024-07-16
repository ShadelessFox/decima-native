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
        case RTTIKind_Compound:
            return "class";
        case RTTIKind_EnumFlags:
            return "enum flags";
        case RTTIKind_POD:
            return "pod";
        default:
            assert(0 && "Unexpected RTTIKind");
            return "";
    }
}

const char *RTTI_Name(struct RTTI *rtti) {
    if (rtti->kind == RTTIKind_Compound)
        return ((struct RTTICompound *) rtti)->mTypeName;
    else if (rtti->kind == RTTIKind_Enum || rtti->kind == RTTIKind_EnumFlags)
        return ((struct RTTIEnum *) rtti)->type_name;
    else if (rtti->kind == RTTIKind_Atom)
        return ((struct RTTIAtom *) rtti)->mTypeName;
    else if (rtti->kind == RTTIKind_Container)
        return ((struct RTTIContainer *) rtti)->mTypeName;
    else if (rtti->kind == RTTIKind_Pointer)
        return ((struct RTTIPointer *) rtti)->mTypeName;
    assert(0 && "Unexpected RTTIKind");
    return "";
}

static char *RTTI_DisplayNameInternal(struct RTTI *rtti, char *ptr) {
    struct RTTIContainer *container = NULL;
    const char *name;

    if (RTTI_AsContainer(rtti, &container) || RTTI_AsPointer(rtti, (struct RTTIPointer **) &container)) {
        name = container->mContainerType->mTypeName;
    } else {
        name = RTTI_Name(rtti);
    }

    while (*name) {
        *ptr++ = *name++;
    }

    if (container) {
        *ptr++ = '<';
        ptr = RTTI_DisplayNameInternal(container->mItemType, ptr);
        *ptr++ = '>';
    }

    return ptr;
}

const char *RTTI_DisplayName(struct RTTI *rtti) {
    if (rtti->kind != RTTIKind_Container && rtti->kind != RTTIKind_Pointer)
        return RTTI_Name(rtti);

    static char buffer[4096];
    *RTTI_DisplayNameInternal(rtti, buffer) = '\0';
    return buffer;
}

_Bool RTTI_AsCompound(struct RTTI *rtti, struct RTTICompound **result) {
    if (rtti->kind == RTTIKind_Compound) {
        *result = (struct RTTICompound *) rtti;
        return true;
    }

    return false;
}

_Bool RTTI_AsContainer(struct RTTI *rtti, struct RTTIContainer **result) {
    if (rtti->kind == RTTIKind_Container) {
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
