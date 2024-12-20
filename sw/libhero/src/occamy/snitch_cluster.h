// Copyright 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>

#pragma once

// The virtual addresses of the hardware
static volatile void* occ_quad_ctrl;
static volatile void* occ_soc_ctrl;
static volatile void* occ_snitch_cluster;
static volatile void* occ_l3;
static volatile void* occ_l2;
static volatile void* occ_clint;

struct l3_layout {
  uint32_t a2h_rb;
  uint32_t a2h_mbox;
  uint32_t h2a_mbox;
  uint32_t heap;
};

#define QCTL_RESET_N_REG_OFFSET 0x4
#define QCTL_ISOLATE_REG_OFFSET 0x8
#define QCTL_TLB_WIDE_ENABLE_OFFSET 0x18
#define QCTL_TLB_NARROW_ENABLE_OFFSET 0x1c
#define QCTL_TLB_NARROW_REG_OFFSET 0x0800
#define QCTL_TLB_WIDE_REG_OFFSET 0x1000

#define QCTL_TLB_REG_STRIDE 0x20

#define QCTL_ISOLATE_NARROW_IN_BIT 0
#define QCTL_ISOLATE_NARROW_OUT_BIT 1
#define QCTL_ISOLATE_WIDE_IN_BIT 2
#define QCTL_ISOLATE_WIDE_OUT_BIT 3

#define SCTL_SCRATCH_0_REG_OFFSET 0x14
#define SCTL_SCRATCH_1_REG_OFFSET 0x18
#define SCTL_SCRATCH_2_REG_OFFSET 0x1c
#define SCTL_SCRATCH_3_REG_OFFSET 0x20
