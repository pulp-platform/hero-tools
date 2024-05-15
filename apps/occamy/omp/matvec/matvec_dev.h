#include <inttypes.h>


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
