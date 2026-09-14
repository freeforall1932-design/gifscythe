/* config.wasm.h - product-owned Emscripten build configuration for gifsicle.
   Staged as config.h in a temporary build directory by web/wasm/build_wasm.sh;
   deliberately outside reference_code so the vendored upstream snapshot
   remains immutable (same pattern as build_support/gifsicle/config.native.h).

   Deltas vs the native config, all conservative:
   - No ENABLE_THREADS: single-threaded build (all pthread uses in the built
     sources are inside `#if ENABLE_THREADS`, and xform.c forces nthreads = 1
     on the `#else` branch, so the `-j` flag the shared command builder always
     emits is accepted and ignored).
   - No SIMD: scalar fallback in kcolor.h (correct everywhere; revise only
     with measured need).
   - wasm32 sizes: unsigned long 4, void* 4 (native LP64 uses 8).
   - No GETTIMEOFDAY_PROTO: only gifview.c reads it, and gifview is not built.
   Everything else matches native: Emscripten's libc provides all the headers,
   mkstemp, random(), pow/cbrtf. */

#ifndef GIFSICLE_CONFIG_H
#define GIFSICLE_CONFIG_H

#include <stddef.h>

/* Indicates config.h was included. */
#define HAVE_CONFIG_H 1

/* Headers available under Emscripten. */
#define HAVE_INTTYPES_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_SELECT_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_UNISTD_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STDINT_H 1
#define HAVE_TIME_H 1
#define HAVE_SYS_TIME_H 1

/* Core functions available under Emscripten. */
#define HAVE_STRTOUL 1
#define HAVE_MKSTEMP 1
#define HAVE_SNPRINTF 1
#define HAVE_STRERROR 1

/* Math library functions. */
#define HAVE_POW 1
#define HAVE_CBRTF 1

/* Random function. */
#define RANDOM random

/* Integer types. */
#define HAVE_UINT64_T 1
#define HAVE_INT64_T 1
#define HAVE_UINTPTR_T 1
#define HAVE_SIZEOF_FLOAT 1
#define HAVE_SIZEOF_UNSIGNED_INT 1
#define HAVE_SIZEOF_UNSIGNED_LONG 1

#ifdef HAVE_SIZEOF_FLOAT
#define SIZEOF_FLOAT 4
#endif
#ifdef HAVE_SIZEOF_UNSIGNED_INT
#define SIZEOF_UNSIGNED_INT 4
#endif
#ifdef HAVE_SIZEOF_UNSIGNED_LONG
#define SIZEOF_UNSIGNED_LONG 4
#endif
#define SIZEOF_VOID_P 4

/* Pathname separator (Emscripten virtual FS). */
#define PATHNAME_SEPARATOR '/'

/* We build gifsicle only (no X10 gifview), so define X_DISPLAY_MISSING. */
#define X_DISPLAY_MISSING 1

/* No ENABLE_THREADS (single-threaded; see above). No SIMD (scalar fallback). */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Use the clean-failing malloc library in fmalloc.c. */
#define GIF_ALLOCATOR_DEFINED   1
#define Gif_Free free

/* Prototype strerror if we don't have it. */
#ifndef HAVE_STRERROR
char *strerror(int errno);
#endif

#ifdef __cplusplus
}
/* Get rid of a possible inline macro under C++. */
# define inline inline
#endif

#if defined(_MSDOS) || defined(_WIN32) || defined(__EMX__) || defined(__DJGPP__)
# include <fcntl.h>
# include <io.h>
# define isatty _isatty
#endif

#ifndef HAVE_SNPRINTF
#define snprintf(s, n, ...) sprintf((s), __VA_ARGS__)
#endif

#endif /* GIFSICLE_CONFIG_H */
