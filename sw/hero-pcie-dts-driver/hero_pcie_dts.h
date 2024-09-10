// Copyright 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: GPL-2.0 OR Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>

#pragma once

#define DRIVER_NAME "eth_snitch_cluster"

#define DTB_OFFSET   0xD0000000

// Arbitrary allow 10KiB to map were the DTB is written
#define DTB_MAP_SIZE 1024 * 10
