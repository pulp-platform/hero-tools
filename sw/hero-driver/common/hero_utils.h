#pragma once

#include <linux/of.h>
#include <linux/platform_device.h>

#define RDWR 0x11
#define RDONLY 0x01
#define WRONLY 0x10

#define PDATA_SIZE 2048
#define PDATA_PERM RDWR
#define PDATA_SERIAL "1"

// General description of memory region
struct shared_mem {
    phys_addr_t pbase;
    void __iomem *vbase;
    resource_size_t size;
};

int probe_node(struct platform_device *pdev, const char *name, struct shared_mem *result, char *buf_result, unsigned int *buf_size_result);
