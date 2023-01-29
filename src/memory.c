#include "memory.h"

#include <stdint.h>
#include <stdio.h>

BOOL FindSection(HMODULE module, const char *name, struct Section* result) {
    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER) module;
    PIMAGE_NT_HEADERS64 nt_header = (PIMAGE_NT_HEADERS) ((uint8_t *) module + dos_header->e_lfanew);
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt_header);

    for (WORD index = 0; index < nt_header->FileHeader.NumberOfSections; index++) {
        if (memcmp(section->Name, name, sizeof(section->Name)) == 0) {
            result->start = (uint8_t *) module + section->VirtualAddress;
            result->end = (uint8_t *) module + section->VirtualAddress + section->Misc.VirtualSize;
            return TRUE;
        }

        section++;
    }

    return FALSE;
}

BOOL FindPattern(void **cur_ptr, const void *end_ptr, void **result, const void *pattern, size_t size) {
    uintptr_t start = (uintptr_t) *cur_ptr;
    uintptr_t end = (uintptr_t) end_ptr;

    while (1) {
        if (end < start)
            return FALSE;
        if (end - start < 8)
            return FALSE;

        if (memcmp((void *) start, pattern, size) == 0) {
            *cur_ptr = (void *) (start + 1);
            *result = (void *) start;
            return TRUE;
        }

        start++;
    }
}

BOOL WithinSection(const struct Section *section, const void *data) {
    return (uintptr_t) section->start <= (uintptr_t) data && (uintptr_t) data < (uintptr_t) section->end;
}
