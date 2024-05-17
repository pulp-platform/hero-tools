/* Inference for Llama-2 Transformer model in pure C */

////// HERO_1 includes /////
#ifdef __HERO_1
#include "encoding.h"
#include "inttypes.h"
#include "matvec_dev.h"
extern volatile uint32_t dma_wait_cycles;
////// HOST includes /////
#else
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <libhero/hero_api.h>
#include <omp.h>
#define snrt_printf printf

static inline void fence() { asm volatile("fence" ::: "memory"); }

#endif
///// ALL includes /////
#include <hero_64.h>
///// END includes /////

#define DTYPE float

#define MIN(A, B) (A < B ? A : B)

// TCDM holds 32768 float-32 elements.
// We keep 32 KiB of TCDM for stacks
#define MAX_ELEM (128-32) * 1024 / (sizeof(DTYPE))
#define CORES 8
#define VERIFY 1

int matmul(DTYPE *xout_, uint32_t xout_p_, DTYPE *x_, uint32_t x_p_, DTYPE *w_, uint32_t w_p_, int n_, int d_) {

    if (n_ * 17 > MAX_ELEM) {
        printf("Error : Size too large\n\r");
        return -1;
    }

    char toprint[128];
    snprintf(toprint, 128, "enter_omp_matvec-%u", n_);
    hero_add_timestamp(toprint,__func__,0);

#pragma omp target device(1) map(to : n_, d_, xout_p_, x_p_, w_p_)
    {
        volatile uint32_t n = n_;
        volatile uint32_t d = d_;
        volatile uint32_t xout_p = xout_p_;
        volatile uint32_t x_p = x_p_;
        volatile uint32_t w_p = w_p_;

#ifdef __HERO_1

        if (!n || !d || !xout_p || !x_p || !w_p) {
            goto omp_exit;
        }

        //snrt_printf(" %x %x %x %x %x\n\r", (uint32_t)n, (uint32_t)d, (uint32_t)xout_p, (uint32_t)x_p, (uint32_t)w_p);
        //printf("%x - %x %x %x %x\n\r", snrt_hartid(), snrt_cluster_core_idx(),snrt_is_dm_core(), snrt_is_compute_core());

        DTYPE *x_l1[2];
        x_l1[0] = (DTYPE *) snrt_l1alloc(n * sizeof(DTYPE));
        DTYPE *w_row_l1[2];
        w_row_l1[0] = (DTYPE *) snrt_l1alloc(8 * n * sizeof(DTYPE));
        w_row_l1[1] = (DTYPE *) snrt_l1alloc(8 * n * sizeof(DTYPE));

        for(int i = 0; i < n; i++) {
            w_row_l1[1][i] = i;
            w_row_l1[0][i] = (float)(i%2);
        }

        dm_memcpy_async((void*)x_l1[0], (const void*)x_p, n * sizeof(DTYPE));
        dm_memcpy_async((void*)w_row_l1[0], (const void*)w_p , n * 8 * sizeof(DTYPE));

        int it = 0;
        uint32_t t0 = 0, tot_dma = 0;

        for (int I = 0; I < d; I += 8) {
            int rows_left = d - I;
            t0 = read_csr(mcycle);
            dm_wait();
            tot_dma += read_csr(mcycle) - t0;
            if (rows_left > 8)
                dm_memcpy_async(w_row_l1[(it+1)%2], w_p + n * sizeof(DTYPE) * (I+8), n * MIN(rows_left - 8, 8) * sizeof(DTYPE));

#pragma omp parallel for
            for (int i = 0; i < 8; i++) {
                DTYPE val = 0.0f;
                if (I + i >= d)
                    goto end;

                //for (int j = 0; j < n; j++) {
                //    val += x_l1[0][j] * w_row_l1[it%2][j + snrt_cluster_core_idx() * n];
                //}
                //((DTYPE *)xout_p)[i + I] = val;
                fdotp_simd32b(x_l1[0], &(w_row_l1[it%2][snrt_cluster_core_idx() * n]), &(((DTYPE *)xout_p)[i + I]), n);
            end:;
            }
            it++;
        }

        //snrt_printf("dma -> %u (now:%u)\n\r", tot_dma, read_csr(mcycle));

#endif

    omp_exit:;
    }
    hero_add_timestamp("enter_omp_end", __func__, 1);
    return 0;
}

int matmul_large(DTYPE *xout_, uint32_t xout_p_, DTYPE *x_, uint32_t x_p_, DTYPE *w_, uint32_t w_p_, int n_, int d_) {

    char toprint[128];
    snprintf(toprint, 128, "enter_omp_matvec-%u", n_);
    hero_add_timestamp(toprint,__func__,0);

#pragma omp target device(1) map(to : n_, d_, xout_p_, x_p_, w_p_)
    {
        volatile uint32_t n = n_;
        volatile uint32_t d = d_;
        volatile uint32_t xout_p = xout_p_;
        volatile uint32_t x_p = x_p_;
        volatile uint32_t w_p = w_p_;

        uint32_t tiling = 1;
        uint32_t smol_n;
        while ( (n_/tiling) * (2*CORES + 2) > MAX_ELEM )
            tiling = tiling << 1;

        smol_n = n_/tiling;

#ifdef __HERO_1

        if (!n || !d || !xout_p || !x_p || !w_p || !tiling) {
            goto omp_exit;
        }

        // printf("%x - %x %x %x %x %x\n\r", (uint32_t)n, (uint32_t)d, (uint32_t)xout_p, (uint32_t)x_p, (uint32_t)w_p);

        DTYPE *x_l1[2];
        x_l1[0] = (DTYPE *) snrt_l1alloc(smol_n * sizeof(DTYPE));
        x_l1[1] = (DTYPE *) snrt_l1alloc(smol_n * sizeof(DTYPE));
        DTYPE *w_row_l1[2];
        w_row_l1[0] = (DTYPE *) snrt_l1alloc(CORES * smol_n * sizeof(DTYPE));
        w_row_l1[1] = (DTYPE *) snrt_l1alloc(CORES * smol_n * sizeof(DTYPE));
        DTYPE *res;
        res = (__device DTYPE *) snrt_l1alloc(CORES * sizeof(DTYPE));

        int it = 0;
        uint32_t t0 = 0, tot_dma = 0, tot_dotp = 0;

        dm_memcpy2d_async(w_row_l1[0], w_p , smol_n*sizeof(DTYPE), smol_n*sizeof(DTYPE), n*sizeof(DTYPE), CORES, 0);
        dm_memcpy_async(x_l1[0],     x_p , smol_n*sizeof(DTYPE));

        //printf("%i (%i %i) %i %i/%i (%i %i)\n\r", d, n, tiling, CORES, CORES/tiling, d, smol_n);

        for (int I = 0; I < d; I += CORES) {
            int rows_left = d - I;

            for (int J = 0; J < n; J += smol_n) {
                //printf("%i (%i->%i) (%i->%i)\n\r", I, I+CORES, J, J+smol_n);      
                //t0 = read_csr(mcycle);
                //dma_wait_cycles += read_csr(mcycle) - t0;

                t0 = read_csr(mcycle);
                snrt_dma_wait_all();
                tot_dma += read_csr(mcycle) - t0;

                //for(int i = 0; i < CORES * smol_n; i++) {
                //    if(i % smol_n == 0)
                //        printf("\n\r");
                //    printf("(%i) %f ", w_row_l1[(it)%2][i]);
                //}
                //printf("\n\r");
                //for(int i = 0; i < smol_n; i++) {
                //    printf("(%i) %f ", x_l1[(it)%2][i]);
                //}
                //printf("\n\r");

                if(J+smol_n >= n) {
                    dm_memcpy2d_async(w_row_l1[(it+1)%2], w_p + (n * I + n * CORES)*sizeof(DTYPE), smol_n*sizeof(DTYPE), smol_n*sizeof(DTYPE), n*sizeof(DTYPE), CORES);
                    dm_memcpy_async(x_l1[(it+1)%2], x_p, smol_n * sizeof(DTYPE));
                } else {
                    dm_memcpy2d_async(w_row_l1[(it+1)%2], w_p + (n * I + (J+smol_n))*sizeof(DTYPE), smol_n*sizeof(DTYPE), smol_n*sizeof(DTYPE), n*sizeof(DTYPE), CORES);
                    dm_memcpy_async(x_l1[(it+1)%2],     x_p + (J+smol_n)*sizeof(DTYPE)      , smol_n * sizeof(DTYPE));
                }

                DTYPE* x_l1_it = (__device DTYPE*)x_l1[it%2];
                DTYPE* w_row_l1_it = (__device DTYPE*)w_row_l1[it%2]; 
                t0 = read_csr(mcycle);
                #pragma omp parallel for
                for (int i = 0; i < CORES; i++) {
                    float acc;
                    if (I + i >= d)
                        goto end;
                    fdotp_simd32b(x_l1[0], &(w_row_l1[it%2][snrt_cluster_core_idx() * n]), &acc, n);
                    if(J)
                        res[i] += acc;
                    else
                        res[i] = acc;
                end:;
                }
                tot_dotp += read_csr(mcycle) - t0;
                //printf("(%i) %f %f\n\r\n\r", res[0], res[1]);
                it++;
            }
            dm_memcpy_async(xout_p + I * sizeof(DTYPE), res, CORES * sizeof(DTYPE));

        }
        //snrt_printf("dma -> %u (now:%u)\n\r", ttot, read_csr(mcycle));

#endif

    omp_exit:;
    }
    hero_add_timestamp("enter_omp_end", __func__, 1);
    return 0;
}

int main(int argc, char *argv[]) {

    // Physical addresses
    uintptr_t C_phys, D_phys, E_phys;
    // Virtual addresses
    DTYPE *C = NULL, *D = NULL, *E;
    // Verification matrices
    DTYPE *C_test = NULL, *D_test = NULL, *E_test;
    int ret;

    int height = 20;
    int width = height;

    if (argc > 1)
        height = strtol(argv[1], NULL, 10);
    if (argc > 2)
        width = strtol(argv[2], NULL, 10);

    // Init Hero OpenMP runtime
    hero_add_timestamp("enter_omp_init", __func__, 1);
#pragma omp target device(1)
    { asm volatile ("nop"); }

    // Device matrices
    C = hero_dev_l3_malloc(NULL, width * height * sizeof(DTYPE), &C_phys);
    D = hero_dev_l3_malloc(NULL, width * sizeof(DTYPE), &D_phys);
    E = hero_dev_l3_malloc(NULL, height * sizeof(DTYPE), &E_phys);
    // Verification matrices
    C_test = malloc(width * height * sizeof(DTYPE));
    D_test = malloc(width * sizeof(DTYPE));
    E_test = malloc(height * sizeof(DTYPE));


    // Prepare data
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            C[i * width + j] = (DTYPE)((i * width + j));
    for (int j = 0; j < width; j++)
        D[j] = (j == 0) ? (DTYPE)(1) : (DTYPE)(0);

    for (int i = 0; i < height * width; i++)
        C_test[i] = C[i];
    for (int i = 0; i < width; i++)
        D_test[i] = D[i];
    
    asm volatile ("fence");

    // Offload !
    ret = matmul(E, E_phys, D, D_phys, C, C_phys, width, height);
    if(ret)
        ret = matmul_large(E, E_phys, D, D_phys, C, C_phys, width, height);

#ifndef __HERO_1

    // Execution on host
    char toprint[128];
    snprintf(toprint, 128, "enter_omp_verify_matvec-%u", width);
    hero_add_timestamp(toprint,__func__,0);
    asm volatile ("fence");
    for (int i = 0; i < height; i++) {
        DTYPE val = 0.0f;
        for (int j = 0; j < width; j++)
            val += C_test[i * width + j] * D_test[j];
        E_test[i] = val;
    }
    snprintf(toprint, 128, "enter_end", width);
    asm volatile ("fence");
    hero_add_timestamp(toprint,__func__,0);

    // Verify result
    for (int i = 0; i < height; i++) {
        if(E_test[i] != E[i])
            printf("nope %i\n\r", i);
    }


    // Print all the recorded timestamps
    hero_print_timestamp();

    // Print the device cycles per region
    for (int i = 0; i < hero_num_device_cycles; i++)
        printf("%u - ", hero_device_cycles[i]);
    printf("\n");
    for (int i = 0; i < hero_num_dma_cycles; i++)
        printf("%u - ", hero_dma_cycles[i]);
    printf("\n");

    hero_dev_l3_free(NULL, D, width  * sizeof(DTYPE));
    hero_dev_l3_free(NULL, C, width  * height * sizeof(DTYPE));
    hero_dev_l3_free(NULL, E, height * sizeof(DTYPE));

#endif

    return 0;
}
