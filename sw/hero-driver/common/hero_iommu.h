// Copyright 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: GPL-2.0 OR Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>

#pragma once

#include <linux/types.h>

struct hero_iommu_region {
    u64 user_addr;
    u64 length;
    u64 iova;
    struct page **pages;
};

struct hero_iommu_region_node {
    struct list_head list;
    struct hero_iommu_region *data;
};

int hero_iommu_region_add(struct iommu_domain *iommu_domain,
                          struct list_head *iommu_region_list, u64 user_addr, u64 iova,
                          u64 length);
