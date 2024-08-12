#pragma once

struct Section {
    void *start;
    void *end;
};

bool FindSection(void* module, const char *name, struct Section *section);

bool FindPattern(void *start, const void *end, const char *pattern, void **position);
