#include <inttypes.h>
#include <hero_64.h>

// Host matmul
void matmul(float* xout, float* x, float* w, int n, int d) {
    // W (d,n) @ x (n,) -> xout (d,)
    // by far the most amount of time is spent inside this little function
    int i;
    #pragma omp parallel for private(i)
    for (i = 0; i < d; i++) {
        float val = 0.0f;
        for (int j = 0; j < n; j++) {
            val += w[i * n + j] * x[j];
        }
        xout[i] = val;
    }
}

// This is required to init the hero library
void init_hero() {
    #pragma omp target device(1)
    {
    }
}

#define DTYPE float
#define MIN(A, B) (A < B ? A : B)

// TCDM holds 32768 float-32 elements.
// We keep 32 KiB of TCDM for stacks
#define MAX_ELEM (128-32) * 1024 / (sizeof(DTYPE))
#define VERIFY 1

#ifdef __HERO_1


#include "alloc_decls.h"
#include "cls_decls.h"

extern __thread cls_t* _cls_ptr;

#define ALIGN_UP(addr, size) (((addr) + (size)-1) & ~((size)-1))

void *hero_l1_alloc(size_t size) {
    __device snrt_allocator_t *alloc = (__device snrt_allocator_t *)&(_cls_ptr->l1_allocator);
    size = ALIGN_UP(size, 8);
    __device void *ret = (__device void *)alloc->next;
    alloc->next += size;
    return ret;
}

uint32_t update_alloc(uint32_t new) {
    __device snrt_allocator_t *alloc = (__device snrt_allocator_t *)&(_cls_ptr->l1_allocator);
    if (new)
        alloc->next = new;
    return alloc->next;
}

// 32-bit dot-product: a * b
float fdotp_simd32b(const float *a, const float *b, float *c, unsigned int K) {

    const register float zero = 0.0;

    // Don't accumulate in first iteration
    asm volatile("vfcpka.s.s ft3, %[zero], %[zero]\n"
                 // Don't accumulate in first iteration
                 "fld ft1, 0(%[a]) \n"
                 "fld ft2, 0(%[b]) \n"
                 "add %[a], %[a], 8 \n"
                 "add %[b], %[b], 8 \n"
                 "vfmul.s ft4, ft1, ft2 \n"
                 // loop over the MACs
                 "li     t0, 2 \n"
                 "3: \n"
                 "fld ft1, 0(%[a]) \n"
                 "fld ft2, 0(%[b]) \n"
                 "vfmac.s ft4, ft1, ft2 \n"
                 "add %[a], %[a], 8 \n"
                 "add %[b], %[b], 8 \n"
                 "addi  t0, t0, 2 \n"
                 "blt   t0, %[K], 3b \n"
                 // Sum reduce vector
                 "vfsum.s ft3, ft4 \n"
                 // Store results
                 "fsw ft3, 0(%[c]) \n"
                 : [a] "+r"(a), [b] "+r"(b)
                 : [c] "r"(c), [K] "r"(K), [zero] "f"(zero)
                 : "ft0", "ft1", "ft2", "ft3", "ft4", "t0");
}

// 32-bit dot-product: a * b
float fdotp_naive_32b(const float *a, const float *b, unsigned int K) {
    float value = 0;
    for(int i = 0; i < K; i++) {
        value += a[i] * b[i];
    }
    return value;
}

#endif

void matmul_snitch(DTYPE *xout_, uint32_t xout_p_, DTYPE *x_, uint32_t x_p_, DTYPE *w_,
            uint32_t w_p_, int n_, int d_) {
    
    // Check the 8*2 matrix rows + 1 vector can fit in TCDM
    // (due to the current tiling implementation) 
    if (n_ * 17 > MAX_ELEM) {
        printf("Error : Size too large\n\r");
        return;
    }

    // Benchmark total omp region
    char toprint[128];
    snprintf(toprint, 128, "enter_omp_matvec-%u", n_);
    hero_add_timestamp(toprint, __func__, 0);

#pragma omp target device(1) map(to : n_, d_, xout_p_, x_p_, w_p_)
    {
        volatile uint32_t n = n_;
        volatile uint32_t d = d_;
        volatile uint32_t xout_p = xout_p_;
        volatile uint32_t x_p = x_p_;
        volatile uint32_t w_p = w_p_;

#ifdef __HERO_1

        // Make sure inputs 
        if (!n || !d || !xout_p || !x_p || !w_p) {
            goto omp_exit;
        }

        // Save heap pointer to restore it later (hack)
        __device snrt_allocator_t *alloc = (__device snrt_allocator_t *)&(_cls_ptr->l1_allocator);
        uint32_t reset_allocator = update_alloc(0);

        // Debug print
        // snrt_printf("%x %x %x %x %x (alloc:%x)\n\r", (uint32_t)n, (uint32_t)d, (uint32_t)xout_p, (uint32_t)x_p, (uint32_t)w_p, (uint32_t) reset_allocator);

        // Alloc two inputs buffers for double buffering
        DTYPE *x_l1[2];
        x_l1[0] = (DTYPE *)hero_l1_alloc(n * sizeof(DTYPE));
        DTYPE *w_row_l1[2];
        w_row_l1[0] = (DTYPE *)hero_l1_alloc(8 * n * sizeof(DTYPE));
        w_row_l1[1] = (DTYPE *)hero_l1_alloc(8 * n * sizeof(DTYPE));

        // Start copying the first tile of inputs
        dm_memcpy_async((void *)x_l1[0], (const void *)x_p, n * sizeof(DTYPE));
        dm_memcpy_async((void *)w_row_l1[0], (const void *)w_p,
                        n * 8 * sizeof(DTYPE));

        int it = 0;

        // For each tile (8 rows)
        for (int I = 0; I < d; I += 8, it++) {
            int rows_left = d - I;

            // Wait for inputs
            dm_wait();

            // Prepare next tile's inputs
            if (rows_left > 8)
                dm_memcpy_async(w_row_l1[(it + 1) % 2],
                                w_p + n * sizeof(DTYPE) * (I + 8),
                                n * MIN(rows_left - 8, 8) * sizeof(DTYPE));
            // Parallelize over 8 cores
#pragma omp parallel for
            for (int i = 0; i < 8; i++) {
                DTYPE val = 0.0f;
                // Make sure there are enough rows for everyone
                if (I + i >= d)
                    goto end;
                // Dop-product
                fdotp_simd32b(x_l1[0], w_row_l1[it % 2]+i*n,
                              &((DTYPE *)xout_p)[i + I], n);
            end:;
            }
        }
        // Hack to free the heap
        reset_allocator = update_alloc(reset_allocator);

#endif

    omp_exit:;
    }
    // Benchmark total omp region
    hero_add_timestamp("enter_omp_end", __func__, 1);
}
