#ifndef REBIND_H
#define REBIND_H

#include <stddef.h>

struct rebind_entry {
    const char *name;
    void *replacement;
    void **replaced;
};

int rebind_symbols(struct rebind_entry *entries, size_t count);

#endif
