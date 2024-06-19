#ifndef __HERO_IOMMU_H
#define __HERO_IOMMU_H

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

#endif
