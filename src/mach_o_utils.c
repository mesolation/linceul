#include <mach-o/getsect.h>
#include <mach-o/nlist.h>
#include <mach-o/dyld_images.h>

struct nlist_64 *get_symbol_for_index(uint32_t index) {
    struct dyld_all_image_infos *infos = (struct dyld_all_image_infos *)_dyld_get_all_image_infos();
    const struct dyld_image_info *image_info = infos->infoArray;
    
    for (uint32_t i = 0; i < infos->infoArrayCount; i++) {
        const struct mach_header_64 *header = (struct mach_header_64 *)image_info[i].imageLoadAddress;
        struct load_command *lc = (struct load_command *)(header + 1);
        struct symtab_command *symtab = NULL;
        
        for (uint32_t j = 0; j < header->ncmds; j++) {
            if (lc->cmd == LC_SYMTAB) {
                symtab = (struct symtab_command *)lc;
                break;
            }
            lc = (struct load_command *)((uintptr_t)lc + lc->cmdsize);
        }

        if (symtab) {
            struct nlist_64 *sym = (struct nlist_64 *)((uintptr_t)header + symtab->symoff);
            if (index < symtab->nsyms) {
                return &sym[index];
            }
        }
    }
    return NULL;
}
