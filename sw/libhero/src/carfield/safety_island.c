// Copyright 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>

#include <stdint.h>

#include "libhero/debug.h"
#include "libhero/hero_api.h"
#include "libhero/io.h"
#include "libhero/utils.h"

#include "allocators.h"
#include "carfield_driver.h"
#include "driver.h"
#include "safety_island.h"

void car_set_isolate(uint32_t status)
{
    writew(status, car_soc_ctrl + CARFIELD_SAFETY_ISLAND_ISOLATE_OFFSET);
    fence();
    while (readw(car_soc_ctrl + CARFIELD_SAFETY_ISLAND_ISOLATE_STATUS_OFFSET) !=
	   status)
	;
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

    char* env_libhero_log = getenv("LIBHERO_LOG");
    if(env_libhero_log)
        libhero_log_level = strtol(env_libhero_log, NULL, 10);

    device_fd = open("/dev/cardev--1", O_RDWR | O_SYNC);
    pr_trace("%s safety_island\n", __func__);
    // Call card_mmap from the driver map address spaces
    if (driver_lookup_mmap(device_fd, SOC_CTRL_MMAP_ID, &car_soc_ctrl))
        goto error_driver;
    
    if (driver_lookup_mmap(device_fd, CTRL_REGS_MMAP_ID, &chs_ctrl_regs))
        goto error_driver;

    if (driver_lookup_mmap(device_fd, L2_INTL_0_MMAP_ID, &car_l2_intl_0))
        goto error_driver;

    if (driver_lookup_mmap(device_fd, L2_CONT_0_MMAP_ID, &car_l2_cont_0))
        goto error_driver;

    if (driver_lookup_mmap(device_fd, L2_INTL_1_MMAP_ID, &car_l2_intl_1))
        goto error_driver;

    if (driver_lookup_mmap(device_fd, L2_CONT_1_MMAP_ID, &car_l2_cont_1))
        goto error_driver;
    
    if (driver_lookup_mmap(device_fd, L3_MMAP_ID, &car_l3))
        goto error_driver;
    
    if (driver_lookup_mmap(device_fd, IDMA_MMAP_ID, &chs_idma))
        goto error_driver;

    if (driver_lookup_mmap(device_fd, SAFETY_ISLAND_MMAP_ID, &car_safety_island))
        goto error_driver;
    
    // Put the safety island memory map in the local_mems list for OpenMP to use
    HeroSubDev_t *local_mems_tail = malloc(sizeof(HeroSubDev_t));
    size_t car_safety_island_size; 
    uintptr_t car_safety_island_phys;
    driver_lookup_mem(device_fd, SAFETY_ISLAND_MMAP_ID, &car_safety_island_size, &car_safety_island_phys);
    if(!local_mems_tail){
        pr_error("Error when allocating local_mems_tail.\n");
        goto error_driver;
    }
    local_mems_tail->v_addr = car_safety_island;
    // TODO: Split lookup between device and host phy addr
    local_mems_tail->p_addr = 0xFFFFFFFF & car_safety_island_phys;
    local_mems_tail->size   = car_safety_island_size;
    local_mems_tail->alias  = "l1_safety_island";
    dev->local_mems = local_mems_tail;

    // Put half of the l2_intl_1 memory map in the global_mems list
    // (We give the first half to openmp and the top half to o1heap)
    size_t car_l2_size; 
    uintptr_t car_l2_phys; 
    driver_lookup_mem(device_fd, L2_INTL_0_MMAP_ID, &car_l2_size, &car_l2_phys);
    HeroSubDev_t *l2_mems_tail = malloc(sizeof(HeroSubDev_t));
    if(!l2_mems_tail){
        pr_error("Error when allocating l2_mems_tail.\n");
        goto error_driver;
    }
    l2_mems_tail->v_addr = car_l2_intl_0;
    // TODO: Split lookup between device and host phy addr
    l2_mems_tail->p_addr = 0xFFFFFFFF & car_l2_phys;
    l2_mems_tail->size   = car_l2_size / 2;
    l2_mems_tail->alias  = "l2_intl_0";
    l2_mems_tail->next   = NULL;
    dev->global_mems = l2_mems_tail;

    // Use the upper half of the device L2 mem for the heap allocator
    // TODO: Get phy addresses from the driver
    l2_heap_start_phy = car_l2_phys + ALIGN_UP(car_l2_size / 2, O1HEAP_ALIGNMENT);
    l2_heap_start_virt = car_l2_intl_0 + ALIGN_UP(car_l2_size / 2, O1HEAP_ALIGNMENT);
    l2_heap_size = car_l2_size / 2;
    pr_trace("%llx %llx %llx %llx\n", car_l2_phys, car_l2_size, car_l2_size / 2, l2_heap_size);
    err = hero_dev_l2_init(dev);
    if(err) {
        pr_error("Error when initializing L2 mem.\n");
        goto end;
    }

    // Use the L3 mem for heap allocator
    // TODO: Get phy addresses from the driver
    size_t car_l3_size; 
    uintptr_t car_l3_phys; 
    driver_lookup_mem(device_fd, L3_MMAP_ID, &car_l3_size, &car_l3_phys);
    l3_heap_start_phy = car_l3_phys + ALIGN_UP(car_l3_size / 2, O1HEAP_ALIGNMENT);
    l3_heap_start_virt = car_l3 + ALIGN_UP(car_l3_size / 2, O1HEAP_ALIGNMENT);
    l3_heap_size = car_l3_size / 2;
    err = hero_dev_l3_init(dev);
    if(err) {
        pr_error("Error when initializing L3 mem.\n");
        goto end;
    }

    goto end;
error_driver:
    err = -1;
    pr_error("Error when communicating with the Carfield driver.\n");
end:
    return err;
}

void hero_dev_exe_start(HeroDev *dev) {
    int err;

    pr_trace("%s safety_island : TODO get bootadress from OMP\n", __func__);

    // Reset Safety Island
    car_set_isolate(1);
    writew(0, car_soc_ctrl + CARFIELD_SAFETY_ISLAND_CLK_EN_OFFSET);
    fence();
    writew(1, car_soc_ctrl + CARFIELD_SAFETY_ISLAND_RST_OFFSET);
    fence();
    for (volatile int i = 0; i < 16; i++);
    writew(0, car_soc_ctrl + CARFIELD_SAFETY_ISLAND_RST_OFFSET);
    fence();
    writew(1, car_soc_ctrl + CARFIELD_SAFETY_ISLAND_CLK_EN_OFFSET);
    fence(); 
    car_set_isolate(0);


	// Write entry point into boot address
    // Todo get address from openmp
	writew(0x60010080, car_soc_ctrl + CARFIELD_SAFETY_ISLAND_BOOT_ADDR_OFFSET);
    writew(0x60010080, car_safety_island + SAFETY_ISLAND_BOOT_ADDR_OFFSET);
    fence();

	// Assert fetch enable
	writew(1, car_soc_ctrl + CARFIELD_SAFETY_ISLAND_FETCH_ENABLE_OFFSET);
}

int hero_dev_init(HeroDev *dev) {
    pr_trace("%s safety_island implementation\n", __func__);
    // Allocate sw mailboxes
    hero_dev_alloc_mboxes(dev);
    writew(dev->mboxes.h2a_mbox_mem.p_addr, chs_ctrl_regs + 0x0);
    writew(dev->mboxes.a2h_mbox_mem.p_addr, chs_ctrl_regs + 0x4);
    return 0;
}

int hero_dev_munmap(HeroDev *dev) {
    int err = 0;
    pr_trace("%p\n", dev);
    hero_dev_free_mboxes(dev);
    close(device_fd);
}
