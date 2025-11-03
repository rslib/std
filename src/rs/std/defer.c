#include <rs/std/defer.h>
#include <rs/std/error.h>
#include <string.h>

// ============================================================================
// Core Implementation
// ============================================================================

void rs_defer_scope_init(rs_defer_scope_t *scope)
{
    if (!scope)
        return;

    scope->count = 0;
    // Zero out the actions array for safety
    memset(scope->actions, 0, sizeof(scope->actions));
}

rs_result_t rs_defer_scope_add(rs_defer_scope_t *scope, rs_defer_fn fn, void *data)
{
    if (!scope || !fn) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Invalid defer scope or function");
    }

    if (scope->count >= RS_DEFER_MAX_ACTIONS) {
        return RS_ERROR_RET(RS_ERR_OVERFLOW, "Defer scope full (max %d actions)", RS_DEFER_MAX_ACTIONS);
    }

    // Add action to the end (will be executed in reverse order)
    scope->actions[scope->count].fn = fn;
    scope->actions[scope->count].data = data;
    scope->count++;

    return RS_OK;
}

void rs_defer_scope_execute(rs_defer_scope_t *scope)
{
    if (!scope)
        return;

    // Execute in reverse order (LIFO - last in, first out)
    // This ensures that resources are cleaned up in the opposite
    // order they were acquired, which is usually what you want
    for (rs_size_t i = scope->count; i > 0; i--) {
        rs_defer_action_t *action = &scope->actions[i - 1];
        if (action->fn) {
            action->fn(action->data);
        }
    }

    // Clear the scope after execution
    scope->count = 0;
}

void rs_defer_scope_clear(rs_defer_scope_t *scope)
{
    if (!scope)
        return;

    // Just reset the count without executing
    scope->count = 0;
}

void rs_defer_scope_cleanup(rs_defer_scope_t *scope)
{
    // This is the cleanup function called by __attribute__((cleanup))
    // It's the same as execute, but with a different name for clarity
    rs_defer_scope_execute(scope);
}
