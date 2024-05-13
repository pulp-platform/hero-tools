// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>


#include <stdint.h>

#include "o1heap.h"
#include "libhero/debug.h"
#include "libhero/herodev.h"
#include "libhero/io.h"
#include "libhero/util.h"

#include "occamy_driver.h"
#include "snitch_cluster.h"
#include "snitch_regs.h"

#include <stdint.h>


#define ALIGN_UP(x, p) (((x) + (p)-1) & ~((p)-1))

extern struct O1HeapInstance *l2_heap_manager;
extern uint64_t l2_heap_start_phy, l2_heap_start_virt, l2_heap_size;
extern uint64_t l3_heap_start_phy, l3_heap_start_virt, l3_heap_size;

uintptr_t occamy_lookup_mem(int device_fd, int mmap_id, size_t *size_b, uintptr_t *p_addr) {
    struct card_ioctl_arg chunk;
    chunk.mmap_id = mmap_id;
    int err = ioctl(device_fd, IOCTL_MEM_INFOS, &chunk);
    pr_trace("Lookup %llx %llx\n", chunk.size, chunk.result_phys_addr);
    if (err) {
        pr_error("Communication error from driver\n");
        return err;
    }
    *size_b = chunk.size;
    *p_addr = chunk.result_phys_addr;
    return err;
}

int occamy_mmap(int device_fd, int mmap_id, size_t length,
                  void **res) {
    *res = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, device_fd,
                mmap_id * getpagesize());

    if (*res == MAP_FAILED) {
        printf("mmap() failed %s for offset: %x length: %llx\n", strerror(errno),
               mmap_id, length);
        *res = NULL;
        return -EIO;
    }
    return 0;
}

int occamy_lookup_mmap(int device_fd, int mmap_id, void **res) {
    size_t phy_len = 0;
    uintptr_t phy_base = NULL;

    occamy_lookup_mem(device_fd, mmap_id, &phy_len, &phy_base);

    return occamy_mmap(device_fd, mmap_id, phy_len, res);
}

static void occamy_set_isolation(int iso) {
    uint32_t mask, val;
    val = iso ? 1U : 0U;
    mask = (val << QCTL_ISOLATE_NARROW_IN_BIT) | (val << QCTL_ISOLATE_NARROW_OUT_BIT) |
           (val << QCTL_ISOLATE_WIDE_IN_BIT) | (val << QCTL_ISOLATE_WIDE_OUT_BIT);
    writew(mask, occ_quad_ctrl + QCTL_ISOLATE_REG_OFFSET);
    fence();
}

void clint_set_irq(uint32_t irq) {
    int val = readw((uint32_t *)occ_clint);
    writew(val | irq, (uint32_t *)occ_clint);
}

void clint_clear_irq(uint32_t irq) {
    int val = readw((uint32_t *)occ_clint);
    writew(val & ~irq, (uint32_t *)occ_clint);
}

// Set up transparent TLB
static int occamy_tlb_write(uint32_t idx, uint64_t addr_begin, uint64_t addr_end, uint32_t flags) {
    uint64_t page_num_base = addr_begin >> 12;
    uint64_t page_num_first = addr_begin >> 12;
    uint64_t page_num_last = addr_end >> 12;

    writew((uint32_t)  page_num_first        , (uint32_t *) (occ_quad_ctrl + QCTL_TLB_NARROW_REG_OFFSET + QCTL_TLB_REG_STRIDE * idx + 0x00) );
    writew((uint32_t) (page_num_first >> 32) , (uint32_t *) (occ_quad_ctrl + QCTL_TLB_NARROW_REG_OFFSET + QCTL_TLB_REG_STRIDE * idx + 0x04) );
    writew((uint32_t)  page_num_last         , (uint32_t *) (occ_quad_ctrl + QCTL_TLB_NARROW_REG_OFFSET + QCTL_TLB_REG_STRIDE * idx + 0x08) );
    writew((uint32_t) (page_num_last >> 32)  , (uint32_t *) (occ_quad_ctrl + QCTL_TLB_NARROW_REG_OFFSET + QCTL_TLB_REG_STRIDE * idx + 0x0c) );
    writew((uint32_t)  page_num_base         , (uint32_t *) (occ_quad_ctrl + QCTL_TLB_NARROW_REG_OFFSET + QCTL_TLB_REG_STRIDE * idx + 0x10) );
    writew((uint32_t) (page_num_base >> 32)  , (uint32_t *) (occ_quad_ctrl + QCTL_TLB_NARROW_REG_OFFSET + QCTL_TLB_REG_STRIDE * idx + 0x14) );
    writew((uint32_t)  flags                 , (uint32_t *) (occ_quad_ctrl + QCTL_TLB_NARROW_REG_OFFSET + QCTL_TLB_REG_STRIDE * idx + 0x18) );
    writew((uint32_t)  page_num_first        , (uint32_t *) (occ_quad_ctrl + QCTL_TLB_WIDE_REG_OFFSET   + QCTL_TLB_REG_STRIDE * idx + 0x00) );
    writew((uint32_t) (page_num_first >> 32) , (uint32_t *) (occ_quad_ctrl + QCTL_TLB_WIDE_REG_OFFSET   + QCTL_TLB_REG_STRIDE * idx + 0x04) );
    writew((uint32_t)  page_num_last         , (uint32_t *) (occ_quad_ctrl + QCTL_TLB_WIDE_REG_OFFSET   + QCTL_TLB_REG_STRIDE * idx + 0x08) );
    writew((uint32_t) (page_num_last >> 32)  , (uint32_t *) (occ_quad_ctrl + QCTL_TLB_WIDE_REG_OFFSET   + QCTL_TLB_REG_STRIDE * idx + 0x0c) );
    writew((uint32_t)  page_num_base         , (uint32_t *) (occ_quad_ctrl + QCTL_TLB_WIDE_REG_OFFSET   + QCTL_TLB_REG_STRIDE * idx + 0x10) );
    writew((uint32_t) (page_num_base >> 32)  , (uint32_t *) (occ_quad_ctrl + QCTL_TLB_WIDE_REG_OFFSET   + QCTL_TLB_REG_STRIDE * idx + 0x14) );
    writew((uint32_t)  flags                 , (uint32_t *) (occ_quad_ctrl + QCTL_TLB_WIDE_REG_OFFSET   + QCTL_TLB_REG_STRIDE * idx + 0x18) );
    return 0;
}

void hero_dev_reset(HeroDev *dev, unsigned full) {
    int err;
    pr_trace("%s snitch_cluster\n", __func__);
    // Isolate
    occamy_set_isolation(1);
    // Reset
    writew(0, occ_quad_ctrl + QCTL_RESET_N_REG_OFFSET);
    clint_clear_irq(0x1FF << 1);
    fence();
    for (volatile int i = 0; i < 16; i++)
	;
    // De-reset
    writew(1, occ_quad_ctrl + QCTL_RESET_N_REG_OFFSET);
    fence();
    // De-isolate
    occamy_set_isolation(0);
}

// TODO: Removed numerical constants and share some header file
// with the driver
int hero_dev_mmap(HeroDev *dev) {
    int err = 0;
    device_fd = open("/dev/occamydev--1", O_RDWR | O_SYNC);
    pr_trace("%s snitch_cluster\n", __func__);

    // Call card_mmap from the driver map address spaces
    if (occamy_lookup_mmap(device_fd, SNITCH_CLUSTER_MAPID, &occ_snitch_cluster))
        goto error_driver;
    if (occamy_lookup_mmap(device_fd, QUADRANT_CTRL_MAPID, &occ_quad_ctrl))
        goto error_driver;
    if (occamy_lookup_mmap(device_fd, SOC_CTRL_MAPID, &occ_soc_ctrl))
        goto error_driver;
    if (occamy_lookup_mmap(device_fd, L3_MMAP_ID, &occ_l3))
        goto error_driver;
    if (occamy_lookup_mmap(device_fd, SCRATCHPAD_WIDE_MAPID, &occ_l2))
        goto error_driver;
    if (occamy_lookup_mmap(device_fd, CLINT_MAPID, &occ_clint))
        goto error_driver;
    
    // Put the occamy tcdm in the local_mems list for OpenMP to use
    HeroSubDev_t *local_mems_tail = malloc(sizeof(HeroSubDev_t));
    size_t occ_snitch_cluster_size; 
    uintptr_t occ_snitch_cluster_phys;
    occamy_lookup_mem(device_fd, SNITCH_CLUSTER_MAPID, &occ_snitch_cluster_size, &occ_snitch_cluster_phys);
    if(!local_mems_tail){
        pr_error("Error when allocating local_mems_tail.\n");
        goto error_driver;
    }
    local_mems_tail->v_addr = occ_snitch_cluster;
    // TODO: Split lookup between device and host phy addr
    local_mems_tail->p_addr = 0xFFFFFFFF & occ_snitch_cluster_phys;
    local_mems_tail->size   = occ_snitch_cluster_size;
    local_mems_tail->alias  = "l1_snitch_cluster";
    dev->local_mems = local_mems_tail;

    // Put half of the l2 memory map in the global_mems list
    // (We give the first half to openmp and the top half to o1heap)
    size_t occ_l2_size; 
    uintptr_t occ_l2_phys; 
    occamy_lookup_mem(device_fd, SCRATCHPAD_WIDE_MAPID, &occ_l2_size, &occ_l2_phys);

    // Use the upper half of the device L2 mem for the heap allocator
    // TODO: Get phy addresses from the driver
    l2_heap_start_phy = occ_l2_phys + ALIGN_UP(occ_l2_size / 2, O1HEAP_ALIGNMENT);
    l2_heap_start_virt = occ_l2 + ALIGN_UP(occ_l2_size / 2, O1HEAP_ALIGNMENT);
    l2_heap_size = occ_l2_size / 2;
    pr_trace("%llx %llx %llx %llx\n", occ_l2_phys, occ_l2_size, occ_l2_size / 2, l2_heap_size);
    err = hero_dev_l2_init(dev);
    if(err) {
        pr_error("Error when initializing L2 mem.\n");
        goto end;
    }

    // Use the L3 mem for heap allocator
    // TODO: Get phy addresses from the driver
    size_t occ_l3_size; 
    uintptr_t occ_l3_phys; 
    occamy_lookup_mem(device_fd, L3_MMAP_ID, &occ_l3_size, &occ_l3_phys);
    l3_heap_start_phy = occ_l3_phys + ALIGN_UP(occ_l3_size / 2, O1HEAP_ALIGNMENT);
    l3_heap_start_virt = occ_l3 + ALIGN_UP(occ_l3_size / 2, O1HEAP_ALIGNMENT);
    l3_heap_size = occ_l3_size / 2;
    err = hero_dev_l3_init(dev);
    if(err) {
        pr_error("Error when initializing L3 mem.\n");
        goto end;
    }

    HeroSubDev_t *l2_mems_tail = malloc(sizeof(HeroSubDev_t));
    if(!l2_mems_tail){
        pr_error("Error when allocating l2_mems_tail.\n");
        goto error_driver;
    }
    l2_mems_tail->v_addr = occ_l3;
    // TODO: Split lookup between device and host phy addr
    l2_mems_tail->p_addr = 0xFFFFFFFF & occ_l3_phys;
    l2_mems_tail->size   = occ_l3_size / 2;
    l2_mems_tail->alias  = "l3";
    l2_mems_tail->next   = NULL;
    dev->global_mems = l2_mems_tail;
    
    goto end;
error_driver:
    err = -1;
    pr_error("Error when communicating with the Carfield driver.\n");
end:
    return err;
}

int hero_dev_init(HeroDev *dev) {
    pr_trace("%s snitch_cluster\n", __func__);

    // Allocate sw mailboxes
    hero_dev_alloc_mboxes(dev);
    // Point to the mailboxes
    int64_t mbox_ptrs_phy;
    struct l3_layout *mbox_ptrs = hero_dev_l3_malloc(dev, sizeof(struct l3_layout), &mbox_ptrs_phy);

    mbox_ptrs->h2a_mbox = (uint32_t) dev->mboxes.h2a_mbox_mem.p_addr;
    mbox_ptrs->a2h_mbox = (uint32_t) dev->mboxes.a2h_mbox_mem.p_addr;
    mbox_ptrs->a2h_rb = (uint32_t) dev->mboxes.rb_mbox_mem.p_addr;
    mbox_ptrs->heap = l3_heap_start_phy;
    // Put the mailboxes struct somewhere in L3 (unused)

    writew(mbox_ptrs_phy, occ_soc_ctrl + SCTL_SCRATCH_2_REG_OFFSET);

    occamy_tlb_write(0, 0x01000000, 0x0101ffff, 0x3);  // BOOTROM
    occamy_tlb_write(1, 0x02000000, 0x02000fff, 0x3);  // SoC Control
    occamy_tlb_write(2, 0x04000000, 0x040fffff, 0x1);  // CLINT
    occamy_tlb_write(3, 0x10000000, 0x105fffff, 0x1);  // Quadrants
    occamy_tlb_write(4, 0xC0000000, 0xffffffff, 0x1);  // HBM0/1
    occamy_tlb_write(5, 0x71000000, 0x71100000, 0x1);  // SPM wide

    // Enable tlb
    writew(1, occ_quad_ctrl + 0x18);
    writew(1, occ_quad_ctrl + 0x1c);

    return 0;
}

void hero_dev_exe_start(HeroDev *dev) {
    pr_trace("%s snitch_cluster\n", __func__);

    // Set entry-point, bootrom pointer and l3 layout struct pointer
    writew((uint32_t) 0xc0000000             , occ_soc_ctrl + SCTL_SCRATCH_0_REG_OFFSET);
    writew((uint32_t)(uint64_t)0             , occ_soc_ctrl + SCTL_SCRATCH_1_REG_OFFSET);

    fence();

    clint_set_irq(0x1FF << 1);
    pr_trace("%s done\n", __func__);

}

uintptr_t hero_host_l3_malloc(HeroDev *dev, unsigned size_b, uintptr_t *p_addr) {
    struct card_ioctl_arg chunk;
    long err;
    uintptr_t user_virt_address = 0;

    // MMAP requires page granularity
    chunk.size = ALIGN_UP(size_b, 0x1000);
    pr_trace("calling ioctl\n");
    err = ioctl(device_fd, IOCTL_DMA_ALLOC, &chunk);
    pr_trace("done\n");
    if (err) {
        pr_error("%s driver allocator failed\n", __func__);
        return NULL;
    }

    *p_addr = chunk.result_phys_addr;

    if(occamy_mmap(device_fd, DMA_BUFS_MMAP_ID, size_b, &user_virt_address)) {
        pr_error("mmap error!\n");
    }

    pr_trace("%p\n", user_virt_address);

    return user_virt_address;
}
