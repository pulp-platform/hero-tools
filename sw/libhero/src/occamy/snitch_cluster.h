#pragma once

#include "libhero/util.h"

// quadrant_ctrl
#define OCCAMY_QUADRANT_S1_RESET_N_REG_OFFSET 0x4
#define OCCAMY_QUADRANT_S1_ISOLATE_REG_OFFSET 0x8

// The virtual addresses of the hardware
static volatile void* occ_quad_ctrl;

inline void occ_set_isolate(uint32_t status)
{
    uint32_t val = status ? 0x4 : 0x0;

    writew(val, occ_quad_ctrl + OCCAMY_QUADRANT_S1_ISOLATE_REG_OFFSET);
    fence();
}

// Active low reset
inline void occ_set_reset(uint32_t status)
{
    writew(val ? 0 : 1, occ_quad_ctrl + OCCAMY_QUADRANT_S1_RESET_N_REG_OFFSET);
}

// The Occamy driver device file
int device_fd;