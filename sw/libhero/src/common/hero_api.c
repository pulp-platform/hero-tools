// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>

#include "libhero/debug.h"
#include "libhero/herodev.h"
#include "libhero/ringbuf.h"
#include "libhero/io.h"

// Open trace file
#include <sys/fcntl.h>

#include <stdint.h>
#include <time.h>

int libhero_log_level = LOG_MAX;

// Stucture containing the *device* L2 and L3 allocator
struct O1HeapInstance *l2_heap_manager, *l3_heap_manager;
uintptr_t l2_heap_start_phy, l2_heap_start_virt;
size_t l2_heap_size;
uintptr_t l3_heap_start_phy, l3_heap_start_virt;
size_t l3_heap_size;

struct hero_timestamp time_list[MAX_TIMESTAMPS];
int hero_num_time_list = 0;
int hero_num_device_cycles;
uint32_t hero_device_cycles[MAX_TIMESTAMPS];

int hero_marker_fd;

static void sub_timespec(struct timespec t1, struct timespec t2, struct timespec *td) {
    td->tv_nsec = t2.tv_nsec - t1.tv_nsec;
    td->tv_sec = t2.tv_sec - t1.tv_sec;
    if (td->tv_sec > 0 && td->tv_nsec < 0) {
        td->tv_nsec += NS_PER_SECOND;
        td->tv_sec--;
    } else if (td->tv_sec < 0 && td->tv_nsec > 0) {
        td->tv_nsec -= NS_PER_SECOND;
        td->tv_sec++;
    }
}

void hero_add_timestamp(char str_info[32], char str_func[32], int add_to_ftrace) {
    if (hero_num_time_list >= MAX_TIMESTAMPS) {
        printf("%s : Max timestamps\n\r", __func__);
        return;
    }
    if (!hero_marker_fd) {
        hero_marker_fd = open("/sys/kernel/tracing/trace_marker", O_WRONLY);
    }
#if HERO_TIMESTAMP_MODE == 0
    asm volatile ("csrr %0,cycle"   : "=r" (time_list[hero_num_time_list].cycle));
#elif HERO_TIMESTAMP_MODE == 1
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_list[hero_num_time_list].timespec);
#endif
    memcpy(time_list[hero_num_time_list].str_info, str_info, 32 * sizeof(char));
    memcpy(time_list[hero_num_time_list].str_func, str_func, 32 * sizeof(char));
    // Add to ftrace to compare with context switch
    if(add_to_ftrace)
        write(hero_marker_fd, str_info, 32);
    hero_num_time_list++;
}

void hero_print_timestamp() {
#if HERO_TIMESTAMP_MODE == 0
    printf("info function cycle diff\n");
    for(int i = 0; i < hero_num_time_list; i++) {
        printf("%s %s %lu %lu\n", time_list[i].str_info, time_list[i].str_func, time_list[i].cycle, (i<hero_num_time_list-1) ? time_list[i+1].cycle - time_list[i].cycle : 0);
    }
#elif HERO_TIMESTAMP_MODE == 1
    printf("info function time diff\n");
    struct timespec diff = {0};
    for(int i = 0; i < hero_num_time_list; i++) {
        if (i < hero_num_time_list - 1)
            sub_timespec(time_list[i].timespec, time_list[i+1].timespec, &diff);
        printf("%s %s %d.%.9ld %d.%.9ld\n", time_list[i].str_info, time_list[i].str_func, time_list[i].timespec.tv_sec, time_list[i].timespec.tv_nsec, diff.tv_sec, diff.tv_nsec);
    }
#endif
}

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
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

static void *fesrv_run_wrap(void *fs) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

// ----------------------------------------------------------------------------
//
//   RTL interface
//
// ----------------------------------------------------------------------------

uint32_t hero_dev_read32(const volatile uint32_t *base_addr, uint32_t off,
                         char off_type) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

void hero_dev_write32(volatile uint32_t *base_addr, uint32_t off, char off_type,
                      uint32_t value) {
    pr_warn("%s unimplemented\n", __func__);
}

int hero_dev_mbox_read(const HeroDev *dev, uint32_t *buffer, size_t n_words) {
    int ret, retry = 0;
    while (n_words--) {
        do {
            // If this region is cached, need a fence
#ifndef HOST_UNCACHED_IO
            // asm volatile ("fence");
#endif
            ret = rb_host_get(dev->mboxes.a2h_mbox, &buffer[n_words]);
            if (ret) {
                if (++retry == 100)
                    pr_warn("high retry on mbox read()\n");
                // For now avoid sleep that creates context switch
                for(int i = 0; i < HOST_MBOX_CYCLES; i++) {
                    asm volatile ("nop");
                }
            }
        } while (ret);
    }

    return 0;
}

int hero_dev_mbox_write(HeroDev *dev, uint32_t word) {
    int ret, retry = 0;
    do {
        // If this region is cached, need a fence
#ifndef HOST_UNCACHED_IO
            // asm volatile ("fence");
#endif
        ret = rb_host_put(dev->mboxes.h2a_mbox, &word);
        if (ret) {
            if (++retry == 100)
                pr_warn("high retry on mbox write()\n");
            // For now avoid sleep that creates context switch
            for(int i = 0; i < HOST_MBOX_CYCLES; i++) {
                asm volatile ("nop");
            }
        }
    } while (ret);
    
    return ret;
}

int hero_dev_reserve_v_addr(HeroDev *dev) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_free_v_addr(const HeroDev *dev) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

void hero_dev_print_v_addr(HeroDev *dev) {
    pr_warn("%s unimplemented\n", __func__);
}

__attribute__((weak)) int hero_dev_mmap(HeroDev *dev) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_alloc_mboxes(HeroDev *dev) {
    pr_trace("%s default\n", __func__);

    // Alloc ringbuf structure
    dev->mboxes.h2a_mbox = hero_dev_l2_malloc(dev, sizeof(struct ring_buf), &dev->mboxes.h2a_mbox_mem.p_addr);
    // Alloc data array for ringbuf
    dev->mboxes.h2a_mbox->data_v = hero_dev_l2_malloc(dev, sizeof(uint32_t)*16, &dev->mboxes.h2a_mbox->data_p);

    // Same for the accel2host mailbox
    dev->mboxes.a2h_mbox = hero_dev_l2_malloc(dev, sizeof(struct ring_buf), &dev->mboxes.a2h_mbox_mem.p_addr);
    dev->mboxes.a2h_mbox->data_v = hero_dev_l2_malloc(dev, sizeof(uint32_t)*16, &dev->mboxes.a2h_mbox->data_p);

    // Same for the rb mailbox
    dev->mboxes.rb_mbox = hero_dev_l2_malloc(dev, sizeof(struct ring_buf), &dev->mboxes.rb_mbox_mem.p_addr);
    dev->mboxes.rb_mbox->data_v = hero_dev_l2_malloc(dev, sizeof(uint32_t)*16, &dev->mboxes.rb_mbox->data_p);

    if(dev->mboxes.a2h_mbox < 0 || dev->mboxes.h2a_mbox < 0 || dev->mboxes.rb_mbox < 0) {
        return -1;
    }

    rb_init(dev->mboxes.a2h_mbox, 16, sizeof(uint32_t));
    rb_init(dev->mboxes.h2a_mbox, 16, sizeof(uint32_t));
    rb_init(dev->mboxes.rb_mbox, 16, sizeof(uint32_t));

    return 0;
}

int hero_dev_free_mboxes(HeroDev *dev) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_munmap(HeroDev *dev) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_clking_set_freq(HeroDev *dev, unsigned des_freq_mhz) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_clking_measure_freq(HeroDev *dev) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

__attribute__((weak)) int hero_dev_init(HeroDev *dev) {

    pr_warn("%s unimplemented\n", __func__);
}

__attribute__((weak)) void hero_dev_reset(HeroDev *dev, unsigned full) {
    pr_warn("%s unimplemented\n", __func__);
}

int hero_dev_get_nb_pe(HeroDev *dev) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_boot(HeroDev *dev, const TaskDesc *task) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_load_bin(HeroDev *dev, const char *name) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_load_bin_from_mem(HeroDev *dev, void *ptr, unsigned size) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

__attribute__((weak)) void hero_dev_exe_start(HeroDev *dev) {
    pr_warn("%s unimplemented\n", __func__);
    while(1) {}
}

void hero_dev_exe_stop(HeroDev *dev) {
    pr_warn("%s unimplemented\n", __func__);
}

int hero_dev_exe_wait(const HeroDev *dev, int timeout_s) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_rab_req(const HeroDev *dev, unsigned addr_start, unsigned size_b,
                     unsigned char prot, unsigned char port,
                     unsigned char date_exp, unsigned char date_cur,
                     unsigned char use_acp, unsigned char rab_lvl) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

void hero_dev_rab_free(const HeroDev *dev, unsigned char date_cur) {
    pr_warn("%s unimplemented\n", __func__);
}

int hero_dev_rab_req_striped(const HeroDev *dev, const TaskDesc *task,
                             ElemPassType **pass_type, int n_elements) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

void hero_dev_rab_free_striped(const HeroDev *dev) {
    pr_warn("%s unimplemented\n", __func__);
}

int hero_dev_rab_soc_mh_enable(const HeroDev *dev,
                               const unsigned static_2nd_lvl_slices) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_rab_soc_mh_disable(const HeroDev *dev) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_rab_mh_enable(const HeroDev *dev, unsigned char use_acp,
                           unsigned char rab_mh_lvl) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_rab_mh_disable(const HeroDev *dev) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_smmu_enable(const HeroDev *dev, const unsigned char flags) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_smmu_disable(const HeroDev *dev) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_rab_ax_log_read(const HeroDev *dev) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_profile_info(const HeroDev *dev, ProfileInfo *profile_info) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_out(HeroDev *dev, TaskDesc *task) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_in(HeroDev *dev, TaskDesc *task) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_get_pass_type(const TaskDesc *task,
                                   ElemPassType **pass_type) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_rab_setup(const HeroDev *dev, const TaskDesc *task,
                               ElemPassType **pass_type, int n_ref) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_rab_free(const HeroDev *dev, const TaskDesc *task,
                              const ElemPassType **pass_type, int n_ref) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_pass_desc(HeroDev *dev, const TaskDesc *task,
                               const ElemPassType **pass_type) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_get_desc(const HeroDev *dev, TaskDesc *task,
                              const ElemPassType **pass_type) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_l3_copy_raw_out(HeroDev *dev, TaskDesc *task,
                                     const ElemPassType **pass_type) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_offload_l3_copy_raw_in(HeroDev *dev, const TaskDesc *task,
                                    const ElemPassType **pass_type) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

uintptr_t hero_dev_l2_malloc(HeroDev *dev, unsigned size_b, uintptr_t *p_addr) {
    void *result = o1heapAllocate(l2_heap_manager, size_b);
    *p_addr = (void *) result - l2_heap_start_virt + l2_heap_start_phy;
    pr_trace("%s Allocated %u bytes at %lx\n", __func__, size_b, (void *) result - l2_heap_start_virt + l2_heap_start_phy);
    return result;
}

uintptr_t hero_dev_l3_malloc(HeroDev *dev, unsigned size_b, uintptr_t *p_addr) {
    void *result = o1heapAllocate(l3_heap_manager, size_b);
    *p_addr = (void *) result - l3_heap_start_virt + l3_heap_start_phy;
    pr_trace("%s Allocated %u bytes at %lx\n", __func__, size_b, (void *) result - l3_heap_start_virt + l3_heap_start_phy);
    return result;
}

void hero_dev_l3_free(HeroDev *dev, uintptr_t v_addr, uintptr_t p_addr) {
    //pr_warn("%s unimplemented\n", __func__);
    o1heapFree(l3_heap_manager, v_addr);
}

int hero_dev_dma_xfer(const HeroDev *dev, uintptr_t addr_l3,
                      uintptr_t addr_pulp, size_t size_b, int host_read) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_omp_offload_task(HeroDev *dev, TaskDesc *task) {
    pr_warn("%s unimplemented\n", __func__);
    return 0;
}

int hero_dev_l3_init(HeroDev *dev) {
    // Initialize L2 heap manager in the middle of the reserved memory range
    if (!l3_heap_manager) {
        if(!l3_heap_start_phy || !l3_heap_size) {
            pr_error("%s does not know where to put the heap manager\n", __func__);
            return -1;
        }
        pr_trace("Initializing o1heap at %p (%p) size %lx\n", (void *)(l3_heap_start_phy), (void *)(l3_heap_start_virt), l3_heap_size);
        l3_heap_manager = o1heapInit((void *)(l3_heap_start_virt), l3_heap_size, NULL, NULL);
        if (l3_heap_manager == NULL) {
            pr_error("Failed to initialize L3 heap manager.\n");
            return -ENOMEM;
        } else {
            pr_debug("Allocated L3 heap manager at %p.\n", l3_heap_manager);
        }
    } else {
        pr_warn("%s is already initialized\n", __func__);
    }
    return 0;
}

int hero_dev_l2_init(HeroDev *dev) {
    // Initialize L2 heap manager in the middle of the reserved memory range
    if (!l2_heap_manager) {
        if(!l2_heap_start_phy || !l2_heap_size) {
            pr_error("%s does not know where to put the heap manager\n", __func__);
            return -1;
        }
        pr_trace("Initializing o1heap at %p (%p) size %x\n", (void *) l2_heap_start_phy, (void *) l2_heap_start_virt, l2_heap_size);
        l2_heap_manager = o1heapInit((void *) l2_heap_start_virt, l2_heap_size, NULL, NULL);
        if (l2_heap_manager == NULL) {
            pr_error("Failed to initialize L2 heap manager.\n");
            return -ENOMEM;
        } else {
            pr_debug("Allocated L2 heap manager at %p.\n", l2_heap_manager);
        }
    } else {
        pr_warn("%s is already initialized\n", __func__);
    }
    return 0;
}

__attribute__((weak)) uintptr_t hero_host_l3_malloc(HeroDev *dev, unsigned size_b, uintptr_t *p_addr) {
    pr_warn("%s unimplemented\n", __func__);
    return NULL;
}
