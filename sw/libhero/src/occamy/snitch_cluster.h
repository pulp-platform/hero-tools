#pragma once

// The virtual addresses of the hardware
static volatile void* occ_quad_ctrl;
static volatile void* occ_soc_ctrl;
static volatile void* occ_snitch_cluster;
static volatile void* occ_l3;
static volatile void* occ_l2;
static volatile void* occ_clint;

struct l3_layout {
  uint32_t a2h_rb;
  uint32_t a2h_mbox;
  uint32_t h2a_mbox;
  uint32_t heap;
};
