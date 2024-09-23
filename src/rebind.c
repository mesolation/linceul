#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <mach-o/nlist.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "mach_o_utils.h"
#include "rebind.h"

static pthread_mutex_t rebind_lock = PTHREAD_MUTEX_INITIALIZER;

static inline void *get_page_aligned_address(void *address) {
    uintptr_t page_size = (uintptr_t)sysconf(_SC_PAGESIZE);
    return (void *)((uintptr_t)address & ~(page_size - 1));
}

static int rebind_symbol_for_image(const struct mach_header *header, intptr_t slide, struct rebind_entry *entries, size_t count) {
    uintptr_t cur = (uintptr_t)header + sizeof(struct mach_header_64);
    struct segment_command_64 *seg_cmd;
    struct section_64 *sect;

    for (uint32_t i = 0; i < header->ncmds; i++, cur += seg_cmd->cmdsize) {
        seg_cmd = (struct segment_command_64 *)cur;
        if (seg_cmd->cmd != LC_SEGMENT_64) continue;

        if (strcmp(seg_cmd->segname, "__DATA") == 0 || strcmp(seg_cmd->segname, "__DATA_CONST") == 0) {
            sect = (struct section_64 *)((uintptr_t)seg_cmd + sizeof(struct segment_command_64));

            if (strcmp(sect->sectname, "__nl_symbol_ptr") == 0 || strcmp(sect->sectname, "__got") == 0) {
                uintptr_t base = (uintptr_t)(sect->addr) + slide;
                uint64_t *indirect_sym_table = (uint64_t *)base;
                void *page_start = get_page_aligned_address(indirect_sym_table);
                size_t page_size = sysconf(_SC_PAGESIZE);
                size_t size = sect->size + ((uintptr_t)indirect_sym_table - (uintptr_t)page_start);

                mprotect(page_start, size, PROT_READ | PROT_WRITE);
                for (uint32_t j = 0; j < sect->size / sizeof(void *); j++) {
                    struct nlist_64 *sym = get_symbol_for_index(j);
                    if (sym) {
                        for (size_t k = 0; k < count; k++) {
                            if (strcmp(entries[k].name, (char *)sym->n_un.n_strx) == 0) {
                                if (entries[k].replaced) {
                                    *entries[k].replaced = (void *)(uintptr_t)indirect_sym_table[j];
                                }
                                indirect_sym_table[j] = (uintptr_t)entries[k].replacement;
                            }
                        }
                    }
                }
                mprotect(page_start, size, PROT_READ);
            }
        }
    }
    return 0;
}

int rebind_symbols(struct rebind_entry *entries, size_t count) {
    pthread_mutex_lock(&rebind_lock);
    uint32_t image_count = _dyld_image_count();
    for (uint32_t i = 0; i < image_count; i++) {
        const struct mach_header *header = _dyld_get_image_header(i);
        intptr_t slide = _dyld_get_image_vmaddr_slide(i);
        rebind_symbol_for_image(header, slide, entries, count);
    }
    pthread_mutex_unlock(&rebind_lock);
    return 0;
}
