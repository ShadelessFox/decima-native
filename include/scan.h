#ifndef DECIMA_NATIVE_SCAN_H
#define DECIMA_NATIVE_SCAN_H

struct Section {
    void *start;
    void *end;
};

_Bool FindSection(void* module, const char *name, struct Section *section);

_Bool FindPattern(void *start, const void *end, const char *pattern, void **position);

#endif //DECIMA_NATIVE_SCAN_H
