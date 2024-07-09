// Copyright 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>

#pragma once

#define DTYPE float
#define MIN(A, B) (A < B ? A : B)

#define MAX_ELEM (128 - 32) * 1024 / (sizeof(DTYPE))

#ifdef __HERO_DEV
#include "kernels/fdotp.h"
#endif

#ifdef __HERO_OCCAMY
#include "encoding.h"
#include "inttypes.h"
#define CORES 8
#define hero_dma_1d_async dm_memcpy_async
#define hero_dma_wait_all dm_wait
#define fdotp_32b fdotp_simd_32b
#endif

#ifdef __HERO_SPATZ_CLUSTER
#include "kernels/fdotp.h"
#include "omp.h"
#include "printf.h"
#include "snrt.h"
#define CORES 2
#define hero_dma_1d_async snrt_dma_start_1d_wideptr
#define hero_dma_wait_all snrt_dma_wait_all
#define fdotp_32b fdotp_rvv_32b
#endif

int matvec(DTYPE *xout_, uint32_t xout_p_, DTYPE *x_, uint32_t x_p_, DTYPE *w_, uint32_t w_p_, int n_, int d_)
{

    if (n_ * 9 > MAX_ELEM) {
        printf("Error : Size too large\n\r");
        return -1;
    }

    char toprint[128];
    snprintf(toprint, 128, "enter_omp_matvec-%u", n_);
    hero_add_timestamp(toprint, __func__, 0);

#pragma omp target device(1) map(to : n_, d_, xout_p_, x_p_, w_p_)
    {
        volatile uint32_t n      = n_;
        volatile uint32_t d      = d_;
        volatile uint32_t xout_p = xout_p_;
        volatile uint32_t x_p    = x_p_;
        volatile uint32_t w_p    = w_p_;

#ifdef __HERO_DEV

        if (!n || !d || !xout_p || !x_p || !w_p) {
            goto omp_exit;
        }

        // printf("%x - %x %x %x %x %x\n\r", (uint32_t)n, (uint32_t)d, (uint32_t)xout_p, (uint32_t)x_p,
        // (uint32_t)w_p);

        DTYPE *x_l1[2];
        x_l1[0] = (DTYPE *)snrt_l1alloc(n * sizeof(DTYPE));
        DTYPE *w_row_l1[2];
        w_row_l1[0] = (DTYPE *)snrt_l1alloc(2 * n * sizeof(DTYPE));
        w_row_l1[1] = (DTYPE *)snrt_l1alloc(2 * n * sizeof(DTYPE));

        for (int i = 0; i < n; i++) {
            w_row_l1[1][i] = i;
            w_row_l1[0][i] = (float)(i % 2);
        }

        hero_dma_1d_async((void *)x_l1[0], (const void *)x_p, n * sizeof(DTYPE));
        hero_dma_1d_async((void *)w_row_l1[0], (const void *)w_p, n * 2 * sizeof(DTYPE));

        int it      = 0;
        uint32_t t0 = 0, tot_dma = 0, tot_dotp = 0;

        for (int I = 0; I < d; I += CORES) {
            int rows_left = d - I;
            t0            = read_csr(mcycle);
            hero_dma_wait_all();
            tot_dma += read_csr(mcycle) - t0;
            if (rows_left > 2)
                hero_dma_1d_async(w_row_l1[(it + 1) % 2], w_p + n * sizeof(DTYPE) * (I + CORES),
                                  n * MIN(rows_left - CORES, CORES) * sizeof(DTYPE));

            t0 = read_csr(mcycle);
#pragma omp parallel for
            for (int i = 0; i < CORES; i++) {
                DTYPE val = 0.0f;
                if (I + i >= d)
                    goto end;
                fdotp_32b(x_l1[0], &(w_row_l1[it % 2][snrt_cluster_core_idx() * n]), &((DTYPE *)xout_p)[i + I], n);

            end:;
            }
            tot_dotp += read_csr(mcycle) - t0;
            it++;
        }
        // printf("%i %i %i\n\r", tot_dma, tot_dotp);

#endif

    omp_exit:;
    }
    hero_add_timestamp("enter_omp_end", __func__, 1);
    return 0;
}

#if 0
int matvec_large(DTYPE *xout_, uint32_t xout_p_, DTYPE *x_, uint32_t x_p_, DTYPE *w_, uint32_t w_p_, int n_, int d_) {

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

#ifdef __HERO_DEV

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

        snrt_dma_start_2d_wideptr(w_row_l1[0], w_p , smol_n*sizeof(DTYPE), smol_n*sizeof(DTYPE), n*sizeof(DTYPE), CORES);
        hero_dma_1d_async(x_l1[0],     x_p , smol_n*sizeof(DTYPE));

        //printf("%i (%i %i) %i %i/%i (%i %i)\n\r", d, n, tiling, CORES, CORES/tiling, d, smol_n);

        for (int I = 0; I < d; I += CORES) {
            int rows_left = d - I;

            for (int J = 0; J < n; J += smol_n) {
                t0 = read_csr(mcycle);
                hero_dma_wait_all();
                tot_dma += read_csr(mcycle) - t0;

                if(J+smol_n >= n) {
                    snrt_dma_start_2d_wideptr(w_row_l1[(it+1)%2], w_p + (n * I + n * CORES)*sizeof(DTYPE), smol_n*sizeof(DTYPE), smol_n*sizeof(DTYPE), n*sizeof(DTYPE), CORES);
                    hero_dma_1d_async(x_l1[(it+1)%2], x_p, smol_n * sizeof(DTYPE));
                } else {
                    snrt_dma_start_2d_wideptr(w_row_l1[(it+1)%2], w_p + (n * I + (J+smol_n))*sizeof(DTYPE), smol_n*sizeof(DTYPE), smol_n*sizeof(DTYPE), n*sizeof(DTYPE), CORES);
                    hero_dma_1d_async(x_l1[(it+1)%2],     x_p + (J+smol_n)*sizeof(DTYPE)      , smol_n * sizeof(DTYPE));
                }

                DTYPE* x_l1_it = (__device DTYPE*)x_l1[it%2];
                DTYPE* w_row_l1_it = (__device DTYPE*)w_row_l1[it%2]; 
                t0 = read_csr(mcycle);
#pragma omp parallel for
                for (int i = 0; i < CORES; i++) {
                    if (I + i >= d)
                        goto end;
                    if(J)
                        res[i] += fdotp_v32b(&(x_l1_it[0]), &(w_row_l1_it[i * smol_n + J]), smol_n);
                    else
                        res[i] = fdotp_v32b(&(x_l1_it[0]), &(w_row_l1_it[i * smol_n + J]), smol_n);
                end:;
                }
                tot_dotp += read_csr(mcycle) - t0;
                it++;
            }
            hero_dma_1d_async(xout_p + I * sizeof(DTYPE), res, CORES * sizeof(DTYPE));

        }
#endif

    omp_exit:;
    }
    hero_add_timestamp("enter_omp_end", __func__, 1);
    return 0;
}
#endif
