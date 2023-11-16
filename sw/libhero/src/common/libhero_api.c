// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>

#include "debug.h"
#include "herodev.h"
#include "ringbuf.h"
#include "io.h"

#include <stdint.h>

// ----------------------------------------------------------------------------
//
//   Macros
//
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//
//   Global Data
//
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//
//   Static
//
// ----------------------------------------------------------------------------

static int memtest(void *mem, size_t size, const char *cmt, uint8_t fillPat) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

static void *fesrv_run_wrap(void *fs) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

// ----------------------------------------------------------------------------
//
//   RTL interface
//
// ----------------------------------------------------------------------------

uint32_t hero_dev_read32(const volatile uint32_t *base_addr, uint32_t off,
                         char off_type) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

void hero_dev_write32(volatile uint32_t *base_addr, uint32_t off, char off_type,
                      uint32_t value) {
    pr_error("%s unimplemented\n", __func__);
}

int hero_dev_mbox_read(const HeroDev *dev, uint32_t *buffer, size_t n_words) {
    int ret, retry = 0;
    pr_trace("%s default\n", __func__);
    pr_trace("dev->mboxes.a2h_mbox = %p\n", dev->mboxes.a2h_mbox);

    while (n_words--) {
        do {
            ret = rb_host_get(dev->mboxes.a2h_mbox, &buffer[n_words]);
            if (ret) {
                if (++retry == 100)
                    pr_warn("high retry on mbox read()\n");
                usleep(10000);
            }
        } while (ret);
    }

    return 0;
}

int hero_dev_mbox_write(HeroDev *dev, uint32_t word) {
    int ret, retry = 0;
    pr_trace("%s default\n", __func__);
    pr_trace("dev->mboxes.h2a_mbox = %p\n", dev->mboxes.h2a_mbox);

    do {
        ret = rb_host_put(dev->mboxes.h2a_mbox, &word);
        if (ret) {
            if (++retry == 100)
                pr_warn("high retry on mbox write()\n");
        usleep(10000);
        }
    } while (ret);
    
    return ret;
}

int hero_dev_reserve_v_addr(HeroDev *dev) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_free_v_addr(const HeroDev *dev) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

void hero_dev_print_v_addr(HeroDev *dev) {
    pr_error("%s unimplemented\n", __func__);
}

__attribute__((weak)) int hero_dev_mmap(HeroDev *dev) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_alloc_mboxes(HeroDev *dev) {
    pr_trace("%s default\n", __func__);
    dev->mboxes.a2h_mbox = hero_dev_l3_malloc(dev, sizeof(struct ring_buf), &dev->mboxes.a2h_mbox_mem.p_addr);
    dev->mboxes.a2h_mbox->data_v = hero_dev_l3_malloc(dev, sizeof(uint32_t)*16, &dev->mboxes.a2h_mbox->data_p);
    rb_init(dev->mboxes.a2h_mbox, 16, sizeof(uint32_t));

    dev->mboxes.h2a_mbox = hero_dev_l3_malloc(dev, sizeof(struct ring_buf), &dev->mboxes.h2a_mbox_mem.p_addr);
    dev->mboxes.h2a_mbox->data_v = hero_dev_l3_malloc(dev, sizeof(uint32_t)*16, &dev->mboxes.h2a_mbox->data_p);
    rb_init(dev->mboxes.h2a_mbox, 16, sizeof(uint32_t));

    if(dev->mboxes.a2h_mbox < 0 || dev->mboxes.h2a_mbox < 0) {
        return -1;
    }
    return 0;
}

int hero_dev_free_mboxes(HeroDev *dev) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_munmap(HeroDev *dev) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_clking_set_freq(HeroDev *dev, unsigned des_freq_mhz) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_clking_measure_freq(HeroDev *dev) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

__attribute__((weak)) int hero_dev_init(HeroDev *dev) {

    pr_error("%s unimplemented\n", __func__);
}

__attribute__((weak)) void hero_dev_reset(HeroDev *dev, unsigned full) {
    pr_error("%s unimplemented\n", __func__);
}

int hero_dev_get_nb_pe(HeroDev *dev) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_boot(HeroDev *dev, const TaskDesc *task) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_load_bin(HeroDev *dev, const char *name) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_load_bin_from_mem(HeroDev *dev, void *ptr, unsigned size) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

__attribute__((weak)) void hero_dev_exe_start(HeroDev *dev) {
    pr_error("%s unimplemented\n", __func__);
    while(1) {}
}

void hero_dev_exe_stop(HeroDev *dev) {
    pr_error("%s unimplemented\n", __func__);
}

int hero_dev_exe_wait(const HeroDev *dev, int timeout_s) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_rab_req(const HeroDev *dev, unsigned addr_start, unsigned size_b,
                     unsigned char prot, unsigned char port,
                     unsigned char date_exp, unsigned char date_cur,
                     unsigned char use_acp, unsigned char rab_lvl) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

void hero_dev_rab_free(const HeroDev *dev, unsigned char date_cur) {
    pr_error("%s unimplemented\n", __func__);
}

int hero_dev_rab_req_striped(const HeroDev *dev, const TaskDesc *task,
                             ElemPassType **pass_type, int n_elements) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

void hero_dev_rab_free_striped(const HeroDev *dev) {
    pr_error("%s unimplemented\n", __func__);
}

int hero_dev_rab_soc_mh_enable(const HeroDev *dev,
                               const unsigned static_2nd_lvl_slices) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_rab_soc_mh_disable(const HeroDev *dev) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_rab_mh_enable(const HeroDev *dev, unsigned char use_acp,
                           unsigned char rab_mh_lvl) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_rab_mh_disable(const HeroDev *dev) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_smmu_enable(const HeroDev *dev, const unsigned char flags) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_smmu_disable(const HeroDev *dev) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_rab_ax_log_read(const HeroDev *dev) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_profile_info(const HeroDev *dev, ProfileInfo *profile_info) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_out(HeroDev *dev, TaskDesc *task) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_in(HeroDev *dev, TaskDesc *task) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_get_pass_type(const TaskDesc *task,
                                   ElemPassType **pass_type) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_rab_setup(const HeroDev *dev, const TaskDesc *task,
                               ElemPassType **pass_type, int n_ref) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_rab_free(const HeroDev *dev, const TaskDesc *task,
                              const ElemPassType **pass_type, int n_ref) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_pass_desc(HeroDev *dev, const TaskDesc *task,
                               const ElemPassType **pass_type) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_get_desc(const HeroDev *dev, TaskDesc *task,
                              const ElemPassType **pass_type) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_l3_copy_raw_out(HeroDev *dev, TaskDesc *task,
                                     const ElemPassType **pass_type) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_l3_copy_raw_in(HeroDev *dev, const TaskDesc *task,
                                    const ElemPassType **pass_type) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

__attribute__((weak)) uintptr_t hero_dev_l3_malloc(HeroDev *dev, unsigned size_b, uintptr_t *p_addr) {
    pr_error("%s unimplemented\n", __func__);
    return -1;
}

void hero_dev_l3_free(HeroDev *dev, uintptr_t v_addr, uintptr_t p_addr) {
    pr_error("%s unimplemented\n", __func__);
}

int hero_dev_dma_xfer(const HeroDev *dev, uintptr_t addr_l3,
                      uintptr_t addr_pulp, size_t size_b, int host_read) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_omp_offload_task(HeroDev *dev, TaskDesc *task) {
    pr_error("%s unimplemented\n", __func__);
    return 0;
}
