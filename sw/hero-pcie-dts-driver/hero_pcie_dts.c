// Copyright 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: GPL-2.0 OR Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>

#include "hero_pcie_dts.h"

#include <asm/io.h>  // ioremap, iounmap, iowrite32
#include <linux/init.h>
#include <linux/kernel.h>  // Needed for KERN_INFO
#include <linux/module.h>  // Needed by all modules
#include <linux/of.h>      // struct device_type
#include <linux/of_fdt.h>  // of_fdt_unflatten_tree
#include <linux/of_platform.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/slab.h>  // kmalloc()

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pulp Platform");
MODULE_DESCRIPTION("");
MODULE_VERSION("");

#define HEXDUMP_IO(addr, len)                                          \
  do {                                                                 \
    for (i = 0; i < len; i++) {                                        \
      if (i % 4 == 0) printk(KERN_CONT "\n%p (%04x) : ", addr + i, i); \
      printk(KERN_CONT " %08x - ", ioread32(addr + i));                \
    }                                                                  \
  } while (0)

#define HEXDUMP(addr, len)                                             \
  do {                                                                 \
    for (i = 0; i < len; i++) {                                        \
      if (i % 4 == 0) printk(KERN_CONT "\n%p (%04x) : ", addr + i, i); \
      printk(KERN_CONT " %08x - ", *(addr + i));                       \
    }                                                                  \
  } while (0)

//////////////////////////////////////////

static int bar_read_dtb(struct pci_dev *pci_dev, int bar, u64 offset, void **ret_dtb_blob,
                        u32 *ret_dtb_size) {
  int i;
  u32 dtb_size, dtb_magic;
  u64 start, size;
  u32 flags;
  void *dtb_map;

  start = pci_resource_start(pci_dev, bar);
  if (!start) return -EINVAL;

  flags = pci_resource_flags(pci_dev, bar);
  if (!(flags & IORESOURCE_MEM)) return -EINVAL;

  size = pci_resource_len(pci_dev, bar);

  dtb_map = ioremap(start + DTB_OFFSET, DTB_MAP_SIZE);

  if (!dtb_map) {
    pr_err("ioremap failed\n");
    return -ENOMEM;
  }

  dtb_magic = be32_to_cpu(ioread32(((__be32 *)dtb_map)));
  if(dtb_magic != 0xd00dfeed) {
   pr_err("bad dtb magic\n");
    return -EINVAL;
  }

  // Copy DTB from FPGA
  dtb_size = be32_to_cpu(ioread32(((__be32 *)dtb_map) + 1));
  *ret_dtb_blob = kzalloc(DTB_MAP_SIZE, GFP_KERNEL);
  if (!*ret_dtb_blob) {
    pr_err("kzalloc failed\n");
    return -ENOMEM;
  }

  for (i = 0; i < dtb_size; i++)
    *(((uint8_t *)*ret_dtb_blob) + i) = *(((uint8_t *)dtb_map) + 0 + i);

  *ret_dtb_size = dtb_size;

  iounmap(dtb_map);

  return 0;
}

//////////////////////////////////////////

struct hero_pci {
  struct device *dev;
  struct pci_dev *pci_dev;
  struct of_changeset of_cs;
  int ovcs_id;
  void *dtb_blob;
  u32 dtb_size;
  bool populated;
};

// extern struct device_node *of_root;

static int hero_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id) {
  int i;
  struct device_node *ppnode, *np = NULL;
  const char *pci_type = "dev";
  struct device *dev = &pdev->dev;
  struct hero_pci *data;
  struct of_changeset *cset = NULL;
  const char *name;
  int ret;
  struct pci_dev *pdev_2;
  u32 val[1][6];

  if (!of_root) {
    dev_err(dev, "Missing of_root\n");
    // return -EINVAL;
  }

  if (!dev->of_node) {
    dev_err(dev, "Missing of_node for device\n");
    // return -EINVAL;
  } else {
    pci_info(pdev, "parent pdev = %p, parent dev = %p, parent of_node = %p\n", pdev->bus->self,
             &pdev->bus->self->dev, pdev->bus->self->dev.of_node);
    pdev_2 = pdev->bus->self;
    pci_info(pdev_2, "parent bus = %p pdev = %p dev.name = %s, grand parent bus %p\n", pdev_2->bus,
             pdev_2->bus->self, pdev_2->bus->dev.init_name, pdev_2->bus->parent);
  }

  dev_info(dev, "Enabling PCI\n");

  ret = pcim_enable_device(pdev);
  if (ret) return ret;

  pci_set_master(pdev);

  data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
  if (!data) return -ENOMEM;

  dev_set_drvdata(dev, data);
  data->dev = dev;
  data->pci_dev = pdev;
  data->ovcs_id = -1;
  data->populated = 0;

  dev_info(dev, "Read BAR\n");

  ret = bar_read_dtb(pdev, 0, 0, &data->dtb_blob, &data->dtb_size);
  if (ret) return ret;

  dev_info(dev, "Read dtb at %p\n", data->dtb_blob);

  HEXDUMP(((uint32_t *)data->dtb_blob), 8);

  dev_info(dev, "Applying fdt to %p\n", data->dev->parent->of_node);
  ret = of_overlay_fdt_apply(data->dtb_blob, data->dtb_size, &data->ovcs_id,
                             data->dev->parent->of_node);
  if (ret) dev_err(dev, "oops of_overlay_fdt_apply_to_node %i\n", ret);

  dev_info(dev, "Populating platform %p\n", data->dev->of_node);

  ret = of_platform_default_populate(data->dev->of_node, NULL, data->dev);
  if (ret) dev_err(dev, "oops of_platform_default_populate %i\n", ret);
  else data->populated = 1;

  dev_info(dev, "Done\n");

  return 0;

error_overlay_remove:
  of_overlay_remove(&data->ovcs_id);
  return ret;
}

static void hero_pci_remove(struct pci_dev *pdev) {
  struct device *dev = &pdev->dev;
  struct hero_pci *data = dev_get_drvdata(dev);

  if (data->dtb_blob) kfree(data->dtb_blob);

  if (data->populated) of_platform_depopulate(data->dev);

  if (data->ovcs_id > 0) of_overlay_remove(&data->ovcs_id);

  pci_clear_master(pdev);
}

/* PCI device */
static struct pci_device_id hero_ids[] = {{
                                              PCI_DEVICE(0x10ee, 0x9014),
                                          },
                                          {
                                              0,
                                          }};
MODULE_DEVICE_TABLE(pci, hero_ids);

static struct pci_driver hero_pci_driver = {
    .name = "hero_pci",
    .id_table = hero_ids,
    .probe = hero_pci_probe,
    .remove = hero_pci_remove,
};

static int __init hero_pci_driver_init(void) {
  int ret;

  ret = pci_register_driver(&hero_pci_driver);

  return ret;
}
module_init(hero_pci_driver_init);

static void __exit hero_pci_driver_exit(void) { pci_unregister_driver(&hero_pci_driver); }
module_exit(hero_pci_driver_exit);
