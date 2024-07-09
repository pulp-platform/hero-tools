
#include <linux/iommu.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/version.h>
#include "hero_iommu.h"

int hero_iommu_region_add(struct iommu_domain *iommu_domain,
                          struct list_head *iommu_region_list, u64 user_addr, u64 iova,
                          u64 length) {
    int ret, i;
    unsigned long nr_pages = length >> PAGE_SHIFT;
    struct page **pages;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,5,0)
    pages =
        (struct page **)kcalloc(nr_pages, sizeof(struct page *), GFP_KERNEL);

    pr_debug("pin_user_pages_fast %llx %llx %x %llx\n", user_addr, nr_pages,
            FOLL_WRITE, pages);

    ret = get_user_pages_fast(user_addr, nr_pages, 0, pages);

    if (ret < nr_pages || nr_pages == 0) {
        pr_err("pin_pages failed (%u)\n", ret);
        return -1;
    }

    for (i = 0; i < nr_pages; i++) {
        iommu_map(iommu_domain, iova + PAGE_SIZE * i,
                  page_to_phys(pages[i]), PAGE_SIZE, IOMMU_READ | IOMMU_WRITE,
                  GFP_KERNEL);
    }

    // Add to the buffer list
    struct hero_iommu_region_node *new =
        kmalloc(sizeof(struct hero_iommu_region_node), GFP_KERNEL);
    new->data = kmalloc(sizeof(struct hero_iommu_region), GFP_KERNEL);
    new->data->user_addr = user_addr;
    new->data->length = length;
    new->data->iova = user_addr;
    list_add_tail(&new->list, iommu_region_list);
    return nr_pages;
#else
    return -1;
#endif
}
