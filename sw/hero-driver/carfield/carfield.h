#pragma once


// General description of memory region
struct shared_mem {
    phys_addr_t pbase;
    void __iomem *vbase;
    resource_size_t size;
};

struct k_list {
    struct list_head list;
    struct shared_mem *data;
};

// Device private data structure
struct cardev_private_data {
    struct platform_device *pdev;
    struct shared_mem idma_mem;
    struct shared_mem soc_ctrl_mem;
    struct shared_mem mboxes_mem;
    struct shared_mem ctrl_regs_mem;
    struct shared_mem gpio_mem;
    struct shared_mem l2_intl_0_mem;
    struct shared_mem l2_cont_0_mem;
    struct shared_mem l2_intl_1_mem;
    struct shared_mem l2_cont_1_mem;
    struct shared_mem safety_island_mem;
    struct shared_mem integer_cluster_mem;
    struct shared_mem spatz_cluster_mem;
    struct shared_mem l3_mem;
    // Not accessible from the host (> 4GB)
    struct shared_mem pcie_axi_bar_mem;
    // DMA buffer list
    struct list_head test_head;
    // Char device
    char *buffer;
    unsigned int buffer_size;
    dev_t dev_num;
    struct cdev cdev;
};

// Driver private data structure
struct cardrv_private_data {
    int total_devices;
    dev_t device_num_base;
    struct class *class_card;
    struct device *device_card;
};

// File operations (carfield_fops.c)
extern struct file_operations card_fops;

// File
#define RDWR 0x11
#define RDONLY 0x01
#define WRONLY 0x10

#define PDATA_SIZE 2048
#define PDATA_PERM RDWR
#define PDATA_SERIAL "1"

// Memmap macro
#define MAP_DEVICE_REGION(NAME, MEM_ENTRY)                                     \
    strncpy(type, #NAME, sizeof(type));                                        \
    pr_info("Ready to map " #NAME);                                            \
    mapoffset = cardev_data->MEM_ENTRY.pbase;                                  \
    psize = cardev_data->MEM_ENTRY.size;

#define ISOLATE_BEGIN_OFFSET 15 * 4
#define ISOLATE_END_OFFSET 20 * 4
#define CARFIELD_GPIO_N_IRQS 4
#define CARFIELD_GPIO_FIRST_IRQ 19
