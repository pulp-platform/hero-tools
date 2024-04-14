/* Inference for Llama-2 Transformer model in pure C */

////// HERO_1 includes /////
#ifdef __HERO_1

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

#include <libhero/herodev.h>
#include <omp.h>
#define snrt_printf printf

long time_in_ms();
static inline void fence() { asm volatile("fence" ::: "memory"); }

#endif
///// ALL includes /////
#include <hero_64.h>
///// END includes /////

#define DTYPE uint32_t

#define MIN(A, B) (A < B ? A : B)

// TCDM holds 32768 float-32 elements.
// We allow only using the quarter of the TCDM
#define MAX_ELEM 128 * 1024 / (4 * sizeof(DTYPE))
#define VERIFY 1

#ifndef __HERO_DEV

long time_in_ms() {
    // return time in milliseconds, for benchmarking the model speed
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

#endif

void matmul(DTYPE *xout_, uint32_t xout_p_, DTYPE *x_, uint32_t x_p_, DTYPE *w_,
            uint32_t w_p_, int n_, int d_) {

    hero_add_timestamp("region_omp_matvec", __func__, 1);

#pragma omp target device(1) map(to : n_, d_, xout_p_, x_p_, w_p_)
    {
        int n = n_;
        int d = d_;
        uint32_t xout_p = xout_p_;
        uint32_t x_p = x_p_;
        uint32_t w_p = w_p_;

        snrt_printf("W [%x] (%d x %d) @ X [%x] (%d) -> %x\n\r", w_p, d, n, x_p,
                    n, xout_p);
#ifdef __HERO_1

        snrt_alloc_init();

        DTYPE *x_l1 = snrt_l1alloc(n * sizeof(DTYPE));
        DTYPE *w_row_l1 = snrt_l1alloc(8 * n * sizeof(DTYPE));

        dm_memcpy_async(x_l1, x_p_, n * sizeof(DTYPE));

        for (int I = 0; I < d; I += 8) {
            int rows_left = d - I;

            dm_memcpy_async(w_row_l1, w_p_ + n * sizeof(DTYPE) * I,
                            n * MIN(rows_left, 8) * sizeof(DTYPE));
            dm_wait();

#pragma omp parallel for
            for (int i = 0; i < 8; i++) {
                DTYPE val = 0.0f;
                if (I + i >= d)
                    goto end;
                for (int j = 0; j < n; j++) {
                    val += x_l1[j] * w_row_l1[j + snrt_cluster_core_idx() * n];
                }
                ((DTYPE *)xout_p)[i + I] = val;
            end:;
            }
        }

#endif

    }

    hero_add_timestamp("region_verify_matvec", __func__, 1);

#ifdef VERIFY
    int i;
    for (i = 0; i < d_; i++) {
        DTYPE val = 0.0f;
        for (int j = 0; j < n_; j++) {
            val += w_[i * n_ + j] * x_[j];
        }
        // printf("%i : %f\n", i, val);
        if (xout_[i] != val)
            printf("Error : (%i) %x != %x\n", i, xout_[i], val);
    }
#endif

    hero_add_timestamp("region_end", __func__, 1);
}

int main(int argc, char *argv[]) {

    printf("cva6 main()\n");

    uint32_t tmp_1 = 5;
    uint32_t tmp_2 = 10;

    hero_add_timestamp("region_omp_init", __func__, 1);
    // Init Hero OpenMP runtime
#pragma omp target device(1) map(tofrom : tmp_1, tmp_2)
    { tmp_1 = tmp_2; }

    hero_add_timestamp("region_data_prep", __func__, 1);

    uintptr_t C_phys, D_phys, E_phys;
    DTYPE *C = NULL, *D = NULL, *E;

    int width = 244;
    int height = 200;

    if (argc > 1)
        height = strtol(argv[1], NULL, 10);

    C = hero_dev_l3_malloc(NULL, width * height * sizeof(DTYPE), &C_phys);
    D = hero_dev_l3_malloc(NULL, width * sizeof(DTYPE), &D_phys);
    E = hero_dev_l3_malloc(NULL, height * sizeof(DTYPE), &E_phys);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            C[i * width + j] = ((DTYPE)i + (DTYPE)j);
        }
    }

    for (int j = 0; j < width; j++) {
        D[j] = (DTYPE)j;
    }

    matmul(E, E_phys, D, D_phys, C, C_phys, width, height);

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
