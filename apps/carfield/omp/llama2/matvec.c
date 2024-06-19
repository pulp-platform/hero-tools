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

#define ALIGN_UP(addr, size) (((addr) + (size)-1) & ~((size)-1))

// 32-bit dot-product: a * b
float fdotp_vec_32b(const float *a, const float *b, unsigned int avl) {
  const unsigned int orig_avl = avl;
  unsigned int vl;

  float red;

  // Stripmine and accumulate a partial reduced vector
  do {
    // Set the vl
    asm volatile("vsetvli %0, %1, e32, m8, ta, ma" : "=r"(vl) : "r"(avl));

    // Load chunk a and b
    asm volatile("vle32.v v8,  (%0)" ::"r"(a));
    asm volatile("vle32.v v16, (%0)" ::"r"(b));

    // Multiply and accumulate
    if (avl == orig_avl) {
      asm volatile("vfmul.vv v24, v8, v16");
    } else {
      asm volatile("vfmacc.vv v24, v8, v16");
    }

    // Bump pointers
    a += vl;
    b += vl;
    avl -= vl;
  } while (avl > 0);

  // Reduce and return
  asm volatile ("vmv.v.i v0, 0");
  asm volatile("vfredusum.vs v0, v24, v0");
  asm volatile("vfmv.f.s %0, v0" : "=f"(red));
  return red;
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

// TCDM holds 32768 float-32 elements.
// We keep 32 KiB of TCDM for stacks
#define MAX_ELEM (128-32) * 1024 / (sizeof(DTYPE))
#define VERIFY 1
#define CORES 2

#ifdef __HERO_1
#include "snrt.h"
#include "team.h"
#include "printf.h"
#endif

int spatz_matmul(DTYPE *xout_, uint32_t xout_p_, DTYPE *x_, uint32_t x_p_, DTYPE *w_, uint32_t w_p_, int n_, int d_) {

    if (n_ * 9 > MAX_ELEM) {
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

        printf("%x - %x %x %x %x %x\n\r", (uint32_t)n, (uint32_t)d, (uint32_t)xout_p, (uint32_t)x_p, (uint32_t)w_p);

        printf("%x - %x\n\r", snrt_l1_get_next());

        uint32_t alloc_save = snrt_l1_get_next();

        DTYPE *x_l1[2];
        x_l1[0] = (DTYPE *) snrt_l1alloc(n * sizeof(DTYPE));
        DTYPE *w_row_l1[2];
        w_row_l1[0] = (DTYPE *) snrt_l1alloc(2 * n * sizeof(DTYPE));
        w_row_l1[1] = (DTYPE *) snrt_l1alloc(2 * n * sizeof(DTYPE));

        for(int i = 0; i < n; i++) {
            w_row_l1[1][i] = i;
            w_row_l1[0][i] = (float)(i%2);
        }

        snrt_dma_start_1d_wideptr((void*)x_l1[0], (const void*)x_p, n * sizeof(DTYPE));
        snrt_dma_start_1d_wideptr((void*)w_row_l1[0], (const void*)w_p , n * 2 * sizeof(DTYPE));

        int it = 0;
        uint32_t t0 = 0, tot_dma = 0, tot_dotp = 0;

        for (int I = 0; I < d; I += CORES) {
            int rows_left = d - I;
            t0 = read_csr(mcycle);
            snrt_dma_wait_all();
            tot_dma += read_csr(mcycle) - t0;
            if (rows_left > 2)
                snrt_dma_start_1d_wideptr(w_row_l1[(it+1)%2], w_p + n * sizeof(DTYPE) * (I+CORES), n * MIN(rows_left - CORES, CORES) * sizeof(DTYPE));

            t0 = read_csr(mcycle);
#pragma omp parallel for
            for (int i = 0; i < CORES; i++) {
                DTYPE val = 0.0f;
                if (I + i >= d)
                    goto end;
                val = fdotp_naive_32b(x_l1[0], &(w_row_l1[it%2][snrt_cluster_core_idx() * n]), n);
                ((DTYPE *)xout_p)[i + I] = val;
            end:;
            }
            tot_dotp += read_csr(mcycle) - t0;
            it++;
        }
        //printf("%i %i %i\n\r", tot_dma, tot_dotp);

        omp_exit:;
        //snrt_current_team()->allocator.l1.next = saved_alloc;
        snrt_l1_set_next(alloc_save);

#endif
    }
    hero_add_timestamp("enter_omp_end", __func__, 1);
    return 0;
}
