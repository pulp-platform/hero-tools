#ifndef SAFETY_ISLAND_H__
#define SAFETY_ISLAND_H__

#include "libhero/util.h"

// soc_ctrl
#define CARFIELD_SAFETY_ISLAND_RST_OFFSET 0x28
#define CARFIELD_SAFETY_ISLAND_ISOLATE_OFFSET 0x40
#define CARFIELD_SAFETY_ISLAND_ISOLATE_STATUS_OFFSET 0x58
#define CARFIELD_SAFETY_ISLAND_CLK_EN_OFFSET 0x70
#define CARFIELD_SAFETY_ISLAND_BOOT_ADDR_OFFSET 0xcc
#define CARFIELD_SAFETY_ISLAND_FETCH_ENABLE_OFFSET 0xb8
#define CARFIELD_SPATZ_CLUSTER_RST_OFFSET 0x34
#define CARFIELD_SPATZ_CLUSTER_ISOLATE_OFFSET 0x4c
#define CARFIELD_SPATZ_CLUSTER_ISOLATE_STATUS_OFFSET 0x64
#define CARFIELD_SPATZ_CLUSTER_CLK_EN_OFFSET 0x7c
#define CARFIELD_SPATZ_CLUSTER_BOOT_ADDR_OFFSET 0xd8
#define CARFIELD_SPATZ_CLUSTER_BUSY_OFFSET 0xe8

// safety-island ctrl
#define SAFETY_ISLAND_BOOT_ADDR_OFFSET (0x00200000 + 0x0)
#define SAFETY_ISLAND_FETCH_ENABLE_OFFSET (0x00200000 + 0x4)
#define SAFETY_ISLAND_BOOTMODE_OFFSET (0x00200000 + 0xc)

// spatz-peripherals
#define CARFIELD_SPATZ_CLUSTER_PERIPHERAL_OFFSET 0x20000
#define CARFIELD_SPATZ_CLUSTER_PERIPHERAL_CLUSTER_BOOT_CONTROL_OFFSET 0x58

// mboxes
#define CARFIELD_MBOX_HOST_2_SPATZ_0_INT_SND_STAT 0x200
#define CARFIELD_MBOX_HOST_2_SPATZ_0_INT_SND_SET  0x204
#define CARFIELD_MBOX_HOST_2_SPATZ_0_INT_SND_CLR  0x208
#define CARFIELD_MBOX_HOST_2_SPATZ_0_INT_SND_EN   0x20c
#define CARFIELD_MBOX_HOST_2_SPATZ_0_INT_RCV_STAT 0x240
#define CARFIELD_MBOX_HOST_2_SPATZ_0_INT_RCV_SET  0x244
#define CARFIELD_MBOX_HOST_2_SPATZ_0_INT_RCV_CLR  0x248
#define CARFIELD_MBOX_HOST_2_SPATZ_0_INT_RCV_EN   0x24c
#define CARFIELD_MBOX_HOST_2_SPATZ_0_LETTER_0     0x280
#define CARFIELD_MBOX_HOST_2_SPATZ_0_LETTER_1     0x28C



// The Carfield driver device file
int device_fd;

// The virtual addresses of the hardware
volatile void* car_soc_ctrl;
volatile void* chs_ctrl_regs;
volatile void* car_mboxes;
volatile void* chs_idma;
volatile void* car_safety_island;
volatile void* car_spatz_cluster;
volatile void* car_l2_intl_0;
volatile void* car_l2_cont_0;
volatile void* car_l2_intl_1;
volatile void* car_l2_cont_1;
volatile void* car_l3;

// Physical addresses
uintptr_t             car_pcie_axi_bar_phys;


void car_set_isolate(uint32_t status)
{
    writew(status, car_soc_ctrl + CARFIELD_SPATZ_CLUSTER_ISOLATE_OFFSET);
    fence();
    while (readw(car_soc_ctrl + CARFIELD_SPATZ_CLUSTER_ISOLATE_STATUS_OFFSET) !=
	   status)
	;
}

#endif // SAFETY_ISLAND_H__
