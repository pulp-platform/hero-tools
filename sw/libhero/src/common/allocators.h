// Copyright 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>

#pragma once

#include <inttypes.h>

#include "o1heap.h"

extern struct O1HeapInstance *l2_heap_manager;
extern uint64_t l2_heap_start_phy, l2_heap_start_virt, l2_heap_size;
extern struct O1HeapInstance *l3_heap_manager;
extern uint64_t l3_heap_start_phy, l3_heap_start_virt, l3_heap_size;
