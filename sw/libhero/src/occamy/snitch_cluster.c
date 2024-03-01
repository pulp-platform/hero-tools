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

#include "snitch_cluster.h"

#include <stdint.h>

void hero_dev_reset(HeroDev *dev, unsigned full) {
    int err;
    pr_trace("%s snitch_cluster\n", __func__);
    // Isolate
    // occ_set_isolate(1);
    // Reset
    writew(0, occ_quad_ctrl + OCCAMY_QUADRANT_S1_RESET_N_REG_OFFSET);
    fence();
    for (volatile int i = 0; i < 16; i++)
	;
    // De-reset
    writew(1, occ_quad_ctrl + OCCAMY_QUADRANT_S1_RESET_N_REG_OFFSET);
    fence();
    // De-isolate
    // occ_set_isolate(0);
}

int occamy_mmap(int device_fd, unsigned int page_offset, size_t length,
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

// TODO: Removed numerical constants and share some header file
// with the driver
int hero_dev_mmap(HeroDev *dev) {
    int err = 0;
    device_fd = open("/dev/occamydev--1", O_RDWR | O_SYNC);
    pr_trace("%s snitch_cluster\n", __func__);
    // Call card_mmap from the driver map address spaces
    if (occamy_mmap(device_fd, 0, 0x1000, &occ_quad_ctrl))
        goto error_driver;
    if (occamy_mmap(device_fd, 0, 0x1000, &occ_quad_ctrl))
        goto error_driver;
    if (occamy_mmap(device_fd, 0, 0x1000, &occ_quad_ctrl))
        goto error_driver;
    
    // Put the safety island memory map in the local_mems list
    HeroSubDev_t *local_mems_tail = malloc(sizeof(HeroSubDev_t));
    local_mems_tail->v_addr = 0;
    local_mems_tail->p_addr = 0;
    local_mems_tail->size   = 0;
    local_mems_tail->alias  = "cluster_l1";
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
