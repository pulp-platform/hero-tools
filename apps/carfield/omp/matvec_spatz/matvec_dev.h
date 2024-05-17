#include <inttypes.h>

// 32-bit dot-product: a * b
float fdotp_v32b(const float *a, const float *b, unsigned int avl) {
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

float fdotp_32(const float *a, const float *b, unsigned int avl) {
  float val = 0.0;

  for(int i = 0; i < avl; i++) {
    val += a[i] * b[i];
  }

  return val;
}