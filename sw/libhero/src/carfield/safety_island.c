// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>


#include "debug.h"
#include "herodev.h"
#include "io.h"
#include "safety_island.h"
#include "util.h"

#include <stdint.h>

struct card_alloc_arg {
    size_t size;
    uint64_t result_phys_addr;
    uint64_t result_virt_addr;
};

int carfield_mmap(int device_fd, unsigned int page_offset, size_t length,
                  void **res) {
    *res = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, device_fd,
                page_offset * getpagesize());

    if (*res == MAP_FAILED) {
        printf("mmap() failed %s for offset: %x length: %lx\n", strerror(errno),
               page_offset, length);
        *res = NULL;
        return -EIO;
    }
    return 0;
}

void hero_dev_reset(HeroDev *dev, unsigned full) {
    int err;
    pr_trace("%s safety_island\n", __func__);
    // Isolate
    car_set_isolate(1);
    // Disable fetch enable
    writew(0, car_soc_ctrl + CARFIELD_SAFETY_ISLAND_FETCH_ENABLE_OFFSET);
    fence();
    // Disable clock
    writew(0, car_soc_ctrl + CARFIELD_SAFETY_ISLAND_CLK_EN_OFFSET);
    fence();
    // Reset and de-reset
    writew(1, car_soc_ctrl + CARFIELD_SAFETY_ISLAND_RST_OFFSET);
    fence();
    for (volatile int i = 0; i < 16; i++)
	;
    writew(0, car_soc_ctrl + CARFIELD_SAFETY_ISLAND_RST_OFFSET);
    fence();
    // Enable clock
    writew(1, car_soc_ctrl + CARFIELD_SAFETY_ISLAND_CLK_EN_OFFSET);
    fence();
    // De-isolate
    car_set_isolate(0);
}


// TODO: Removed numerical constants and share some header file
// with the driver
int hero_dev_mmap(HeroDev *dev) {
    int err = 0;
    device_fd = open("/dev/cardev--1", O_RDWR | O_SYNC);
    pr_trace("%s safety_island\n", __func__);
    // Call card_mmap from the driver map address spaces
    if (carfield_mmap(device_fd, 0, 0x1000, &car_soc_ctrl))
        goto error_driver;
    
    if (carfield_mmap(device_fd, 5, 0x1000, &chs_ctrl_regs))
        goto error_driver;

    if (carfield_mmap(device_fd, 10, 0x100000, &car_l2_intl_0))
        goto error_driver;

    if (carfield_mmap(device_fd, 11, 0x100000, &car_l2_cont_0))
        goto error_driver;

    if (carfield_mmap(device_fd, 12, 0x100000, &car_l2_intl_1))
        goto error_driver;

    if (carfield_mmap(device_fd, 13, 0x100000, &car_l2_cont_1))
        goto error_driver;

    if (carfield_mmap(device_fd, 100, 0x800000, &car_safety_island))
        goto error_driver;
    
    // Put the safety island memory map in the local_mems list
    HeroSubDev_t *local_mems_tail = malloc(sizeof(HeroSubDev_t));
    local_mems_tail->v_addr = car_safety_island;
    local_mems_tail->p_addr = 0x60000000;
    local_mems_tail->size   = 0x800000;
    local_mems_tail->alias  = "safety_all";
    dev->local_mems = local_mems_tail;

    // Allocate a xKiB of host memory with card_ioctl in driver
    HeroSubDev_t *global_mems_tail = malloc(sizeof(HeroSubDev_t));
    global_mems_tail->v_addr = hero_dev_l3_malloc(dev, 0x1000, &global_mems_tail->p_addr);
    global_mems_tail->size   = 0x1000;
    global_mems_tail->alias  = "chunk_0";
    if (global_mems_tail->v_addr < 0)
        goto error_driver;
    // Use this host memory as global_mem
    dev->global_mems = global_mems_tail;
    
    goto end;
error_driver:
    err = -1;
    pr_error("Error when communicating with the Carfield driver.\n");
end:
    return err;
}

uintptr_t hero_dev_l3_malloc(HeroDev *dev, unsigned size_b, uintptr_t *p_addr) {
    struct card_alloc_arg chunk;
    pr_trace("%s safety_island\n", __func__);

    chunk.size = size_b;
    uint64_t virt_addr = ioctl(device_fd, 1, &chunk);
    if (virt_addr < 0)
        pr_error("%s driver allocator failed\n", __func__);
    *p_addr = chunk.result_phys_addr;

    pr_trace("%p %p %p %p\n", p_addr, *p_addr, chunk.result_virt_addr, chunk.result_phys_addr);

    uintptr_t result = 0;
    if(carfield_mmap(device_fd, 1, 0x1000, &result)) {
        pr_error("mmap error!\n");
    }

    pr_trace("%p\n", result);

    return result;
}

void hero_dev_exe_start(HeroDev *dev) {
    int err;

    pr_trace("%s safety_island : TODO get bootadress from OMP\n", __func__);

	// Write entry point into boot address
    // Todo get address from openmp
	writew(0x60010080, car_soc_ctrl + CARFIELD_SAFETY_ISLAND_BOOT_ADDR_OFFSET);
    writew(0x60010080, car_safety_island + SAFETY_ISLAND_BOOT_ADDR_OFFSET);
    fence();

	// Assert fetch enable
	writew(1, car_soc_ctrl + CARFIELD_SAFETY_ISLAND_FETCH_ENABLE_OFFSET);
}

int hero_dev_init(HeroDev *dev) {
    pr_trace("safety_island implementation\n");
    // Allocate sw mailboxes
    hero_dev_alloc_mboxes(dev);
    writew(dev->mboxes.h2a_mbox_mem.p_addr, chs_ctrl_regs + 0x0);
    writew(dev->mboxes.a2h_mbox_mem.p_addr, chs_ctrl_regs + 0x4);
    return 0;
}

