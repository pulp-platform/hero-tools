// Remove host functions
#define hero_add_timestamp(A, B, C) ;


// 16-bit dot-product: a * b
_Float16 fdotp_v16b(const _Float16 *a, const _Float16 *b, unsigned int avl) {
  const unsigned int orig_avl = avl;
  unsigned int vl;

  _Float16 red;

  // Stripmine and accumulate a partial reduced vector
  do {
    // Set the vl
    asm volatile("vsetvli %0, %1, e16, m8, ta, ma" : "=r"(vl) : "r"(avl));

    // Load chunk a and b
    asm volatile("vle16.v v8,  (%0)" ::"r"(a));
    asm volatile("vle16.v v16, (%0)" ::"r"(b));

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
  asm volatile("vfredusum.vs v0, v24, v0");
  asm volatile("vfmv.f.s %0, v0" : "=f"(red));
  return red;
}
