#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/iommu.h>
#include <linux/iova.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/kdev_t.h>
#include <linux/log2.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "carfield.h"
#include "carfield_driver.h"

ssize_t card_read(struct file *filp, char __user *buff, size_t count,
                  loff_t *f_pos) {
    struct cardev_private_data *cardev_data =
        (struct cardev_private_data *)filp->private_data;

    int max_size = cardev_data->buffer_size;

    // Adjust the count
    if ((*f_pos + count) > max_size)
        count = max_size - *f_pos;

    // Copy to user
    if (copy_to_user(buff, cardev_data->buffer + (*f_pos), count)) {
        return -EFAULT;
    }

    // Update the current file position
    *f_pos += count;

    return count;
}

int check_permission(int dev_perm, int acc_mode) {
    if (dev_perm == RDWR)
        return 0;

    // ensures readonly access
    if ((dev_perm == RDONLY) &&
        ((acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE)))
        return 0;

    // ensures writeonly access
    if ((dev_perm == WRONLY) &&
        ((acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ)))
        return 0;

    return -EPERM;
}

int card_open(struct inode *inode, struct file *filp) {
    int ret;
    struct cardev_private_data *cardev_data;
    cardev_data = container_of(inode->i_cdev, struct cardev_private_data, cdev);
    filp->private_data = cardev_data;
    ret = check_permission(PDATA_PERM, filp->f_mode);
    return ret;
}

int card_release(struct inode *inode, struct file *filp) {
    // pr_info("release was successful \n");
    return 0;
}

int card_mmap(struct file *filp, struct vm_area_struct *vma) {
    struct k_list *bufs_tail = NULL;
    unsigned long mapoffset, vsize, psize;
    char type[20];
    int ret;
    struct cardev_private_data *cardev_data =
        (struct cardev_private_data *)filp->private_data;

    switch (vma->vm_pgoff) {
    case SOC_CTRL_MMAP_ID:
        MAP_DEVICE_REGION("soc_ctrl", soc_ctrl_mem);
        break;
    case MBOXES_MMAP_ID:
        MAP_DEVICE_REGION("mboxes", mboxes_mem);
        break;
    case L3_MMAP_ID:
        MAP_DEVICE_REGION("l3", l3_mem);
        break;
    case CTRL_REGS_MMAP_ID:
        MAP_DEVICE_REGION("ctrl_regs", ctrl_regs_mem);
        break;
    case L2_INTL_0_MMAP_ID:
        MAP_DEVICE_REGION("l2_intl_0", l2_intl_0_mem);
        break;
    case L2_CONT_0_MMAP_ID:
        MAP_DEVICE_REGION("l2_cont_0", l2_cont_0_mem);
        break;
    case L2_INTL_1_MMAP_ID:
        MAP_DEVICE_REGION("l2_intl_1", l2_intl_1_mem);
        break;
    case L2_CONT_1_MMAP_ID:
        MAP_DEVICE_REGION("l2_cont_1", l2_cont_1_mem);
        break;
    case SPATZ_CLUSTER_MMAP_ID:
        MAP_DEVICE_REGION("spatz_cluster", spatz_cluster_mem);
        break;
    case INTEGER_CLUSTER_MMAP_ID:
        MAP_DEVICE_REGION("integer_cluster", integer_cluster_mem);
        break;
    case SAFETY_ISLAND_MMAP_ID:
        MAP_DEVICE_REGION("safety_island", safety_island_mem);
        break;
    case IDMA_MMAP_ID:
        MAP_DEVICE_REGION("idma", idma_mem);
        break;
    case DMA_BUFS_MMAP_ID:
        strncpy(type, "buffer", sizeof(type));
        pr_info("Ready to map latest buffer\n");
        bufs_tail =
            list_last_entry(&cardev_data->test_head, struct k_list, list);
        mapoffset = bufs_tail->data->pbase;
        psize = bufs_tail->data->size;
        break;
    default:
        pr_err("Unknown page offset\n");
        return -EINVAL;
    }

    vsize = vma->vm_end - vma->vm_start;
    if (vsize > psize) {
        pr_err("error: vsize %ld > psize %ld\n", vsize, psize);
        pr_err("  vma->vm_end %lx vma->vm_start %lx\n", vma->vm_end,
               vma->vm_start);
        // return -EINVAL;
    }

    // set protection flags to avoid caching and paging
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,5,0)
    vm_flags_set(vma, VM_IO);
#else
    vma->vm_flags |= VM_IO | VM_DONTEXPAND | VM_DONTDUMP | VM_PFNMAP;
#endif
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    pr_info("%s mmap: phys: %#lx, virt: %#lx vsize: %#lx psize: %#lx\n", type,
            mapoffset, vma->vm_start, vsize, psize);

    ret = remap_pfn_range(vma, vma->vm_start, mapoffset >> PAGE_SHIFT, vsize,
                          vma->vm_page_prot);

    if (ret)
        pr_info("mmap error: %d\n", ret);

    return ret;
}

static long card_ioctl(struct file *file, unsigned int cmd,
                       unsigned long arg_user_addr) {
    // Pointers to user arguments
    int err;
    void __user *argp = (void __user *)arg_user_addr;
    // Get driver data
    struct cardev_private_data *cardev_data =
        (struct cardev_private_data *)file->private_data;
    // Fetch user arguments
    struct card_ioctl_arg arg;
    if (copy_from_user(&arg, argp, sizeof(struct card_ioctl_arg)))
        return -EFAULT;

    pr_info("Driver IOCTL %u\n", cmd);

    switch (cmd) {
    // Alloc physically contiguous memory
    case IOCTL_DMA_ALLOC: {
        dma_addr_t result_phys = 0;
        void *result_virt = 0;
        printk("dma_alloc_coherent %p, %llx (%llx pages)\n",
               &cardev_data->pdev->dev, arg.size,
               1 << order_base_2(ALIGN(arg.size, PAGE_SIZE) / PAGE_SIZE));
        // Alloc memory region (note PHY address = DMA address), issue with
        // dma_alloc_coherent on milk-v

        result_virt = __get_free_pages(
            GFP_KERNEL | GFP_DMA32,
            order_base_2(ALIGN(arg.size, PAGE_SIZE) / PAGE_SIZE));
        result_phys = virt_to_phys(result_virt);

        // err = iommu_map(cardev_data->iommu_domain, result_virt, result_phys,
        // 	ALIGN(arg.size, PAGE_SIZE), IOMMU_READ | IOMMU_WRITE,
        // GFP_KERNEL);
        // if (err < 0) {
        //     pr_err("iommu_map failed\n");
        // 	return err;
        // }

        // dma_alloc_coherent(&cardev_data->pdev->dev, ALIGN(arg.size,
        // PAGE_SIZE), &result_phys, GFP_KERNEL);

        printk("dma_alloc_coherent returns %llx %llx\n", result_virt,
               result_phys);
        if (!result_virt)
            return -ENOMEM;
        arg.result_virt_addr = result_virt;
        arg.result_phys_addr = result_phys;

        // Offset if there is a PCIe endpoint in the device (then the driver
        // should ran on the PCIe host) The mask removes the host offset to the
        // device's tree
        if (cardev_data->pcie_axi_bar_mem.pbase)
            arg.result_phys_addr +=
                0xffffffff & cardev_data->pcie_axi_bar_mem.pbase - 0x40000000;

        // Add to the buffer list
        struct k_list *new = kmalloc(sizeof(struct k_list), GFP_KERNEL);
        new->data = kmalloc(sizeof(struct shared_mem), GFP_KERNEL);
        new->data->pbase = arg.result_phys_addr;
        new->data->vbase = arg.result_virt_addr;
        new->data->size = arg.size;
        list_add_tail(&new->list, &cardev_data->test_head);

        // Print the buffer list for debug
        pr_info("Reading list :\n");
        struct list_head *p;
        struct k_list *my;
        list_for_each(p, &cardev_data->test_head) {
            my = list_entry(p, struct k_list, list);
            pr_info("pbase = %#llx, psize = %#llx\n", my->data->pbase,
                    my->data->size);
        }
        break;
    }
    case IOCTL_IOMMU_MAP: {
        pr_info("Driver received IOCTL_IOMMU_MAP\n");

        // Here phys_adress to hold the iova (let's put in memory unused by the
        // device)
        arg.result_phys_addr = 0x80000000 + (arg.result_virt_addr % 0x40000000);

        err = hero_iommu_region_add(
            cardev_data->iommu_domain, &cardev_data->iommu_region_list,
            arg.result_virt_addr, arg.result_phys_addr, arg.size);

        if (err < 0) {
            pr_err("hero_iommu_region_add failed\n");
            return err;
        }
        break;
    }
    case IOCTL_MEM_INFOS: {
        pr_info("Lookup %i\n", arg.mmap_id);
        struct shared_mem *requested_mem;
        PTR_TO_DEVDATA_REGION(requested_mem, cardev_data, arg.mmap_id)
        // TODO differenciate errors from uninitialized memory and unknown
        // map_id
        if (!requested_mem) {
            pr_err("Unknown mmap_id %i\n", arg.mmap_id);
            return -1;
        }
        arg.size = requested_mem->size;
        arg.result_phys_addr = requested_mem->pbase;
        break;
    }
    default:
        return -1;
    }

    // Send back result to user
    if (copy_to_user(argp, &arg, sizeof(struct card_ioctl_arg)))
        return -EFAULT;
    return 0;
}

// file operations of the driver
struct file_operations card_fops = {.open = card_open,
                                    .release = card_release,
                                    .read = card_read,
                                    .mmap = card_mmap,
                                    .unlocked_ioctl = card_ioctl,
                                    .owner = THIS_MODULE};
