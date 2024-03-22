////// HERO_1 includes /////
#ifdef __HERO_1

////// HOST includes /////
#else
#include <libhero/herodev.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#endif
///// ALL includes /////
#include <hero_64.h>
///// END includes /////

int main(int argc, char *argv[]) {

#pragma omp target device(1)
    { asm volatile("nop"); }

    // Benchmark memory

    for (int i = 0; i < 10; i++) {

        // Latency

#pragma omp target device(1)
        {

            uint32_t *pcie_mem = 0x180001000, *l3_mem = 0xc00000000,
                     *l2_mem = 0x78000000;
            uint32_t pcie_0, pcie_1, l3_0, l3_1, l2_0, l2_1;

#ifdef __HERO_1
            asm volatile("csrr %0, mcycle       \n\t"
                         "lw   t0, 0(%2)        \n\t"
                         "add  t0, t0    , zero \n\t"
                         "sw   t0, 0(%2)        \n\t"
                         "csrr %1, mcycle       \n\t"
                         : "=r"(l3_0), "=r"(l3_1)
                         : "r"(l3_mem));
            asm volatile("csrr %0, mcycle       \n\t"
                         "lw   t0, 0(%2)        \n\t"
                         "add  t0, t0    , zero \n\t"
                         "sw   t0, 0(%2)        \n\t"
                         "csrr %1, mcycle       \n\t"
                         : "=r"(l2_0), "=r"(l2_1)
                         : "r"(l2_mem));

            snrt_putchar(10);
            snrt_putchar(13);
            snrt_putchar(10);
            snrt_putchar(13);
            snrt_putword(l3_1 - l3_0);
            snrt_putword(l2_1 - l2_0);
            snrt_putchar(10);
            snrt_putchar(13);

            end:;
#endif
        }


// Bandwidth

#pragma omp target device(1)
        {

            uint32_t *pcie_mem = 0x180001000, *l3_mem = 0xc00000000,
                     *l2_mem = 0x78000000;
            uint32_t pcie_b_0, pcie_b_1, l3_b_0, l3_b_1, l2_b_0, l2_b_1;

#ifdef __HERO_1
            uint32_t M = 1024 / 4 * sizeof(uint32_t) * 50;

            snrt_alloc_init();
            uint32_t *tcdm_buf = snrt_l1alloc(M);

            if(!tcdm_buf) goto end;

            asm volatile("csrr %0, mcycle" : "=r"(l3_b_0));
            snrt_dma_start_1d(tcdm_buf, l3_mem, M);
            snrt_dma_wait_all();
            asm volatile("csrr %0, mcycle" : "=r"(l3_b_1));

            asm volatile("csrr %0, mcycle" : "=r"(l2_b_0));
            snrt_dma_start_1d(tcdm_buf, l2_mem, M);
            snrt_dma_wait_all();
            asm volatile("csrr %0, mcycle" : "=r"(l2_b_1));

            snrt_putchar(10);
            snrt_putchar(13);
            snrt_putchar(10);
            snrt_putchar(13);
            snrt_putword(l3_b_1 - l3_b_0);
            snrt_putword(l2_b_1 - l2_b_0);
            snrt_putchar(10);
            snrt_putchar(13);

            end:;
#endif
        }

    }

#ifndef __HERO_1
    // Print the device cycles per region
    for (int i = 0; i < hero_num_device_cycles; i++)
        printf("%u - ", hero_device_cycles[i]);
    printf("\n");
#endif

    return 0;
}
