#include <rs/std/db/sqlite.h>
#include <rs/std/error.h>
#include <rs/std/string/string_view.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

struct rs_db_sqlite_t {
    sqlite3 *db;
    rs_allocator_t *allocator;
};

struct rs_db_sqlite_stmt_t {
    sqlite3_stmt *stmt;
    rs_db_sqlite_t *db;
};

// ============================================================================
// Database Lifecycle
// ============================================================================

rs_db_sqlite_t *rs_db_sqlite_open_flags(const char *path, int flags, rs_allocator_t *allocator)
{
    if (!path) {
        RS_ERROR(RS_ERR_INVALID, "Database path cannot be NULL");
        return NULL;
    }

    // Use system allocator if none provided
    if (!allocator) {
        allocator = rs_allocator_system();
    }

    // Map our flags to SQLite flags
    int sqlite_flags = 0;
    if (flags == 0) {
        // Default: READWRITE | CREATE
        sqlite_flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    } else {
        if (flags & RS_DB_SQLITE_OPEN_READONLY) {
            sqlite_flags |= SQLITE_OPEN_READONLY;
        }
        if (flags & RS_DB_SQLITE_OPEN_READWRITE) {
            sqlite_flags |= SQLITE_OPEN_READWRITE;
        }
        if (flags & RS_DB_SQLITE_OPEN_CREATE) {
            sqlite_flags |= SQLITE_OPEN_CREATE;
        }
        if (flags & RS_DB_SQLITE_OPEN_MEMORY) {
            sqlite_flags |= SQLITE_OPEN_MEMORY;
        }
        if (flags & RS_DB_SQLITE_OPEN_NOMUTEX) {
            sqlite_flags |= SQLITE_OPEN_NOMUTEX;
        }
        if (flags & RS_DB_SQLITE_OPEN_FULLMUTEX) {
            sqlite_flags |= SQLITE_OPEN_FULLMUTEX;
        }
    }

    // Allocate database handle
    rs_db_sqlite_t *db = rs_alloc_type(allocator, rs_db_sqlite_t);
    if (!db) {
        RS_ERROR(RS_ERR_NOMEM, "Failed to allocate database handle");
        return NULL;
    }

    db->allocator = allocator;
    db->db = NULL;

    // Open SQLite database with flags
    int rc = sqlite3_open_v2(path, &db->db, sqlite_flags, NULL);
    if (rc != SQLITE_OK) {
        const char *errmsg = db->db ? sqlite3_errmsg(db->db) : "Unknown error";
        RS_ERROR(RS_ERR_DB, "Failed to open database '%s': %s", path, errmsg);

        if (db->db) {
            sqlite3_close(db->db);
        }
        rs_free(allocator, db, sizeof(rs_db_sqlite_t));
        return NULL;
    }

    return db;
}

rs_db_sqlite_t *rs_db_sqlite_open(const char *path, rs_allocator_t *allocator)
{
    return rs_db_sqlite_open_flags(path, RS_DB_SQLITE_OPEN_READWRITE | RS_DB_SQLITE_OPEN_CREATE, allocator);
}

void rs_db_sqlite_close(rs_db_sqlite_t *db)
{
    if (!db) {
        return;
    }

    if (db->db) {
        sqlite3_close(db->db);
    }

    rs_allocator_t *allocator = db->allocator;
    rs_free(allocator, db, sizeof(rs_db_sqlite_t));
}

// ============================================================================
// Simple Execution
// ============================================================================

rs_result_t rs_db_sqlite_exec(rs_db_sqlite_t *db, rs_string_view_t sql)
{
    if (!db || !db->db) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Database handle is NULL");
    }

    if (!sql.data || sql.len == 0) {
        return RS_ERROR_RET(RS_ERR_INVALID, "SQL statement is NULL or empty");
    }

    // SQLite3's sqlite3_prepare_v2 supports length parameter for non-null-terminated strings
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db->db, sql.data, (int)sql.len, &stmt, NULL);

    if (rc != SQLITE_OK) {
        return RS_ERROR_RET(RS_ERR_DB, "SQL preparation failed: %s", sqlite3_errmsg(db->db));
    }

    // Execute the statement
    rc = sqlite3_step(stmt);

    // SQLITE_DONE means successful execution with no result rows
    // SQLITE_ROW would mean there are results (but exec shouldn't return rows)
    if (rc != SQLITE_DONE) {
        rs_result_t err = RS_ERROR_RET(RS_ERR_DB, "SQL execution failed: %s", sqlite3_errmsg(db->db));
        sqlite3_finalize(stmt);
        return err;
    }

    sqlite3_finalize(stmt);
    return RS_OK;
}

// ============================================================================
// Transactions
// ============================================================================

rs_result_t rs_db_sqlite_begin(rs_db_sqlite_t *db)
{
    return rs_db_sqlite_exec(db, rs_sv_from_cstr("BEGIN TRANSACTION"));
}

rs_result_t rs_db_sqlite_commit(rs_db_sqlite_t *db)
{
    return rs_db_sqlite_exec(db, rs_sv_from_cstr("COMMIT"));
}

rs_result_t rs_db_sqlite_rollback(rs_db_sqlite_t *db)
{
    return rs_db_sqlite_exec(db, rs_sv_from_cstr("ROLLBACK"));
}

// ============================================================================
// Metadata
// ============================================================================

rs_i64 rs_db_sqlite_last_insers_id(rs_db_sqlite_t *db)
{
    if (!db || !db->db) {
        RS_ERROR(RS_ERR_INVALID, "Database handle is NULL");
        return -1;
    }

    return (rs_i64)sqlite3_last_insert_rowid(db->db);
}

rs_i32 rs_db_sqlite_changes(rs_db_sqlite_t *db)
{
    if (!db || !db->db) {
        RS_ERROR(RS_ERR_INVALID, "Database handle is NULL");
        return -1;
    }

    return (rs_i32)sqlite3_changes(db->db);
}

// ============================================================================
// Prepared Statements
// ============================================================================

rs_db_sqlite_stmt_t *rs_db_sqlite_prepare(rs_db_sqlite_t *db, rs_string_view_t sql)
{
    if (!db || !db->db) {
        RS_ERROR(RS_ERR_INVALID, "Database handle is NULL");
        return NULL;
    }

    if (!sql.data || sql.len == 0) {
        RS_ERROR(RS_ERR_INVALID, "SQL statement is NULL or empty");
        return NULL;
    }

    // Allocate statement handle
    rs_db_sqlite_stmt_t *stmt = rs_alloc_type(db->allocator, rs_db_sqlite_stmt_t);
    if (!stmt) {
        RS_ERROR(RS_ERR_NOMEM, "Failed to allocate statement handle");
        return NULL;
    }

    stmt->db = db;
    stmt->stmt = NULL;

    // Prepare statement
    int rc = sqlite3_prepare_v2(db->db, sql.data, (int)sql.len, &stmt->stmt, NULL);
    if (rc != SQLITE_OK) {
        RS_ERROR(RS_ERR_DB, "Failed to prepare statement: %s", sqlite3_errmsg(db->db));
        rs_free(db->allocator, stmt, sizeof(rs_db_sqlite_stmt_t));
        return NULL;
    }

    return stmt;
}

void rs_db_sqlite_stmt_finalize(rs_db_sqlite_stmt_t *stmt)
{
    if (!stmt) {
        return;
    }

    if (stmt->stmt) {
        sqlite3_finalize(stmt->stmt);
    }

    rs_allocator_t *allocator = stmt->db->allocator;
    rs_free(allocator, stmt, sizeof(rs_db_sqlite_stmt_t));
}

rs_result_t rs_db_sqlite_stmt_reset(rs_db_sqlite_stmt_t *stmt)
{
    if (!stmt || !stmt->stmt) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Statement handle is NULL");
    }

    int rc = sqlite3_reset(stmt->stmt);
    if (rc != SQLITE_OK) {
        return RS_ERROR_RET(RS_ERR_DB, "Failed to reset statement: %s", sqlite3_errmsg(stmt->db->db));
    }

    return RS_OK;
}

rs_result_t rs_db_sqlite_stmt_clear_bindings(rs_db_sqlite_stmt_t *stmt)
{
    if (!stmt || !stmt->stmt) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Statement handle is NULL");
    }

    int rc = sqlite3_clear_bindings(stmt->stmt);
    if (rc != SQLITE_OK) {
        return RS_ERROR_RET(RS_ERR_DB, "Failed to clear bindings: %s", sqlite3_errmsg(stmt->db->db));
    }

    return RS_OK;
}

// ----------------------------------------------------------------------------
// Parameter Binding
// ----------------------------------------------------------------------------

rs_result_t rs_db_sqlite_bind_null(rs_db_sqlite_stmt_t *stmt, int index)
{
    if (!stmt || !stmt->stmt) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Statement handle is NULL");
    }

    int rc = sqlite3_bind_null(stmt->stmt, index);
    if (rc != SQLITE_OK) {
        return RS_ERROR_RET(RS_ERR_DB, "Failed to bind NULL: %s", sqlite3_errmsg(stmt->db->db));
    }

    return RS_OK;
}

rs_result_t rs_db_sqlite_bind_int(rs_db_sqlite_stmt_t *stmt, int index, rs_i32 value)
{
    if (!stmt || !stmt->stmt) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Statement handle is NULL");
    }

    int rc = sqlite3_bind_int(stmt->stmt, index, value);
    if (rc != SQLITE_OK) {
        return RS_ERROR_RET(RS_ERR_DB, "Failed to bind int: %s", sqlite3_errmsg(stmt->db->db));
    }

    return RS_OK;
}

rs_result_t rs_db_sqlite_bind_int64(rs_db_sqlite_stmt_t *stmt, int index, rs_i64 value)
{
    if (!stmt || !stmt->stmt) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Statement handle is NULL");
    }

    int rc = sqlite3_bind_int64(stmt->stmt, index, value);
    if (rc != SQLITE_OK) {
        return RS_ERROR_RET(RS_ERR_DB, "Failed to bind int64: %s", sqlite3_errmsg(stmt->db->db));
    }

    return RS_OK;
}

rs_result_t rs_db_sqlite_bind_double(rs_db_sqlite_stmt_t *stmt, int index, double value)
{
    if (!stmt || !stmt->stmt) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Statement handle is NULL");
    }

    int rc = sqlite3_bind_double(stmt->stmt, index, value);
    if (rc != SQLITE_OK) {
        return RS_ERROR_RET(RS_ERR_DB, "Failed to bind double: %s", sqlite3_errmsg(stmt->db->db));
    }

    return RS_OK;
}

rs_result_t rs_db_sqlite_bind_text(rs_db_sqlite_stmt_t *stmt, int index, rs_string_view_t text)
{
    if (!stmt || !stmt->stmt) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Statement handle is NULL");
    }

    if (!text.data) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Text data is NULL");
    }

    // SQLITE_TRANSIENT tells SQLite to make a copy of the data
    int rc = sqlite3_bind_text(stmt->stmt, index, text.data, (int)text.len, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        return RS_ERROR_RET(RS_ERR_DB, "Failed to bind text: %s", sqlite3_errmsg(stmt->db->db));
    }

    return RS_OK;
}

rs_result_t rs_db_sqlite_bind_blob(rs_db_sqlite_stmt_t *stmt, int index, const void *data, rs_usize size)
{
    if (!stmt || !stmt->stmt) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Statement handle is NULL");
    }

    if (!data) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Blob data is NULL");
    }

    // SQLITE_TRANSIENT tells SQLite to make a copy of the data
    int rc = sqlite3_bind_blob(stmt->stmt, index, data, (int)size, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        return RS_ERROR_RET(RS_ERR_DB, "Failed to bind blob: %s", sqlite3_errmsg(stmt->db->db));
    }

    return RS_OK;
}

// ----------------------------------------------------------------------------
// Statement Execution
// ----------------------------------------------------------------------------

rs_result_t rs_db_sqlite_stmt_step(rs_db_sqlite_stmt_t *stmt)
{
    if (!stmt || !stmt->stmt) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Statement handle is NULL");
    }

    int rc = sqlite3_step(stmt->stmt);

    if (rc == SQLITE_ROW) {
        return RS_OK; // Row available
    } else if (rc == SQLITE_DONE) {
        return RS_DONE; // No more rows
    } else {
        return RS_ERROR_RET(RS_ERR_DB, "Statement execution failed: %s", sqlite3_errmsg(stmt->db->db));
    }
}

int rs_db_sqlite_stmt_column_count(rs_db_sqlite_stmt_t *stmt)
{
    if (!stmt || !stmt->stmt) {
        RS_ERROR(RS_ERR_INVALID, "Statement handle is NULL");
        return 0;
    }

    return sqlite3_column_count(stmt->stmt);
}

const char *rs_db_sqlite_stmt_column_name(rs_db_sqlite_stmt_t *stmt, int index)
{
    if (!stmt || !stmt->stmt) {
        RS_ERROR(RS_ERR_INVALID, "Statement handle is NULL");
        return NULL;
    }

    return sqlite3_column_name(stmt->stmt, index);
}

// ----------------------------------------------------------------------------
// Column Access
// ----------------------------------------------------------------------------

rs_db_sqlite_column_type_t rs_db_sqlite_column_type(rs_db_sqlite_stmt_t *stmt, int index)
{
    if (!stmt || !stmt->stmt) {
        RS_ERROR(RS_ERR_INVALID, "Statement handle is NULL");
        return RS_DB_SQLITE_TYPE_NULL;
    }

    return (rs_db_sqlite_column_type_t)sqlite3_column_type(stmt->stmt, index);
}

rs_i32 rs_db_sqlite_column_int(rs_db_sqlite_stmt_t *stmt, int index)
{
    if (!stmt || !stmt->stmt) {
        RS_ERROR(RS_ERR_INVALID, "Statement handle is NULL");
        return 0;
    }

    return (rs_i32)sqlite3_column_int(stmt->stmt, index);
}

rs_i64 rs_db_sqlite_column_int64(rs_db_sqlite_stmt_t *stmt, int index)
{
    if (!stmt || !stmt->stmt) {
        RS_ERROR(RS_ERR_INVALID, "Statement handle is NULL");
        return 0;
    }

    return (rs_i64)sqlite3_column_int64(stmt->stmt, index);
}

double rs_db_sqlite_column_double(rs_db_sqlite_stmt_t *stmt, int index)
{
    if (!stmt || !stmt->stmt) {
        RS_ERROR(RS_ERR_INVALID, "Statement handle is NULL");
        return 0.0;
    }

    return sqlite3_column_double(stmt->stmt, index);
}

rs_string_view_t rs_db_sqlite_column_text(rs_db_sqlite_stmt_t *stmt, int index)
{
    if (!stmt || !stmt->stmt) {
        RS_ERROR(RS_ERR_INVALID, "Statement handle is NULL");
        return (rs_string_view_t){NULL, 0};
    }

    const unsigned char *text = sqlite3_column_text(stmt->stmt, index);
    int len = sqlite3_column_bytes(stmt->stmt, index);

    return (rs_string_view_t){(const char *)text, (rs_usize)len};
}

const void *rs_db_sqlite_column_blob(rs_db_sqlite_stmt_t *stmt, int index, rs_usize *size)
{
    if (!stmt || !stmt->stmt) {
        RS_ERROR(RS_ERR_INVALID, "Statement handle is NULL");
        if (size) {
            *size = 0;
        }
        return NULL;
    }

    const void *blob = sqlite3_column_blob(stmt->stmt, index);
    int len = sqlite3_column_bytes(stmt->stmt, index);

    if (size) {
        *size = (rs_usize)len;
    }

    return blob;
}

// ============================================================================
// Advanced Access
// ============================================================================

void *rs_db_sqlite_handle(rs_db_sqlite_t *db)
{
    return db ? (void *)db->db : NULL;
}

void *rs_db_sqlite_stmt_handle(rs_db_sqlite_stmt_t *stmt)
{
    return stmt ? (void *)stmt->stmt : NULL;
}

rs_allocator_t *rs_db_sqlite_allocator(rs_db_sqlite_t *db)
{
    return db ? db->allocator : NULL;
}
