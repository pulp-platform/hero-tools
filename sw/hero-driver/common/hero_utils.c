#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/module.h>

#include "hero_utils.h"

MODULE_LICENSE("GPL");

// Probe a child node of "pdev" in the device tree based on "name".
// This simple probes excpect an entry with only one region.
int probe_node(struct platform_device *pdev, const char *name, struct shared_mem *result, char *buf_result, unsigned int *buf_size_result) {
    struct device_node *tmp_np, *tmp_mem_np;
    struct reserved_mem *tmp_mem;
    struct resource tmp_res, tmp_mem_res;

    // Get the node in the DTS
    tmp_np = of_get_child_by_name(pdev->dev.of_node, name);
    if (tmp_np) {

        // Get addresses, remap them and save them to the struct shared_mem
        of_address_to_resource(tmp_np, 0, &tmp_res);
        result->vbase = devm_ioremap_resource(&pdev->dev, &tmp_res);

        if (IS_ERR(result->vbase)) {
            pr_err("Could not map %s io-region\n", name);
        } else {
            result->pbase = tmp_res.start;
            result->size = resource_size(&tmp_res);
            pr_debug("Allocated %s io-region\n", name);
            // Try to access the first address
            pr_debug("Probing %s : %x\n", name, *((uint32_t *)result->vbase));
            if (*((uint32_t *)result->vbase) == 0xbadcab1e) {
                pr_warn("%s not found in hardware (0xbadcab1e)!\n", name);
                *result = (struct shared_mem) { 0 };
            }
            // Save the informations in the char device (for card_read)
            if(snprintf(NULL, 0, "%s: %px size = %x\n", name, result->pbase, result->size) + *buf_size_result > PDATA_SIZE) {
                pr_err("No more space in chardevice\n");
            } else {
            *buf_size_result += sprintf(
                buf_result + *buf_size_result, "%s: %px size = %x\n",
                name, result->pbase, result->size);
            }
        }
        return 0;
    } else {
        pr_err("No %s in device tree\n", name);
    }
probe_node_error:
    *result = (struct shared_mem) { 0 };
    return -1;
}
