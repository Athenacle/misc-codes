
#ifndef CONFIG_H_
#define CONFIG_H_

/* #undef UNIX_HAVE_GETOPTLONG */

/* #undef UNIX_HAVE_PTHREAD_SPINLOCK */

#define UNIX_HAVE_PTHREAD_COND

#define HAVE_PTHREAD_H

#define UNIX_HAVE_PTHREAD_MUTEX

/* #undef UNIX_HAVE_DLOPEN */

#define HAVE_BZERO

#define HAVE_BUILTIN_EXPECT

#define UNIX

#define HAVE_CLOCK_REALTIME_COARSE

#define UNIX_HAVE_NANOSLEEP

#define ON_64BITS

#define UNIX_HAVE_GET_NPROCS

#define UNIX_HAVE_SETENV

#define UNIX_HAVE_MMAP

// clang-format off

#define PROJECT_GIT_SHA ""

#define PROJECT_NAME "nDownloader"
#define PROJECT_VERSION "0.0.0"
#define PROJECT_MAJOR_VERSION (0)
#define PROJECT_MINOR_VERSION (0)
#define PROJECT_PATCH_VERSION (0)
#define PROJECT_VERSION_HEX (unsigned)((PROJECT_MAJOR_VERSION << 24) | (PROJECT_MINOR_VERSION<< 8) | (PROJECT_PATCH_VERSION))
// clang-format on

#ifdef HAVE_BUILTIN_EXPECT
#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
#else
#define likely(x) ((x))
#define unlikely(x) ((x))
#endif

#if __cplusplus >= 200809L && __has_cpp_attribute(noreturn)
#define NORETURN [[noreturn]]
#else
#define NORETURN
#endif

#if __cplusplus >= 201603L && __has_cpp_attribute(maybe_unused)
#define MAYBE_UNUSED [[maybe_unused]]
#else
#define MAYBE_UNUSED
#endif

#if defined HAVE_ALLOCA && defined NDEBUG
#define Alloca(size) (alloca(size))
#define AllocaFree(ptr) ((void)(ptr))
#else
#define Alloca(size) (malloc(size))
#define AllocaFree(ptr) (free((void*)(ptr)))
#endif

#endif
