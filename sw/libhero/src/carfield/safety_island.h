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
// safety-island ctrl
#define SAFETY_ISLAND_BOOT_ADDR_OFFSET (0x00200000 + 0x0)
#define SAFETY_ISLAND_FETCH_ENABLE_OFFSET (0x00200000 + 0x4)
#define SAFETY_ISLAND_BOOTMODE_OFFSET (0x00200000 + 0xc)

// The virtual addresses of the hardware
extern volatile void* car_soc_ctrl;
extern volatile void* chs_ctrl_regs;
extern volatile void* chs_idma;
extern volatile void* car_safety_island;
extern volatile void* car_l2_intl_0;
extern volatile void* car_l2_cont_0;
extern volatile void* car_l2_intl_1;
extern volatile void* car_l2_cont_1;
extern volatile void* car_l3;

