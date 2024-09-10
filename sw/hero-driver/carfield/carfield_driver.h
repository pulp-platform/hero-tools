// Copyright 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: GPL-2.0 OR Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>

#ifndef __CARFIELD_DRIVER_H
#define __CARFIELD_DRIVER_H

#include <linux/ioctl.h>

// Memmap offsets, used for mmap and ioctl
#define SOC_CTRL_MMAP_ID 0
#define DMA_BUFS_MMAP_ID 1
#define L3_MMAP_ID 2
#define MBOXES_MMAP_ID 3
#define CTRL_REGS_MMAP_ID 5
#define L2_INTL_0_MMAP_ID 10
#define L2_CONT_0_MMAP_ID 11
#define L2_INTL_1_MMAP_ID 12
#define L2_CONT_1_MMAP_ID 13
#define IDMA_MMAP_ID 20
#define SAFETY_ISLAND_MMAP_ID 100
#define INTEGER_CLUSTER_MMAP_ID 200
#define SPATZ_CLUSTER_MMAP_ID 300

// For now keep all the ioctl args in the same struct
struct card_ioctl_arg {
    size_t size;
    uint64_t result_phys_addr;
    uint64_t result_virt_addr;
    int mmap_id; // ioctl 2
};

// TODO: Define properly with the Linux API
#define IOCTL_DMA_ALLOC _IOWR('C', 1, struct card_ioctl_arg *)
#define IOCTL_MEM_INFOS _IOWR('C', 2, struct card_ioctl_arg *)
#define IOCTL_IOMMU_MAP _IOWR('C', 3, struct card_ioctl_arg *)

#define PTR_TO_DEVDATA_REGION(VAR, DEVDATA, X)                                 \
    switch (X) {                                                               \
    case (SOC_CTRL_MMAP_ID):                                                   \
        VAR = &DEVDATA->soc_ctrl_mem;                                          \
        break;                                                                 \
    case (MBOXES_MMAP_ID):                                                     \
        VAR = &DEVDATA->mboxes_mem;                                            \
        break;                                                                 \
    case (L3_MMAP_ID):                                                         \
        VAR = &DEVDATA->l3_mem;                                                \
        break;                                                                 \
    case (CTRL_REGS_MMAP_ID):                                                  \
        VAR = &DEVDATA->ctrl_regs_mem;                                         \
        break;                                                                 \
    case (L2_INTL_0_MMAP_ID):                                                  \
        VAR = &DEVDATA->l2_intl_0_mem;                                         \
        break;                                                                 \
    case (L2_CONT_0_MMAP_ID):                                                  \
        VAR = &DEVDATA->l2_cont_0_mem;                                         \
        break;                                                                 \
    case (L2_INTL_1_MMAP_ID):                                                  \
        VAR = &DEVDATA->l2_intl_1_mem;                                         \
        break;                                                                 \
    case (L2_CONT_1_MMAP_ID):                                                  \
        VAR = &DEVDATA->l2_cont_1_mem;                                         \
        break;                                                                 \
    case (IDMA_MMAP_ID):                                                       \
        VAR = &DEVDATA->idma_mem;                                              \
        break;                                                                 \
    case (SAFETY_ISLAND_MMAP_ID):                                              \
        VAR = &DEVDATA->safety_island_mem;                                     \
        break;                                                                 \
    case (INTEGER_CLUSTER_MMAP_ID):                                            \
        VAR = &DEVDATA->integer_cluster_mem;                                   \
        break;                                                                 \
    case (SPATZ_CLUSTER_MMAP_ID):                                              \
        VAR = &DEVDATA->spatz_cluster_mem;                                     \
        break;                                                                 \
    default:                                                                   \
        VAR = NULL;                                                            \
        break;                                                                 \
    }

#endif
