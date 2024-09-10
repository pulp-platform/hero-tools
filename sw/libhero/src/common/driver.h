// Copyright 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>

#pragma once

#include <stdint.h>
#include <sys/ioctl.h>

#include "libhero/debug.h"
#include "libhero/utils.h"

// The device driver file
extern int device_fd;

struct driver_ioctl_arg {
    size_t size;
    uint64_t result_phys_addr;
    uint64_t result_virt_addr;
    int mmap_id;
};

int driver_lookup_mem(int device_fd, int mmap_id, size_t *size_b, uintptr_t *p_addr) {
    struct driver_ioctl_arg chunk;
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

int driver_mmap(int device_fd, int mmap_id, size_t length,
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

int driver_lookup_mmap(int device_fd, int mmap_id, void **res) {
    size_t phy_len = 0;
    uintptr_t phy_base = NULL;

    driver_lookup_mem(device_fd, mmap_id, &phy_len, &phy_base);

    return driver_mmap(device_fd, mmap_id, phy_len, res);
}

uintptr_t hero_host_l3_malloc(HeroDev *dev, unsigned size_b, uintptr_t *p_addr) {
    struct driver_ioctl_arg chunk;
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

    if(driver_mmap(device_fd, DMA_BUFS_MMAP_ID, size_b, &user_virt_address)) {
        pr_error("mmap error!\n");
    }

    pr_trace("%p\n", user_virt_address);

    return user_virt_address;
}

#ifdef DEVICE_IOMMU
uintptr_t hero_iommu_map_virt(HeroDev *dev, unsigned size_b, void *v_addr) {
    struct driver_ioctl_arg chunk;
    int err;
    // MMAP requires page granularity
    chunk.size = size_b;
    chunk.result_phys_addr = 0;
    chunk.result_virt_addr = v_addr;
    pr_trace("calling ioctl\n");
    err = ioctl(device_fd, IOCTL_IOMMU_MAP, &chunk);
    pr_trace("done\n");
    if (err) {
        pr_error("%s driver allocator failed\n", __func__);
        return NULL;
    }
    return chunk.result_phys_addr;
}

int hero_iommu_map_virt_to_phys(HeroDev *dev, unsigned size_b, void *v_addr, uintptr_t p_addr) {
    struct driver_ioctl_arg chunk;
    int err;
    // MMAP requires page granularity
    chunk.size = size_b;
    chunk.result_phys_addr = p_addr;
    chunk.result_virt_addr = v_addr;
    pr_trace("calling ioctl\n");
    err = ioctl(device_fd, IOCTL_IOMMU_MAP, &chunk);
    pr_trace("done\n");
    if (err)
        pr_error("%s driver iommu\n", __func__);

    return err;
}
#endif
