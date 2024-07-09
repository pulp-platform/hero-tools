////// HERO_1 includes /////
#ifdef __HERO_1
#include "omp.h"
#include "printf.h"
////// HOST includes /////
#else
#include <fcntl.h>
#include <libhero/hero_api.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#endif
///// ALL includes /////
#include <hero_64.h>
///// END includes /////

#define MAX_FILE_SIZE (0x1000 * 100)
#define PAGE_SIZE 0x1000
#define PAGE_SHIFT 12
#define ALIGN_DOWN(addr, align) ((addr) & ~((align)-1))

static inline void fence()
{
    asm volatile("fence" ::: "memory");
}

int main(int argc, char *argv[])
{

    uint32_t tmp_1;
    uint32_t tmp_2;
    int err;

    uint32_t A[256];

    for (int i = 0; i < rand() % 250000; i++)
        asm volatile("nop");

    // Benchmark omp init

    hero_add_timestamp("enter_omp_init", __func__, 1);

#pragma omp target device(1)
    {
        asm volatile("nop");
    }

    hero_add_timestamp("end", __func__, 1);

    // Benchmark IOMMU

    hero_add_timestamp("enter_omp_iommu", __func__, 1);

    int *C = 0;
    uintptr_t C_iova, C_pin_phys;
    void *C_mapped, *C_pin_virt;
    uint32_t C_lo, C_hi, C_phys_lo, C_phys_hi;
    volatile int a;
    uint32_t file_size;
    uint32_t t0, t1, t2, t3;

#ifndef __HERO_1

    int fd = open("README.md", O_RDWR | O_SYNC);
    if (fd < 0)
        return 0;
    FILE *fp = fdopen(fd, "rw");
    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    file_size = ALIGN_DOWN(file_size, PAGE_SIZE);

    C_mapped = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0x0);
    if (C_mapped == MAP_FAILED)
        return 0;

#if 0

    // IOMMU MAPPING
    fence();
    asm volatile("csrr %0, cycle" : "=r"(t0)::"memory");
    fence();
    for (int i = 0; i < file_size >> PAGE_SHIFT; i++)
        a = *(int *)(C_mapped + PAGE_SIZE * i);
    C_iova = hero_iommu_map_virt(NULL, file_size, C_mapped);

    fence();
    asm volatile("csrr %0, cycle" : "=r"(t1)::"memory");
    fence();

    printf("C_mapped = %llx, file_size = %x, time = %u cycles\n", C_mapped, file_size, t1-t0);

    C_lo = (uint32_t)C_iova;
    C_hi = (uint32_t)((uint64_t)C_iova >> 32);

#else

    // PHYSICAL MAPPING
    fence();
    asm volatile("csrr %0, cycle" : "=r"(t2)::"memory");
    fence();
    C_pin_virt = hero_host_l3_malloc(NULL, file_size, &C_pin_phys);

    if (C_pin_virt == NULL)
        return 0;

    memcpy(C_pin_virt, C_mapped, file_size);

    fence();
    asm volatile("csrr %0, cycle" : "=r"(t3)::"memory");
    fence();

    printf("C_pin_virt = %llx, C_pin_phys = %llx, time = %u cycles\n", C_pin_virt, C_pin_phys, t3 - t2);

    C_lo = (uint32_t)C_pin_phys;
    C_hi = (uint32_t)((uint64_t)C_pin_phys >> 32);

#endif

    printf("Device address : %llx\n", C_iova);

    if (!C_lo)
        return 0;

    asm volatile("fence");

#endif

#pragma omp target device(1) map(to : C_lo, C_hi, file_size)
    {
        uint32_t glob_lo    = (uint32_t)C_lo;
        uint32_t glob_hi    = (uint32_t)C_hi;
        uint32_t glob_size  = (uint32_t)file_size;
        __device void *glob = (__device void *)glob_lo;
#ifdef __HERO_1
        printf("%x %x %x\n", glob, glob_size);
        uint32_t t0, t1, t2, t3;
        if (glob_size > MAX_FILE_SIZE) {
            printf("File size error\n");
        } else {
            __device void *local = (__device void *)snrt_l1alloc(glob_size);
            fence();
            asm volatile("csrr %0, mcycle" : "=r"(t0)::"memory");
            fence();
            snrt_dma_start_1d_wideptr((void *)local, (const void *)glob, glob_size);
            snrt_dma_wait_all();
            fence();
            asm volatile("csrr %0, mcycle" : "=r"(t1)::"memory");
            fence();
            snrt_dma_start_1d_wideptr((void *)glob, (const void *)local, glob_size);
            snrt_dma_wait_all();
            fence();
            asm volatile("csrr %0, mcycle" : "=r"(t2)::"memory");
            fence();
            printf("%x - read %u bytes in %u cycles, write %u bytes in %u cycles \n", glob_size, t1 - t0, glob_size,
                   t2 - t1);
        }
#endif
    }

    hero_add_timestamp("end", __func__, 1);

#ifndef __HERO_1
    // Print all the recorded timestamps
    hero_print_timestamp();

    // Print the device cycles per region
    for (int i = 0; i < hero_num_device_cycles; i++)
        printf("%u - ", hero_device_cycles[i]);
    printf("\n");

    munmap(C_mapped, file_size);
    close(fd);
#endif

    return 0;
}
