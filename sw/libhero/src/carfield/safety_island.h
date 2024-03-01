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
// safety-island ctrl
#define SAFETY_ISLAND_BOOT_ADDR_OFFSET (0x00200000 + 0x0)
#define SAFETY_ISLAND_FETCH_ENABLE_OFFSET (0x00200000 + 0x4)
#define SAFETY_ISLAND_BOOTMODE_OFFSET (0x00200000 + 0xc)

// The Carfield driver device file
int device_fd;

// The virtual addresses of the hardware
volatile void* car_soc_ctrl;
volatile void* chs_ctrl_regs;
volatile void* chs_idma;
volatile void* car_safety_island;
volatile void* car_l2_intl_0;
volatile void* car_l2_cont_0;
volatile void* car_l2_intl_1;
volatile void* car_l2_cont_1;
volatile void* car_l3;

// Physical addresses
uintptr_t             car_pcie_axi_bar_phys;


void car_set_isolate(uint32_t status)
{
    writew(status, car_soc_ctrl + CARFIELD_SAFETY_ISLAND_ISOLATE_OFFSET);
    fence();
    while (readw(car_soc_ctrl + CARFIELD_SAFETY_ISLAND_ISOLATE_STATUS_OFFSET) !=
	   status)
	;
}

#endif // SAFETY_ISLAND_H__
