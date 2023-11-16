/*
 * This file is part of the Snitch device driver.
 *
 * Copyright (C) 2022 ETH Zurich, University of Bologna
 *
 * Author: Cyril Koenig <cykoenig@iis.ee.ethz.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "snitch_pcie.h"

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
MODULE_AUTHOR("Cyril Koenig");
MODULE_DESCRIPTION("");
MODULE_VERSION("");

#define BIG_ENDIAN(n)                                                              \
  (((n >> 24) & 0xFFu) | (((n >> 16) & 0xFFu) << 8) | (((n >> 8) & 0xFFu) << 16) | \
   ((n & 0xFFu) << 24))

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

/* Symbol defined in modified XDMA driver */
extern phys_addr_t pci_dma_bypass_bar_start;
// This bar points to the address 0 of the FPGA
phys_addr_t *__pci_dma_bypass_bar_start;

//////////////////////////////////////////

static int bar_read_dtb(struct pci_dev *pci_dev, int bar, u64 offset, void **ret_dtb_blob,
                        u32 *ret_dtb_size) {
  int i;
  u32 dtb_size;
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

  // Copy DTB from FPGA
  dtb_size = BIG_ENDIAN(ioread32(((uint32_t *)dtb_map) + 2));
  *ret_dtb_blob = kzalloc(DTB_MAP_SIZE, GFP_KERNEL);
  if (!*ret_dtb_blob) {
    pr_err("kzalloc failed\n");
    return -ENOMEM;
  }

  for (i = 0; i < dtb_size; i++)
    *(((uint8_t *)*ret_dtb_blob) + i) = *(((uint8_t *)dtb_map) + 4 + i);

  *ret_dtb_size = dtb_size;

  iounmap(dtb_map);

  return 0;
}

static int bar_write_test(struct pci_dev *pci_dev, int bar, u64 offset) {
  int i, val, sum;
  u64 start, size;
  u64 test_start, test_size;
  u32 flags;
  void *ret, *bar_map;

  start = pci_resource_start(pci_dev, bar);
  if (!start) return -EINVAL;
  test_start = start + HBM_0_OFFSET;

  flags = pci_resource_flags(pci_dev, bar);
  if (!(flags & IORESOURCE_MEM)) return -EINVAL;

  size = pci_resource_len(pci_dev, bar);
  test_size = 0;

  pr_info("start %llx, size %llx, flags %x\n", start, size, flags);

  bar_map = ioremap(start + HBM_0_OFFSET, test_size);

  if (!bar_map) {
    pr_err("ioremap failed\n");
    return -ENOMEM;
  }

  val = 0;

  // Write to fpga
  for (i = 0; i < test_size; i++) *(((uint8_t *)bar_map) + 4 + i) = val;

  // Read from fpga
  for (i = 0; i < test_size; i++) sum += *(((uint8_t *)bar_map) + 4 + i);

  pr_info("test done\n");

  iounmap(bar_map);

  return 0;
}

static int of_bar_remap(struct pci_dev *pci_dev, int bar, u64 offset, u32 val[5]) {
  u64 start, size;
  u32 flags;

  start = pci_resource_start(pci_dev, bar);
  if (!start) return -EINVAL;

  flags = pci_resource_flags(pci_dev, bar);
  if (!(flags & IORESOURCE_MEM)) return -EINVAL;

  /* Bus address */
  val[0] = offset;

  /* PCI bus address */
  val[1] = 0x2 << 24;
  val[2] = start >> 32;
  val[3] = start;

  /* Size */
  size = pci_resource_len(pci_dev, bar);
  val[4] = size >> 32;
  val[5] = size;

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

  dev_info(dev, "Read BAR\n");

  ret = bar_read_dtb(pdev, 2, 0, &data->dtb_blob, &data->dtb_size);
  if (ret) return ret;

  dev_info(dev, "Read dtb at %p\n", data->dtb_blob);

  HEXDUMP(((uint32_t *)data->dtb_blob), 8);

  dev_info(dev, "Applying fdt to %p\n", data->dev->of_node);
  ret = of_overlay_fdt_apply_to_node(data->dtb_blob, data->dtb_size, &data->ovcs_id,
                                     data->dev->of_node);
  if (ret) dev_err(dev, "oops of_overlay_fdt_apply_to_node %i\n", ret);

  dev_info(dev, "Getting range infos\n");

  of_bar_remap(data->pci_dev, 2, 0, val[0]);

  dev_info(dev, "Applying ranges changeset\n");

  of_changeset_init(&data->of_cs);
  of_changeset_add_prop_u32_array(&data->of_cs, data->dev->of_node, "ranges", (const u32 *)val,
                                  ARRAY_SIZE(val) * ARRAY_SIZE(val[0]));
  of_changeset_apply(&data->of_cs);

  dev_info(dev, "Populating platform %p\n", data->dev->of_node);

  ret = of_platform_default_populate(data->dev->of_node, NULL, data->dev);
  if (ret) dev_err(dev, "oops of_platform_default_populate %i\n", ret);

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

  of_platform_depopulate(data->dev);

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