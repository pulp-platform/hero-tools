// Copyright 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>

////// HERO_1 includes /////
#ifdef __HERO_1
////// HOST includes /////
#else
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <libhero/hero_api.h>

static inline void fence()
{
    asm volatile("fence" ::: "memory");
}

#endif
///// ALL includes /////
#include "hero_64.h"
#include "matvec.h"
///// END includes /////

void kernel_1()
{
#pragma omp target device(1)
    asm volatile("nop");
}

#define DTYPE float

#ifndef __HERO_DEV
int main(int argc, char *argv[])
{
    // Physical addresses
    uintptr_t C_phys, D_phys, E_phys;
    // Virtual addresses
    DTYPE *C = NULL, *D = NULL, *E;
    // Verification matrices
    DTYPE *C_test = NULL, *D_test = NULL, *E_test;
    // Return
    int ret;

    int height = 16;

    if (argc > 1)
        height = strtol(argv[1], NULL, 10);
    int width = height;
    if (argc > 2)
        width = strtol(argv[2], NULL, 10);

    // Init Hero OpenMP runtime
    kernel_1();

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

    asm volatile("fence");

    // Offload
    ret = matvec(E, E_phys, D, D_phys, C, C_phys, width, height);
    // if(ret)
    //     ret = matvec_large(E, E_phys, D, D_phys, C, C_phys, width, height);

    // Execution on host
    char toprint[128];
    snprintf(toprint, 128, "enter_omp_verify_matvec-%u", width);
    hero_add_timestamp(toprint, __func__, 0);
    asm volatile("fence");
    for (int i = 0; i < height; i++) {
        DTYPE val = 0.0f;
        for (int j = 0; j < width; j++)
            val += C_test[i * width + j] * D_test[j];
        E_test[i] = val;
    }
    snprintf(toprint, 128, "enter_end", width);
    asm volatile("fence");
    hero_add_timestamp(toprint, __func__, 0);

    // Verify result
    for (int i = 0; i < height; i++) {
        if (E_test[i] != E[i])
            printf("nope %i\n\r", i);
    }

    // Print all the recorded timestamps
    hero_print_timestamp();

    // Print the device cycles per region
    for (int i = 0; i < hero_num_device_cycles; i++)
        printf("%u - ", hero_device_cycles[i]);
    printf("\n");

    hero_dev_l3_free(NULL, D, D_phys);
    hero_dev_l3_free(NULL, C, C_phys);
    hero_dev_l3_free(NULL, E, E_phys);
    free(C_test);
    free(D_test);
    free(E_test);

    return 0;
}
#endif