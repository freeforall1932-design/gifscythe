/* config.native.h - product-owned native build configuration for gifsicle.
   This header is staged as config.h in a temporary build directory; it is
   deliberately outside reference_code so the vendored upstream snapshot
   remains immutable. */

#ifndef GIFSICLE_CONFIG_H
#define GIFSICLE_CONFIG_H

#include <stddef.h>

/* Indicates config.h was included. */
#define HAVE_CONFIG_H 1

/* Headers available on Linux/gcc. */
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

/* Core functions available on Linux/gcc. */
#define HAVE_STRTOUL 1
#define HAVE_MKSTEMP 1
#define HAVE_SNPRINTF 1
#define HAVE_STRERROR 1

/* Math library functions (need -lm). */
#define HAVE_POW 1
#define HAVE_CBRTF 1

/* Random function on glibc. */
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
#define SIZEOF_UNSIGNED_LONG 8
#endif
#define SIZEOF_VOID_P 8

/* Pathname separator (Unix). */
#define PATHNAME_SEPARATOR '/'

/* We build gifsicle only (no X10 gifview), so define X_DISPLAY_MISSING. */
#define X_DISPLAY_MISSING 1

/* SIMD support on gcc (vector_size / ext_vector_type work). */
#define HAVE_SIMD 1
#define HAVE_VECTOR_SIZE_VECTOR_TYPES 1
#define HAVE_EXT_VECTOR_TYPE_VECTOR_TYPES 1
#define HAVE___BUILTIN_SHUFFLEVECTOR 1
#define HAVE___SYNC_ADD_AND_FETCH 1

/* gettimeofday prototype: 2 args on modern glibc. */
#define GETTIMEOFDAY_PROTO 2

/* Threading: enable for gifsicle. */
#define ENABLE_THREADS 1

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
