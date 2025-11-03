#pragma once

#include <rs/std/internal/api.h>
#include <rs/std/types.h>
#include <stdlib.h>

RS_EXTERN_C_BEGIN

// ============================================================================
// Defer - Automatic Cleanup on Scope Exit
// ============================================================================

/**
 * Defer actions allow you to schedule cleanup code that runs automatically
 * when leaving the current scope, regardless of how you exit (return, break,
 * goto, or normal flow).
 *
 * This provides RAII-like functionality in C and helps ensure resources are
 * properly cleaned up without manual tracking.
 *
 * Example:
 *   void example(void) {
 *       RS_SCOPED rs_defer_scope_t defer = RS_DEFER_SCOPE_INIT();
 *
 *       FILE *f = fopen("test.txt", "r");
 *       rs_defer_scope_add(&defer, (rs_defer_fn)fclose, f);
 *
 *       char *buf = malloc(1024);
 *       rs_defer_scope_add(&defer, free, buf);
 *
 *       // ... use resources ...
 *
 *       // Cleanup happens automatically when defer goes out of scope
 *   }
 */

// Maximum number of deferred actions per scope
#ifndef RS_DEFER_MAX_ACTIONS
#define RS_DEFER_MAX_ACTIONS 32
#endif

/**
 * Defer function signature.
 * Called with the data pointer when scope exits.
 */
typedef void (*rs_defer_fn)(void *data);

/**
 * Single defer action.
 */
typedef struct {
    rs_defer_fn fn; // Cleanup function
    void *data;     // Data to pass to cleanup function
} rs_defer_action_t;

/**
 * Defer scope - manages a stack of cleanup actions.
 * Allocated on the stack, no dynamic allocation required.
 */
typedef struct {
    rs_defer_action_t actions[RS_DEFER_MAX_ACTIONS];
    rs_size_t count;
} rs_defer_scope_t;

// ============================================================================
// Core API (Portable - Works Everywhere)
// ============================================================================

/**
 * Initialize a defer scope.
 * Must be called before using rs_defer_scope_add.
 *
 * @param scope Defer scope to initialize
 */
RS_STD_API void rs_defer_scope_init(rs_defer_scope_t *scope);

/**
 * Add a deferred action to the scope.
 * Actions are executed in LIFO order (last added, first executed).
 *
 * @param scope Defer scope
 * @param fn Cleanup function to call
 * @param data Data to pass to cleanup function
 * @return RS_OK on success, RS_ERR_OVERFLOW if scope is full
 */
RS_STD_API rs_result_t rs_defer_scope_add(rs_defer_scope_t *scope, rs_defer_fn fn, void *data);

/**
 * Execute all deferred actions in reverse order.
 * This is called automatically when using RS_SCOPED, but can be
 * called manually if needed.
 *
 * @param scope Defer scope
 */
RS_STD_API void rs_defer_scope_execute(rs_defer_scope_t *scope);

/**
 * Clear all deferred actions without executing them.
 * Useful if you've transferred ownership of resources elsewhere.
 *
 * @param scope Defer scope
 */
RS_STD_API void rs_defer_scope_clear(rs_defer_scope_t *scope);

// ============================================================================
// Platform-Specific Automatic Cleanup
// ============================================================================

// Cleanup function for automatic scope exit
RS_STD_API void rs_defer_scope_cleanup(rs_defer_scope_t *scope);

// Detect cleanup attribute support
#if defined(__GNUC__) || defined(__clang__)
// GCC and Clang support cleanup attribute
#define RS_HAS_CLEANUP_ATTRIBUTE 1
#define RS_SCOPED __attribute__((cleanup(rs_defer_scope_cleanup)))
#elif defined(_MSC_VER) && _MSC_VER >= 1400
// MSVC 2005+ has limited support via __try/__finally
// But it's not suitable for general use, so we don't use it
#define RS_HAS_CLEANUP_ATTRIBUTE 0
#define RS_SCOPED
#else
// No automatic cleanup support
#define RS_HAS_CLEANUP_ATTRIBUTE 0
#define RS_SCOPED
#endif

// ============================================================================
// Convenience Macros
// ============================================================================

/**
 * Initialize a defer scope inline.
 * Use with RS_SCOPED for automatic cleanup.
 *
 * Example:
 *   RS_SCOPED rs_defer_scope_t defer = RS_DEFER_SCOPE_INIT();
 */
#define RS_DEFER_SCOPE_INIT() ((rs_defer_scope_t){.count = 0})

#if RS_HAS_CLEANUP_ATTRIBUTE

/**
 * Begin a defer scope block (automatic cleanup on GCC/Clang).
 * Actions are executed automatically when leaving the block.
 *
 * Example:
 *   RS_DEFER_SCOPE_BEGIN() {
 *       FILE *f = fopen("test.txt", "r");
 *       RS_DEFER_ADD(fclose, f);
 *       // ... use file ...
 *   } RS_DEFER_SCOPE_END();
 */
#define RS_DEFER_SCOPE_BEGIN()                                                                                         \
    do {                                                                                                               \
    RS_SCOPED rs_defer_scope_t _rs_defer_scope = RS_DEFER_SCOPE_INIT()

#define RS_DEFER_SCOPE_END()                                                                                           \
    }                                                                                                                  \
    while (0)

/**
 * Add a defer action in the current scope block.
 * Must be used within RS_DEFER_SCOPE_BEGIN/END.
 */
#define RS_DEFER_ADD(fn, data) rs_defer_scope_add(&_rs_defer_scope, (rs_defer_fn)(fn), (void *)(data))

#else // !RS_HAS_CLEANUP_ATTRIBUTE

/**
 * Begin a defer scope block (manual cleanup required).
 * You must call RS_DEFER_SCOPE_END() to execute cleanup.
 *
 * Example:
 *   RS_DEFER_SCOPE_BEGIN() {
 *       FILE *f = fopen("test.txt", "r");
 *       RS_DEFER_ADD(fclose, f);
 *       // ... use file ...
 *   } RS_DEFER_SCOPE_END();
 */
#define RS_DEFER_SCOPE_BEGIN()                                                                                         \
    do {                                                                                                               \
    rs_defer_scope_t _rs_defer_scope = RS_DEFER_SCOPE_INIT()

#define RS_DEFER_SCOPE_END()                                                                                           \
    rs_defer_scope_execute(&_rs_defer_scope);                                                                          \
    }                                                                                                                  \
    while (0)

/**
 * Add a defer action in the current scope block.
 * Must be used within RS_DEFER_SCOPE_BEGIN/END.
 */
#define RS_DEFER_ADD(fn, data) rs_defer_scope_add(&_rs_defer_scope, (rs_defer_fn)(fn), (void *)(data))

#endif // RS_HAS_CLEANUP_ATTRIBUTE

// ============================================================================
// Advanced: Statement Expressions (GCC/Clang only)
// ============================================================================

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)

/**
 * Defer a code block (GCC/Clang with statement expressions).
 * This allows inline cleanup code instead of requiring a function.
 *
 * Example:
 *   RS_SCOPED rs_defer_scope_t defer = RS_DEFER_SCOPE_INIT();
 *   FILE *f = fopen("test.txt", "r");
 *   RS_DEFER(&defer, { fclose(f); });
 *
 * @param scope Pointer to defer scope
 * @param code Block of code to execute on scope exit
 */
#define RS_DEFER(scope, code)                                                                                          \
    rs_defer_scope_add((scope), ({                                                                                     \
                           static void _rs_defer_fn(void *_rs_unused)                                                  \
                           {                                                                                           \
                               (void)_rs_unused;                                                                       \
                               code;                                                                                   \
                           }                                                                                           \
                           _rs_defer_fn;                                                                               \
                       }),                                                                                             \
                       NULL)

#endif // __GNUC__

// ============================================================================
// Common Helper Wrappers
// ============================================================================

/**
 * Defer free() for a pointer.
 * Convenience wrapper for memory deallocation.
 *
 * @param scope Defer scope
 * @param ptr Pointer to free
 * @return RS_OK on success
 */
static inline rs_result_t rs_defer_free(rs_defer_scope_t *scope, void *ptr)
{
    return rs_defer_scope_add(scope, free, ptr);
}

RS_EXTERN_C_END
