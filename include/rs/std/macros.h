#pragma once

/**
 * Shared utility macros for rs_* projects.
 *
 * These macros are generic utilities that can be used by any project
 * in the rs ecosystem (rs_std, rs_app, etc.).
 */

// ============================================================================
// Platform detection
// ============================================================================

#if defined(_WIN32) || defined(_WIN64)
#define RS_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
#define RS_PLATFORM_MACOS 1
#elif defined(__linux__)
#define RS_PLATFORM_LINUX 1
#elif defined(__unix__)
#define RS_PLATFORM_UNIX 1
#endif

// ============================================================================
// C/C++ compatibility
// ============================================================================

/**
 * Mark functions for C linkage (useful for C++ compatibility).
 */
#ifdef __cplusplus
#define RS_EXTERN_C_BEGIN extern "C" {
#define RS_EXTERN_C_END }
#define RS_EXTERN_C extern "C"
#else
#define RS_EXTERN_C_BEGIN
#define RS_EXTERN_C_END
#define RS_EXTERN_C
#endif

// ============================================================================
// Compiler hints and attributes
// ============================================================================

/**
 * Inline hints (C99+)
 */
#if defined(__GNUC__) || defined(__clang__)
#define RS_INLINE static inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define RS_INLINE static __forceinline
#else
#define RS_INLINE static inline
#endif

/**
 * Compiler hints for likely/unlikely branches (optimization).
 */
#if defined(__GNUC__) || defined(__clang__)
#define RS_LIKELY(x) __builtin_expect(!!(x), 1)
#define RS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define RS_LIKELY(x) (x)
#define RS_UNLIKELY(x) (x)
#endif

/**
 * Deprecated warning.
 */
#if defined(__GNUC__) || defined(__clang__)
#define RS_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#define RS_DEPRECATED(msg) __declspec(deprecated(msg))
#else
#define RS_DEPRECATED(msg)
#endif

/**
 * No-return function attribute.
 */
#if defined(__GNUC__) || defined(__clang__)
#define RS_NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define RS_NORETURN __declspec(noreturn)
#else
#define RS_NORETURN
#endif

/**
 * Suppress unused variable/parameter warnings.
 * Usage: RS_UNUSED(my_variable);
 */
#define RS_UNUSED(x) ((void)(x))

/**
 * Thread-local storage (C11 standard).
 * Usage: RS_THREAD_LOCAL int my_var;
 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#define RS_THREAD_LOCAL _Thread_local
#elif defined(__GNUC__) || defined(__clang__)
#define RS_THREAD_LOCAL __thread
#elif defined(_MSC_VER)
#define RS_THREAD_LOCAL __declspec(thread)
#else
#warning "Thread-local storage not supported, some features may not be thread-safe"
#define RS_THREAD_LOCAL
#endif

// ============================================================================
// Array utilities
// ============================================================================

/**
 * Get the number of elements in a static array.
 * Only works with actual arrays, not pointers!
 *
 * Usage:
 *   int arr[] = {1, 2, 3};
 *   rs_size_t len = RS_ARRAYLEN(arr);  // 3
 */
#define RS_ARRAYLEN(arr) (sizeof(arr) / sizeof((arr)[0]))

// ============================================================================
// Development helpers
// ============================================================================

/**
 * Mark unimplemented code sections (emits compiler warning).
 * Usage: RS_TODO("description of what needs to be implemented");
 */
#if defined(__GNUC__) && __GNUC__ >= 5
// GCC 5+ supports warning attribute on statements
#define RS_TODO(msg)                                                                                                   \
    do {                                                                                                               \
        _Pragma("GCC warning \"TODO: " msg "\"")                                                                       \
    } while (0)
#elif defined(__clang__)
// Clang supports warning pragma
#define RS_TODO(msg) _Pragma("clang warning \"TODO: " msg "\"")
#elif defined(_MSC_VER)
// MSVC uses message pragma
#define RS_TODO(msg) __pragma(message("TODO: " msg))
#else
// Fallback: no-op
#define RS_TODO(msg) ((void)0)
#endif

/**
 * Mark unimplemented code that should never be executed in production.
 * Emits a compiler warning and triggers an assertion in debug builds.
 * Usage: RS_NOT_IMPLEMENTED("feature description");
 */
#ifdef NDEBUG
#define RS_NOT_IMPLEMENTED(msg) RS_TODO(msg)
#else
#include <assert.h>
#define RS_NOT_IMPLEMENTED(msg)                                                                                        \
    do {                                                                                                               \
        RS_TODO(msg);                                                                                                  \
        assert(0 && "Not implemented: " msg);                                                                          \
    } while (0)
#endif
