// Copyright 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>

#pragma once

// soc_ctrl
#define CARFIELD_SAFETY_ISLAND_RST_OFFSET 0x28
#define CARFIELD_SAFETY_ISLAND_ISOLATE_OFFSET 0x40
#define CARFIELD_SAFETY_ISLAND_ISOLATE_STATUS_OFFSET 0x58
#define CARFIELD_SAFETY_ISLAND_CLK_EN_OFFSET 0x70
#define CARFIELD_SAFETY_ISLAND_BOOT_ADDR_OFFSET 0xcc
#define CARFIELD_SAFETY_ISLAND_FETCH_ENABLE_OFFSET 0xb8
#define CARFIELD_SPATZ_CLUSTER_RST_OFFSET 0x34
#define CARFIELD_SPATZ_CLUSTER_ISOLATE_OFFSET 0x4c
#define CARFIELD_SPATZ_CLUSTER_ISOLATE_STATUS_OFFSET 0x64
#define CARFIELD_SPATZ_CLUSTER_CLK_EN_OFFSET 0x7c
#define CARFIELD_SPATZ_CLUSTER_BOOT_ADDR_OFFSET 0xd8
#define CARFIELD_SPATZ_CLUSTER_BUSY_OFFSET 0xe8

// safety-island ctrl
#define SAFETY_ISLAND_BOOT_ADDR_OFFSET (0x00200000 + 0x0)
#define SAFETY_ISLAND_FETCH_ENABLE_OFFSET (0x00200000 + 0x4)
#define SAFETY_ISLAND_BOOTMODE_OFFSET (0x00200000 + 0xc)

// spatz-peripherals
#define CARFIELD_SPATZ_CLUSTER_PERIPHERAL_OFFSET 0x20000
#define CARFIELD_SPATZ_CLUSTER_PERIPHERAL_CLUSTER_BOOT_CONTROL_OFFSET 0x58

// mboxes
#define CARFIELD_MBOX_HOST_2_SPATZ_0_INT_SND_STAT 0x200
#define CARFIELD_MBOX_HOST_2_SPATZ_0_INT_SND_SET  0x204
#define CARFIELD_MBOX_HOST_2_SPATZ_0_INT_SND_CLR  0x208
#define CARFIELD_MBOX_HOST_2_SPATZ_0_INT_SND_EN   0x20c
#define CARFIELD_MBOX_HOST_2_SPATZ_0_INT_RCV_CLR  0x248
#define CARFIELD_MBOX_HOST_2_SPATZ_0_LETTER_0     0x280
#define CARFIELD_MBOX_HOST_2_SPATZ_0_LETTER_1     0x28C

#define CARFIELD_MBOX_HOST_2_SPATZ_1_INT_SND_STAT 0x300
#define CARFIELD_MBOX_HOST_2_SPATZ_1_INT_SND_SET  0x304
#define CARFIELD_MBOX_HOST_2_SPATZ_1_INT_SND_CLR  0x308
#define CARFIELD_MBOX_HOST_2_SPATZ_1_INT_SND_EN   0x30c
#define CARFIELD_MBOX_HOST_2_SPATZ_1_INT_RCV_CLR  0x348
#define CARFIELD_MBOX_HOST_2_SPATZ_1_LETTER_0     0x380
#define CARFIELD_MBOX_HOST_2_SPATZ_1_LETTER_1     0x38C


// The virtual addresses of the hardware
volatile void* car_soc_ctrl;
volatile void* chs_ctrl_regs;
volatile void* car_mboxes;
volatile void* chs_idma;
volatile void* car_safety_island;
volatile void* car_spatz_cluster;
volatile void* car_l2_intl_0;
volatile void* car_l2_cont_0;
volatile void* car_l2_intl_1;
volatile void* car_l2_cont_1;
volatile void* car_l3;

