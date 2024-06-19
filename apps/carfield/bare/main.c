#include <time.h>

#include "libhero/hero_api.h"

int main() {
    HeroDev hero_dev;
    hero_dev_mmap(&hero_dev);

/*
    uintptr_t l3_paddr = 0;
    void *l3_vaddr = (void*) hero_dev_l3_malloc(&hero_dev, 1024, &l3_paddr);

    if(!l3_vaddr) {
        printf("Wtf!\n");
        return 1;
    }

    printf("Carfield L3 alloc = %lx (%p)\n", l3_paddr, l3_vaddr);

    memset(l3_vaddr, 0x00, 1024); 
    printf("L3 mem before : %x\n", *(uint32_t*)l3_vaddr);

    void *l2_vaddr;
    uintptr_t l2_paddr;
    l2_vaddr = (void *) hero_dev_l2_malloc(&hero_dev, 1024, &l2_paddr);

    if(!l2_vaddr) {
        printf("Wtf!\n");
        return 1;
    }

    printf("Carfield L2 alloc = %lx (%p)\n", l2_paddr, l2_vaddr);

    memset(l3_vaddr, 0x0f, 1024); 

    hero_dev_dma_2d_memcpy(&hero_dev, l3_paddr, l2_paddr, 1024, 0, 0, 1);

    printf("L3 mem after : %x\n", *(uint32_t*)l3_vaddr);
*/

    return 0;
}