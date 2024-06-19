#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/iommu.h>
#include <linux/iova.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/kdev_t.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
// drivers/iommu/riscv
#include "iommu-bits.h"

#include "carfield.h"
#include "carfield_driver.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pulp Platform");
MODULE_DESCRIPTION("Carfield driver");

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt, __func__

struct cardrv_private_data cardrv_data;

// Handle GPIO interrupts to clear the GPIO register
// (Note: other drivers, as ethernet, might handle the same irq)
int already_entered[64];
static irqreturn_t carfield_handle_irq(int irq, void *_pdev) {
    struct platform_device *pdev = _pdev;
    unsigned int pending;
    struct cardev_private_data *dev_data = dev_get_drvdata(&pdev->dev);
    struct irq_desc *desc = irq_data_to_desc(irq_get_irq_data(irq));
    uint32_t old;
    if (!already_entered[irq]) {
        already_entered[irq] = 1;
        return IRQ_NONE;
    }
    if (!desc) {
        pr_err("Could not get hw irq for %i\n", irq);
    }
    int hw_irq = desc->irq_data.hwirq;
    old = ioread32(dev_data->gpio_mem.vbase + 0x00);
    iowrite32(BIT(hw_irq - CARFIELD_GPIO_FIRST_IRQ),
              dev_data->gpio_mem.vbase + 0x00);

    already_entered[irq] = 0;
    return IRQ_HANDLED;
}

int probe_node(struct platform_device *pdev,
               struct cardev_private_data *dev_data, struct shared_mem *result,
               const char *name) {
    int err;
    struct device_node *tmp_np, *tmp_mem_np;
    struct reserved_mem *tmp_mem;
    struct resource tmp_res, tmp_mem_res;

    // Get the node in the DTS
    tmp_np = of_get_child_by_name(pdev->dev.of_node, name);
    if (tmp_np) {

        // Check for reserved L3 memory
        tmp_mem_np = of_parse_phandle(tmp_np, "memory-region", 0);
        if (tmp_mem_np) {
            if (of_address_to_resource(tmp_mem_np, 0, &tmp_mem_res)) {
                pr_err("of_address_to_resource error\n");
                goto probe_node_error;
            }
            if (dev_data->l3_mem.pbase &&
                dev_data->l3_mem.pbase != tmp_mem_res.start) {
                pr_err("I do not support multiple L3 memory regions\n");
            }
            if (!dev_data->l3_mem.pbase) {
                dev_data->l3_mem.vbase =
                    devm_ioremap_resource(&pdev->dev, &tmp_mem_res);
                if (!dev_data->l3_mem.vbase)
                    goto probe_node_error;
                dev_data->l3_mem.pbase = tmp_mem_res.start;
                dev_data->l3_mem.size = resource_size(&tmp_mem_res);
                // Save the informations in the char device (for card_read)
                dev_data->buffer_size +=
                    sprintf(dev_data->buffer + dev_data->buffer_size,
                            "%s: %px size = %x\n", tmp_mem_np->name,
                            dev_data->l3_mem.pbase, dev_data->l3_mem.size);
            }
            pr_info("Found reserved mem\n");
        }

        // Get addresses, remap them and save them to the struct shared_mem
        of_address_to_resource(tmp_np, 0, &tmp_res);
        result->vbase = devm_ioremap_resource(&pdev->dev, &tmp_res);

        if (IS_ERR(result->vbase)) {
            pr_err("Could not map %s io-region\n", name);
        } else {
            result->pbase = tmp_res.start;
            result->size = resource_size(&tmp_res);
            pr_info("Allocated %s io-region (%llx %llx -> %llx)\n", name,
                    result->pbase, result->size, result->vbase);
            // Try to access the first address
            pr_info("Probing %s : (%llx)\n", name, (uint32_t *)result->vbase);
            pr_info(" %x\n", *((uint32_t *)result->vbase));
            if (*((uint32_t *)result->vbase) == 0xbadcab1e) {
                pr_warn("%s not found in hardware (0xbadcab1e)!\n", name);
                *result = (struct shared_mem){0};
            }
            // Save the informations in the char device (for card_read)
            dev_data->buffer_size += sprintf(
                dev_data->buffer + dev_data->buffer_size, "%s: %px size = %x\n",
                name, result->pbase, result->size);
        }
        return err;
    } else {
        pr_err("No %s in device tree\n", name);
    }
probe_node_error:
    *result = (struct shared_mem){0};
    return -1;
}

// gets called when matched platform device
int card_platform_driver_probe(struct platform_device *pdev) {
    int ret, irq, i;
    struct cardev_private_data *dev_data;
    struct irq_desc *tmp_irq_desc;
    struct device *spatz_dev;

    pr_debug("A device is detected \n");

    dev_data = devm_kzalloc(&pdev->dev, sizeof(*dev_data), GFP_KERNEL);
    if (!dev_data) {
        pr_debug("Cannot allocate memory \n");
        return -ENOMEM;
    }

    // Save the device private data pointer in platform device structure
    dev_set_drvdata(&pdev->dev, dev_data);

    pr_debug("Device size = %d\n", PDATA_SIZE);
    pr_debug("Device permission = %d\n", PDATA_PERM);
    pr_debug("Device serial number = %s\n", PDATA_SERIAL);
    pr_debug(
        "platform_device(%p) : name=%s ; id=%i ; id_auto=%i ; dev=%p(%s) ; "
        "num_ressources=%i ; id_entry=%p\n",
        pdev, pdev->name, pdev->id, pdev->id_auto, pdev->dev,
        pdev->dev.init_name, pdev->num_resources, pdev->id_entry);

    /* Dynamically allocate memory for the device buffer using size information
     * from the platform data */
    dev_data->buffer = devm_kzalloc(&pdev->dev, PDATA_SIZE, GFP_KERNEL);
    if (!dev_data->buffer) {
        pr_debug("Cannot allocate memory \n");
        return -ENOMEM;
    }

    // Get the device number
    dev_data->dev_num = cardrv_data.device_num_base + pdev->id;

    // cdev init and cdev add
    cdev_init(&dev_data->cdev, &card_fops);
    dev_data->cdev.owner = THIS_MODULE;

    // Probe soc_ctrl
    probe_node(pdev, dev_data, &dev_data->soc_ctrl_mem, "soc-ctrl");

    // Probe mboxes
    probe_node(pdev, dev_data, &dev_data->mboxes_mem, "mboxes");

    // Probe idma
    probe_node(pdev, dev_data, &dev_data->idma_mem, "idma");

    // Probe pcie-axi-bar-mem
    probe_node(pdev, dev_data, &dev_data->pcie_axi_bar_mem, "pcie-axi-bar-mem");

    // Probe scratch registers
    probe_node(pdev, dev_data, &dev_data->ctrl_regs_mem, "ctrl-regs");

    // Probe gpio and activate rising edge interrupts
    probe_node(pdev, dev_data, &dev_data->gpio_mem, "gpio");

    // Request gpio irqs
    // TODO: Do not do that if running on PCIe host
    for (i = 0; i < CARFIELD_GPIO_N_IRQS; i++) {
        irq = of_irq_get(of_get_child_by_name(pdev->dev.of_node, "gpio"), i);
        if (irq < 0)
            pr_err("Gpio irq %i not found in device tree\n", i);
        ret = request_irq(irq, carfield_handle_irq, IRQF_SHARED, pdev->name,
                          pdev);
        if (ret)
            pr_err("Request gpio irq %i failed with: %i\n", i, ret);
        tmp_irq_desc = irq_data_to_desc(irq_get_irq_data(irq));
        pr_info("Request gpio irq %i (%i) : %i\n", irq,
                tmp_irq_desc->irq_data.hwirq, ret);
    }

    *((uint32_t *)(dev_data->gpio_mem.vbase + 0x04)) = (uint32_t)0xffffffff;
    *((uint32_t *)(dev_data->gpio_mem.vbase + 0x34)) = (uint32_t)0xffffffff;

    // Deisolate all islands
    for (i = ISOLATE_BEGIN_OFFSET; i < ISOLATE_END_OFFSET; i += 4)
        *((uint32_t *)(dev_data->soc_ctrl_mem.vbase + i)) = (uint32_t)0x0;

    // Probe safety_island
    probe_node(pdev, dev_data, &dev_data->safety_island_mem, "safety-island");

    // Probe integer_cluster
    probe_node(pdev, dev_data, &dev_data->integer_cluster_mem,
               "integer-cluster");

    // Probe spatz_cluster
    probe_node(pdev, dev_data, &dev_data->spatz_cluster_mem, "spatz-cluster");
    spatz_dev = &of_find_device_by_node(
                     of_get_child_by_name(pdev->dev.of_node, "spatz-cluster"))
                     ->dev;
    if (spatz_dev) {
        if (dma_set_mask_and_coherent(spatz_dev, DMA_BIT_MASK(32))) {
            pr_err("Cannot get DMA for Spatz\n");
        }
    }

    // Probe L2
    probe_node(pdev, dev_data, &dev_data->l2_intl_0_mem, "l2-intl-0");
    probe_node(pdev, dev_data, &dev_data->l2_cont_0_mem, "l2-cont-0");
    probe_node(pdev, dev_data, &dev_data->l2_intl_1_mem, "l2-intl-1");
    probe_node(pdev, dev_data, &dev_data->l2_cont_1_mem, "l2-cont-1");

    // Probe L3
    probe_node(pdev, dev_data, &dev_data->l3_mem, "l3-buffer");

    // IOMMU region list
    INIT_LIST_HEAD(&dev_data->iommu_region_list);

    // Get IOMMU
    if (iommu_present(&platform_bus_type)) {
        dev_data->iommu_domain = iommu_domain_alloc(&platform_bus_type);
        if (!dev_data->iommu_domain) {
            pr_err("Cannot allocate iommu_domain\n");
            return -ENOMEM;
        }

        ret = iommu_attach_device(dev_data->iommu_domain, spatz_dev);
        if (ret) {
            iommu_domain_free(dev_data->iommu_domain);
            pr_err("iommu_attach_device failed\n");
            return ret;
        }

        ret = iommu_map(dev_data->iommu_domain, dev_data->l2_intl_0_mem.pbase,
                        dev_data->l2_intl_0_mem.pbase,
                        dev_data->l2_intl_0_mem.size, IOMMU_READ | IOMMU_WRITE,
                        GFP_KERNEL);
        ret = iommu_map(dev_data->iommu_domain, dev_data->l2_cont_0_mem.pbase,
                        dev_data->l2_cont_0_mem.pbase,
                        dev_data->l2_cont_0_mem.size, IOMMU_READ | IOMMU_WRITE,
                        GFP_KERNEL);
        ret = iommu_map(
            dev_data->iommu_domain, dev_data->spatz_cluster_mem.pbase,
            dev_data->spatz_cluster_mem.pbase, dev_data->spatz_cluster_mem.size,
            IOMMU_READ | IOMMU_WRITE, GFP_KERNEL);
        ret =
            iommu_map(dev_data->iommu_domain, dev_data->soc_ctrl_mem.pbase,
                      dev_data->soc_ctrl_mem.pbase, dev_data->soc_ctrl_mem.size,
                      IOMMU_READ | IOMMU_WRITE, GFP_KERNEL);
        ret = iommu_map(dev_data->iommu_domain, dev_data->mboxes_mem.pbase,
                        dev_data->mboxes_mem.pbase, dev_data->mboxes_mem.size,
                        IOMMU_READ | IOMMU_WRITE, GFP_KERNEL);
        ret = iommu_map(
            dev_data->iommu_domain, dev_data->pcie_axi_bar_mem.pbase,
            dev_data->pcie_axi_bar_mem.pbase, dev_data->pcie_axi_bar_mem.size,
            IOMMU_READ | IOMMU_WRITE, GFP_KERNEL);
        ret = iommu_map(dev_data->iommu_domain, dev_data->ctrl_regs_mem.pbase,
                        dev_data->ctrl_regs_mem.pbase,
                        dev_data->ctrl_regs_mem.size, IOMMU_READ | IOMMU_WRITE,
                        GFP_KERNEL);
        ret = iommu_map(dev_data->iommu_domain, dev_data->l3_mem.pbase,
                        dev_data->l3_mem.pbase, dev_data->l3_mem.size,
                        IOMMU_READ | IOMMU_WRITE, GFP_KERNEL);

        pr_info("Attached Spatz to IOMMU domain\n");
    }

    // DMA buffer list
    INIT_LIST_HEAD(&dev_data->test_head);

    // Char device
    ret = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);
    if (ret < 0) {
        pr_err("Cdev add failed \n");
        return ret;
    }

    // Create device file for the detected platform device
    cardrv_data.device_card =
        device_create(cardrv_data.class_card, NULL, dev_data->dev_num, NULL,
                      "cardev-%d", pdev->id);
    if (IS_ERR(cardrv_data.device_card)) {
        pr_err("Device create failed \n");
        ret = PTR_ERR(cardrv_data.device_card);
        cdev_del(&dev_data->cdev);
        return ret;
    }

    cardrv_data.total_devices++;

    pr_debug("Probe was successful \n");

    return 0;
}

// gets called when the device is removed from the system
int card_platform_driver_remove(struct platform_device *pdev) {
    struct cardev_private_data *dev_data = dev_get_drvdata(&pdev->dev);
    struct device *spatz_dev;
    int ret;

    // ret = iommu_unmap(dev_data->iommu_domain, dev_data->l2_intl_0_mem.pbase,
    //                   dev_data->l2_intl_0_mem.size);
    // ret = iommu_unmap(dev_data->iommu_domain, dev_data->l2_cont_0_mem.pbase,
    //                   dev_data->l2_cont_0_mem.size);
    // ret = iommu_unmap(dev_data->iommu_domain, dev_data->spatz_cluster_mem.pbase,
    //                   dev_data->spatz_cluster_mem.size);
    // ret = iommu_unmap(dev_data->iommu_domain, dev_data->soc_ctrl_mem.pbase,
    //                   dev_data->soc_ctrl_mem.size);
    // ret = iommu_unmap(dev_data->iommu_domain, dev_data->mboxes_mem.pbase,
    //                   dev_data->mboxes_mem.size);
    // ret = iommu_unmap(dev_data->iommu_domain, dev_data->pcie_axi_bar_mem.pbase,
    //                   dev_data->pcie_axi_bar_mem.size);
    // ret = iommu_unmap(dev_data->iommu_domain, dev_data->ctrl_regs_mem.pbase,
    //                   dev_data->ctrl_regs_mem.size);
    // ret = iommu_unmap(dev_data->iommu_domain, dev_data->l3_mem.pbase,
    //                   dev_data->l3_mem.size);

    spatz_dev = &of_find_device_by_node(
                     of_get_child_by_name(pdev->dev.of_node, "spatz-cluster"))
                     ->dev;
    if (spatz_dev) {
        iommu_detach_device(dev_data->iommu_domain, spatz_dev);
    }
    iommu_domain_free(dev_data->iommu_domain);

    // Remove a device that was created with device_create()
    device_destroy(cardrv_data.class_card, dev_data->dev_num);

    // Remove a cdev entry from the system
    cdev_del(&dev_data->cdev);

    cardrv_data.total_devices--;

    pr_debug("A device is removed \n");

    return 0;
}

static const struct of_device_id carfield_of_match[] = {
    {
        .compatible = "eth,carfield-soc",
    },
    {},
};
MODULE_DEVICE_TABLE(of, carfield_of_match);

struct platform_driver card_platform_driver = {
    .probe = card_platform_driver_probe,
    .remove = card_platform_driver_remove,
    .driver = {
        .name = "eth-carfield",
        .of_match_table = carfield_of_match,
    }};

static const struct of_device_id spatz_of_match[] = {
    {
        .compatible = "spatz-cluster,carfield",
    },
    {},
};
MODULE_DEVICE_TABLE(of, spatz_of_match);

struct platform_driver spatz_platform_driver = {
    .driver = {
        .name = "eth-spatz",
        .of_match_table = spatz_of_match,
    }};

#define MAX_DEVICES 10

static int __init card_platform_driver_init(void) {
    int ret;

    // Dynamically allocate a device number for MAX_DEVICES
    ret = alloc_chrdev_region(&cardrv_data.device_num_base, 0, MAX_DEVICES,
                              "cardevs");
    if (ret < 0) {
        pr_err("Alloc chrdev failed \n");
        return ret;
    }

    // Create a device class under /sys/class
    cardrv_data.class_card = class_create("card_class");
    if (IS_ERR(cardrv_data.class_card)) {
        pr_err("Class creation failed \n");
        ret = PTR_ERR(cardrv_data.class_card);
        unregister_chrdev_region(cardrv_data.device_num_base, MAX_DEVICES);
        return ret;
    }

    // Register Spatz to probe its IOMMU
    platform_driver_register(&spatz_platform_driver);
    platform_driver_register(&card_platform_driver);

    pr_debug("card platform driver loaded \n");

    return 0;
}

static void __exit card_platform_driver_cleanup(void) {
    // Unregister the platform driver
    platform_driver_unregister(&card_platform_driver);
    platform_driver_register(&spatz_platform_driver);

    // Class destroy
    class_destroy(cardrv_data.class_card);

    // Unregister device numbers for MAX_DEVICES
    unregister_chrdev_region(cardrv_data.device_num_base, MAX_DEVICES);

    pr_debug("card platform driver unloaded \n");
}

module_init(card_platform_driver_init);
module_exit(card_platform_driver_cleanup);
