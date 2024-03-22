////// HERO_1 includes /////
#ifdef __HERO_1
#include "axpy_spatz.h"
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

#define DTYPE uint32_t

int main(int argc, char *argv[]) {

    //printf("cva6 main()\n");

    hero_add_timestamp("test_measure", __func__, 0);
    hero_add_timestamp("test_measure", __func__, 0);
    hero_add_timestamp("test_measure", __func__, 0);

    // Benchmark omp init

    hero_add_timestamp("enter_omp_init", __func__, 1);

#pragma omp target device(1)
    { asm volatile("nop"); }

    // Benchmark offloads

    for (int i = 0; i < 1; i++) {

        uintptr_t x_p, y_p;
        uint32_t M = 64;
        DTYPE *x = hero_dev_l2_malloc(NULL, M*sizeof(DTYPE), &x_p);
        DTYPE *y = hero_dev_l2_malloc(NULL, M*sizeof(DTYPE), &y_p);
        DTYPE a = 0;

        for(int i = 0; i < M; i++) {
            x[i] = (DTYPE) 1;
            y[i] = (DTYPE) 2;
        }

        // Benchmark offload with data copy to
        printf("x_p (%x) y_p (%x) \n", x_p, y_p);

        printf("y (%i) : \n", M);
        for(int i = 0; i < M; i++) {
            printf("%f - ", y[i]);
        }
        printf("\n");

        printf("x (%i) : \n", M);
        for(int i = 0; i < M; i++) {
            printf("%f - ", x[i]);
        }
        printf("\n");

        printf("\n ----- \n\n");

        hero_add_timestamp("enter_omp_map_to", __func__, 1);

#pragma omp target device(1) map(to: x_p, y_p, M);
        {
            DTYPE *_x = (__device DTYPE *) x_p;
            DTYPE *_y = (__device DTYPE *) y_p;
            uint32_t _M = M;
#ifdef __HERO_1
            DTYPE* x_loc = snrt_l1alloc(_M * sizeof(DTYPE));
            DTYPE* y_loc = snrt_l1alloc(_M * sizeof(DTYPE));
            snrt_putchar(126);snrt_putchar(10);snrt_putchar(13);
            snrt_putword(_x);
            snrt_putword(*_x);
            snrt_dma_start_1d(x_loc, _x, _M * sizeof(DTYPE));
            snrt_dma_start_1d(y_loc, _y, _M * sizeof(DTYPE));
            snrt_dma_wait_all();

            uint32_t* cluster_status = 0x51020050;
            *cluster_status = 0x1;

            uint32_t csr_status = 0x0;
            asm volatile("csrr %0, mstatus" : "=r"(csr_status));
            csr_status |= 0x00000600;
            asm volatile("csrw mstatus, %0" : : "r"(csr_status));

            snrt_putchar(126);snrt_putchar(10);snrt_putchar(13);
            snrt_putword(csr_status);

            unsigned int vl, avl=32;
            asm volatile("vsetvli %0, %1, e32, m8, ta, ma" : "=r"(vl) : "r"(avl));
            snrt_putchar(126);snrt_putchar(10);snrt_putchar(13);
            snrt_putword(vl);
            snrt_putword(avl);
            snrt_putchar(126);snrt_putchar(10);snrt_putchar(13);
            snrt_putword(x_loc);
            snrt_putword(*x_loc);
            snrt_putword(y_loc);
            snrt_putword(*y_loc);
            snrt_putchar(126);snrt_putchar(10);snrt_putchar(13);
            asm volatile("vle32.v v8,  (%0)" ::"r"(x_loc));
            asm volatile("vse32.v v8,  (%0)" ::"r"(y_loc));
            snrt_putword(x_loc);
            snrt_putword(*x_loc);
            snrt_putword(y_loc);
            snrt_putword(*y_loc);

            *cluster_status = 0x0;

            //faxpy_v32b(x_loc, y_loc, _M);
            snrt_dma_start_1d(_y, x_loc, _M * sizeof(DTYPE));
            snrt_dma_wait_all();
#endif
        }


        hero_add_timestamp("end_omp_map_to", __func__, 1);

        printf("\n ----- \n\n");

        // Benchmark offload with data copy to
        // printf("x_p (%x) y_p (%x) \n", x_p, y_p);

        printf("y (%i) : \n", M);
        for(int i = 0; i < M; i++) {
            printf("%f - ", y[i]);
        }
        printf("\n");

    }

#ifndef __HERO_1
    // Print all the recorded timestamps
    hero_print_timestamp();

    // Print the device cycles per region
    for (int i = 0; i < hero_num_device_cycles; i++)
        printf("%u - ", hero_device_cycles[i]);
    printf("\n");
#endif

    return 0;
}
