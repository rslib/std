#pragma once

#include <rs/std/allocators/allocator.h>
#include <rs/std/internal/api.h>
#include <rs/std/string/string_view.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN

/**
 * Opaque SQLite database handle.
 * Uses allocator for all internal allocations.
 */
typedef struct rs_db_sqlite_t rs_db_sqlite_t;

// ============================================================================
// Database Lifecycle
// ============================================================================

/**
 * SQLite database open flags.
 */
typedef enum {
    RS_DB_SQLITE_OPEN_READONLY = 1 << 0,  // Open database read-only
    RS_DB_SQLITE_OPEN_READWRITE = 1 << 1, // Open database for reading and writing
    RS_DB_SQLITE_OPEN_CREATE = 1 << 2,    // Create database if it doesn't exist
    RS_DB_SQLITE_OPEN_MEMORY = 1 << 3,    // Open in-memory database
    RS_DB_SQLITE_OPEN_NOMUTEX = 1 << 4,   // Open without mutex
    RS_DB_SQLITE_OPEN_FULLMUTEX = 1 << 5  // Open with full mutex
} rs_db_sqlite_open_flags_t;

/**
 * Open SQLite database with allocator and flags.
 *
 * @param path Path to database file (or ":memory:" for in-memory DB)
 * @param flags Database open flags (e.g., RS_DB_SQLITE_OPEN_READWRITE | RS_DB_SQLITE_OPEN_CREATE)
 *              Pass 0 to use default: RS_DB_SQLITE_OPEN_READWRITE | RS_DB_SQLITE_OPEN_CREATE
 * @param allocator Allocator for internal allocations (NULL = system allocator)
 * @return Database handle or NULL on error
 */
RS_STD_API rs_db_sqlite_t *rs_db_sqlite_open_flags(const char *path, int flags, rs_allocator_t *allocator);

/**
 * Open SQLite database with default flags (READWRITE | CREATE).
 *
 * @param path Path to database file (or ":memory:" for in-memory DB)
 * @param allocator Allocator for internal allocations (NULL = system allocator)
 * @return Database handle or NULL on error
 */
RS_STD_API rs_db_sqlite_t *rs_db_sqlite_open(const char *path, rs_allocator_t *allocator);

/**
 * Open SQLite database with system allocator (convenience macro).
 * Equivalent to: rs_db_sqlite_open(path, NULL)
 *
 * Usage:
 *   rs_db_sqlite_t *db = rs_db_sqlite_open_default("state.db");
 */
#define rs_db_sqlite_open_default(path) rs_db_sqlite_open((path), NULL)

/**
 * Close database and free resources using the allocator.
 */
RS_STD_API void rs_db_sqlite_close(rs_db_sqlite_t *db);

// ============================================================================
// Simple Execution
// ============================================================================

/**
 * Execute SQL statement (no results).
 * Accepts rs_string_view_t which may not be null-terminated.
 *
 * @param db Database handle
 * @param sql SQL statement as string view
 * @return RS_OK on success, error code on failure
 */
RS_STD_API rs_result_t rs_db_sqlite_exec(rs_db_sqlite_t *db, rs_string_view_t sql);

/**
 * Helper macro for convenient execution with const char*.
 * Usage:
 *   rs_db_sqlite_exec(db, rs_sv_from_cstr("CREATE TABLE ..."));
 *   rs_db_sqlite_exec_cstr(db, "CREATE TABLE ...");  // Convenience macro
 */
#define rs_db_sqlite_exec_cstr(db, sql) rs_db_sqlite_exec((db), rs_sv_from_cstr(sql))

// ============================================================================
// Transactions
// ============================================================================

/**
 * Begin transaction.
 */
RS_STD_API rs_result_t rs_db_sqlite_begin(rs_db_sqlite_t *db);

/**
 * Commit transaction.
 */
RS_STD_API rs_result_t rs_db_sqlite_commit(rs_db_sqlite_t *db);

/**
 * Rollback transaction.
 */
RS_STD_API rs_result_t rs_db_sqlite_rollback(rs_db_sqlite_t *db);

// ============================================================================
// Metadata
// ============================================================================

/**
 * Get last insert rowid.
 */
RS_STD_API rs_i64 rs_db_sqlite_last_insers_id(rs_db_sqlite_t *db);

/**
 * Get number of rows changed by last statement.
 */
RS_STD_API rs_i32 rs_db_sqlite_changes(rs_db_sqlite_t *db);

// ============================================================================
// Prepared Statements
// ============================================================================

/**
 * Opaque prepared statement handle.
 */
typedef struct rs_db_sqlite_stmt_t rs_db_sqlite_stmt_t;

/**
 * Prepare SQL statement for execution.
 *
 * @param db Database handle
 * @param sql SQL statement as string view
 * @return Statement handle or NULL on error
 */
RS_STD_API rs_db_sqlite_stmt_t *rs_db_sqlite_prepare(rs_db_sqlite_t *db, rs_string_view_t sql);

/**
 * Helper macro to prepare statement from const char*.
 * Usage: rs_db_sqlite_prepare_cstr(db, "SELECT * FROM users WHERE id = ?")
 */
#define rs_db_sqlite_prepare_cstr(db, sql) rs_db_sqlite_prepare((db), rs_sv_from_cstr(sql))

/**
 * Finalize (destroy) prepared statement and free resources.
 */
RS_STD_API void rs_db_sqlite_stmt_finalize(rs_db_sqlite_stmt_t *stmt);

/**
 * Reset statement to initial state for re-execution.
 */
RS_STD_API rs_result_t rs_db_sqlite_stmt_reset(rs_db_sqlite_stmt_t *stmt);

/**
 * Clear all bindings on a prepared statement.
 */
RS_STD_API rs_result_t rs_db_sqlite_stmt_clear_bindings(rs_db_sqlite_stmt_t *stmt);

// ----------------------------------------------------------------------------
// Parameter Binding
// ----------------------------------------------------------------------------

/**
 * Bind NULL to parameter.
 * @param stmt Statement handle
 * @param index Parameter index (1-based)
 */
RS_STD_API rs_result_t rs_db_sqlite_bind_null(rs_db_sqlite_stmt_t *stmt, int index);

/**
 * Bind integer to parameter.
 */
RS_STD_API rs_result_t rs_db_sqlite_bind_int(rs_db_sqlite_stmt_t *stmt, int index, rs_i32 value);

/**
 * Bind 64-bit integer to parameter.
 */
RS_STD_API rs_result_t rs_db_sqlite_bind_int64(rs_db_sqlite_stmt_t *stmt, int index, rs_i64 value);

/**
 * Bind double to parameter.
 */
RS_STD_API rs_result_t rs_db_sqlite_bind_double(rs_db_sqlite_stmt_t *stmt, int index, double value);

/**
 * Bind text string (copied).
 * @param stmt Statement handle
 * @param index Parameter index (1-based)
 * @param text String view of text to bind (will be copied by SQLite)
 */
RS_STD_API rs_result_t rs_db_sqlite_bind_text(rs_db_sqlite_stmt_t *stmt, int index, rs_string_view_t text);

/**
 * Helper macro to bind const char* as text.
 */
#define rs_db_sqlite_bind_text_cstr(stmt, index, text) rs_db_sqlite_bind_text((stmt), (index), rs_sv_from_cstr(text))

/**
 * Bind blob data (copied).
 * @param stmt Statement handle
 * @param index Parameter index (1-based)
 * @param data Pointer to blob data
 * @param size Size of blob in bytes
 */
RS_STD_API rs_result_t rs_db_sqlite_bind_blob(rs_db_sqlite_stmt_t *stmt, int index, const void *data, rs_usize size);

// ----------------------------------------------------------------------------
// Statement Execution
// ----------------------------------------------------------------------------

/**
 * Execute statement and advance to next row.
 * @return RS_OK if row available, RS_DONE if no more rows, error code on failure
 */
RS_STD_API rs_result_t rs_db_sqlite_stmt_step(rs_db_sqlite_stmt_t *stmt);

/**
 * Get number of columns in result set.
 */
RS_STD_API int rs_db_sqlite_stmt_column_count(rs_db_sqlite_stmt_t *stmt);

/**
 * Get column name.
 * @param stmt Statement handle
 * @param index Column index (0-based)
 * @return Column name (valid until statement is finalized)
 */
RS_STD_API const char *rs_db_sqlite_stmt_column_name(rs_db_sqlite_stmt_t *stmt, int index);

// ----------------------------------------------------------------------------
// Column Access
// ----------------------------------------------------------------------------

/**
 * Column data types.
 */
typedef enum {
    RS_DB_SQLITE_TYPE_INTEGER = 1,
    RS_DB_SQLITE_TYPE_FLOAT = 2,
    RS_DB_SQLITE_TYPE_TEXT = 3,
    RS_DB_SQLITE_TYPE_BLOB = 4,
    RS_DB_SQLITE_TYPE_NULL = 5
} rs_db_sqlite_column_type_t;

/**
 * Get column type.
 */
RS_STD_API rs_db_sqlite_column_type_t rs_db_sqlite_column_type(rs_db_sqlite_stmt_t *stmt, int index);

/**
 * Get integer column value.
 */
RS_STD_API rs_i32 rs_db_sqlite_column_int(rs_db_sqlite_stmt_t *stmt, int index);

/**
 * Get 64-bit integer column value.
 */
RS_STD_API rs_i64 rs_db_sqlite_column_int64(rs_db_sqlite_stmt_t *stmt, int index);

/**
 * Get double column value.
 */
RS_STD_API double rs_db_sqlite_column_double(rs_db_sqlite_stmt_t *stmt, int index);

/**
 * Get text column value as string view.
 * Note: Returned string_view is valid until next call to step/reset/finalize.
 */
RS_STD_API rs_string_view_t rs_db_sqlite_column_text(rs_db_sqlite_stmt_t *stmt, int index);

/**
 * Get blob column value.
 * Note: Returned pointer is valid until next call to step/reset/finalize.
 * @param stmt Statement handle
 * @param index Column index (0-based)
 * @param size Output parameter for blob size (optional, pass NULL to ignore)
 * @return Pointer to blob data
 */
RS_STD_API const void *rs_db_sqlite_column_blob(rs_db_sqlite_stmt_t *stmt, int index, rs_usize *size);

// ============================================================================
// Advanced Access
// ============================================================================

/**
 * Get underlying sqlite3* handle (for advanced usage).
 * Allows using raw SQLite3 API when needed.
 * @return void* that can be cast to sqlite3*
 */
RS_STD_API void *rs_db_sqlite_handle(rs_db_sqlite_t *db);

/**
 * Get underlying sqlite3_stmt* handle (for advanced usage).
 * @return void* that can be cast to sqlite3_stmt*
 */
RS_STD_API void *rs_db_sqlite_stmt_handle(rs_db_sqlite_stmt_t *stmt);

/**
 * Get allocator used by this database.
 */
RS_STD_API rs_allocator_t *rs_db_sqlite_allocator(rs_db_sqlite_t *db);

RS_EXTERN_C_END
