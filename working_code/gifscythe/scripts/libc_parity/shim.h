/* libc_parity shim - force-included (-include) into every engine translation unit by
   libc_parity.py. Inert unless one of the two guards below applies. N-32 probe.

   __wasi__          wasi-libc has no popen/pclose/umask/mkstemp. gifsicle's xform.c reaches them
                     only for --transform-colormap (an external command); the probe never uses
                     that option, so link-time stubs that fail are enough.

   GS_STABLE_LIBC    pin the two libc behaviours that make gifsicle's OUTPUT libc-specific:
                     - qsort: the order it gives equal keys (the quantizer and the optimizer sort
                       colours with it) -> a stable merge sort, identical on every libc;
                     - random(): quantize.c seeds the dither's error pattern with RANDOM(), which
                       the config headers map to libc random() -> a fixed LCG.
                     With both pinned, glibc, musl and wasi-libc builds of the same sources agree
                     byte for byte (measured S34, 9/9 cases). */
#ifndef GS_LIBC_PARITY_SHIM_H
#define GS_LIBC_PARITY_SHIM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef __wasi__
static inline mode_t umask(mode_t m) { (void)m; return 0; }
static inline int mkstemp(char *t) { (void)t; return -1; }
static inline FILE *popen(const char *c, const char *m) { (void)c; (void)m; return (FILE *)0; }
static inline int pclose(FILE *f) { (void)f; return -1; }
#endif

#ifdef GS_STABLE_LIBC
static void gs_msort_(char *a, char *tmp, size_t n, size_t sz, int (*cmp)(const void *, const void *)) {
  if (n < 2) return;
  size_t h = n / 2, i = 0, j = h, k = 0;
  gs_msort_(a, tmp, h, sz, cmp);
  gs_msort_(a + h * sz, tmp, n - h, sz, cmp);
  while (i < h && j < n) {
    if (cmp(a + j * sz, a + i * sz) < 0) { memcpy(tmp + k * sz, a + j * sz, sz); j++; }
    else { memcpy(tmp + k * sz, a + i * sz, sz); i++; }
    k++;
  }
  while (i < h) { memcpy(tmp + k * sz, a + i * sz, sz); i++; k++; }
  while (j < n) { memcpy(tmp + k * sz, a + j * sz, sz); j++; k++; }
  memcpy(a, tmp, n * sz);
}
static void gs_qsort(void *base, size_t n, size_t sz, int (*cmp)(const void *, const void *)) {
  char *tmp;
  if (n < 2 || sz == 0) return;
  tmp = (char *)malloc(n * sz);
  if (!tmp) return;
  gs_msort_((char *)base, tmp, n, sz, cmp);
  free(tmp);
}
static long gs_random(void) {
  static unsigned long long s = 1ULL;
  s = s * 6364136223846793005ULL + 1442695040888963407ULL;
  return (long)((s >> 33) & 0x7fffffffULL);
}
#define qsort gs_qsort
#define random gs_random
#endif

#endif
