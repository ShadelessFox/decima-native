#ifndef DECIMA_NATIVE_MEMORY_H
#define DECIMA_NATIVE_MEMORY_H

#include <Windows.h>

struct Section {
    void *start;
    void *end;
};

BOOL FindSection(HMODULE module, const char *name, struct Section *section);

BOOL FindPattern(void **cur_ptr, const void *end_ptr, void **result, const void *pattern, size_t size);

BOOL WithinSection(const struct Section *section, const void *data);

#endif //DECIMA_NATIVE_MEMORY_H
