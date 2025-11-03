#include <rs/std/allocators/allocator.h>
#include <rs/std/db/sqlite.h>
#include <rs/std/error.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string_view.h>
#include <rs/std/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_sqlite"

static rs_allocator_t *allocator;

void setUp(void)
{
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
    allocator = rs_allocator_system();
}

void tearDown(void)
{
    rs_log_shutdown();
}

// ============================================================================
// Basic Operations Tests
// ============================================================================

void test_sqlite_open_memory(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open(":memory:", allocator);
    TEST_ASSERT_NOT_NULL(db);

    // Verify we got a valid handle
    void *handle = rs_db_sqlite_handle(db);
    TEST_ASSERT_NOT_NULL(handle);

    // Verify allocator is stored
    rs_allocator_t *db_alloc = rs_db_sqlite_allocator(db);
    TEST_ASSERT_EQUAL_PTR(allocator, db_alloc);

    rs_db_sqlite_close(db);
}

void test_sqlite_open_default(void)
{
    // Test convenience macro with system allocator
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    // Should use system allocator
    rs_allocator_t *db_alloc = rs_db_sqlite_allocator(db);
    TEST_ASSERT_EQUAL_PTR(rs_allocator_system(), db_alloc);

    rs_db_sqlite_close(db);
}

void test_sqlite_open_invalid_path(void)
{
    // Try to open database with NULL path
    rs_db_sqlite_t *db = rs_db_sqlite_open(NULL, allocator);
    TEST_ASSERT_NULL(db);

    // Should have error set
    const rs_error_t *err = rs_last_error();
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_EQUAL(RS_ERR_INVALID, err->code);
}

void test_sqlite_close_null(void)
{
    // Closing NULL should be safe (no-op)
    rs_db_sqlite_close(NULL);
    TEST_ASSERT_TRUE(1); // If we get here, didn't crash
}

// ============================================================================
// SQL Execution Tests
// ============================================================================

void test_sqlite_exec_create_table(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    // Create table using const char*
    rs_result_t result = rs_db_sqlite_exec_cstr(db, "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)");
    TEST_ASSERT_EQUAL(RS_OK, result);

    rs_db_sqlite_close(db);
}

void test_sqlite_exec_with_string_view(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    // Create SQL as string_view
    rs_string_view_t sql = rs_sv_from_cstr("CREATE TABLE test (id INTEGER)");

    // Execute using string_view directly
    rs_result_t result = rs_db_sqlite_exec(db, sql);
    TEST_ASSERT_EQUAL(RS_OK, result);

    rs_db_sqlite_close(db);
}

void test_sqlite_exec_invalid_sql(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    // Try to execute invalid SQL
    rs_result_t result = rs_db_sqlite_exec_cstr(db, "INVALID SQL STATEMENT");
    TEST_ASSERT_EQUAL(RS_ERR_DB, result);

    // Should have error message
    const rs_error_t *err = rs_last_error();
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_EQUAL(RS_ERR_DB, err->code);

    rs_db_sqlite_close(db);
}

void test_sqlite_exec_null_db(void)
{
    rs_result_t result = rs_db_sqlite_exec_cstr(NULL, "SELECT 1");
    TEST_ASSERT_EQUAL(RS_ERR_INVALID, result);
}

void test_sqlite_exec_null_sql(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    // Create an empty string_view
    rs_string_view_t empty_sql = {NULL, 0};
    rs_result_t result = rs_db_sqlite_exec(db, empty_sql);
    TEST_ASSERT_EQUAL(RS_ERR_INVALID, result);

    rs_db_sqlite_close(db);
}

// ============================================================================
// Transaction Tests
// ============================================================================

void test_sqlite_transaction_commit(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    // Create table
    rs_db_sqlite_exec_cstr(db, "CREATE TABLE test (id INTEGER)");

    // Begin transaction
    rs_result_t result = rs_db_sqlite_begin(db);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Insert data
    rs_db_sqlite_exec_cstr(db, "INSERT INTO test (id) VALUES (1)");
    rs_db_sqlite_exec_cstr(db, "INSERT INTO test (id) VALUES (2)");

    // Commit
    result = rs_db_sqlite_commit(db);
    TEST_ASSERT_EQUAL(RS_OK, result);

    rs_db_sqlite_close(db);
}

void test_sqlite_transaction_rollback(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    // Create table
    rs_db_sqlite_exec_cstr(db, "CREATE TABLE test (id INTEGER)");

    // Begin transaction
    rs_result_t result = rs_db_sqlite_begin(db);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Insert data
    rs_db_sqlite_exec_cstr(db, "INSERT INTO test (id) VALUES (1)");

    // Rollback
    result = rs_db_sqlite_rollback(db);
    TEST_ASSERT_EQUAL(RS_OK, result);

    rs_db_sqlite_close(db);
}

// ============================================================================
// Metadata Tests
// ============================================================================

void test_sqlite_last_insers_id(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    // Create table
    rs_db_sqlite_exec_cstr(db, "CREATE TABLE test (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT)");

    // Insert row
    rs_db_sqlite_exec_cstr(db, "INSERT INTO test (name) VALUES ('Alice')");

    // Get last insert ID
    rs_i64 id = rs_db_sqlite_last_insers_id(db);
    TEST_ASSERT_EQUAL(1, id);

    // Insert another row
    rs_db_sqlite_exec_cstr(db, "INSERT INTO test (name) VALUES ('Bob')");
    id = rs_db_sqlite_last_insers_id(db);
    TEST_ASSERT_EQUAL(2, id);

    rs_db_sqlite_close(db);
}

void test_sqlite_changes(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    // Create table
    rs_db_sqlite_exec_cstr(db, "CREATE TABLE test (id INTEGER, name TEXT)");

    // Insert row
    rs_db_sqlite_exec_cstr(db, "INSERT INTO test (id, name) VALUES (1, 'Alice')");

    rs_i32 changes = rs_db_sqlite_changes(db);
    TEST_ASSERT_EQUAL(1, changes);

    // Insert multiple rows in one statement
    rs_db_sqlite_exec_cstr(db, "INSERT INTO test (id, name) VALUES (2, 'Bob'), (3, 'Charlie')");

    changes = rs_db_sqlite_changes(db);
    TEST_ASSERT_EQUAL(2, changes);

    rs_db_sqlite_close(db);
}

// ============================================================================
// Integration Tests
// ============================================================================

void test_sqlite_integration_workflow(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    // Create users table
    rs_result_t result = rs_db_sqlite_exec_cstr(
        db, "CREATE TABLE users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, age INTEGER)");
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Begin transaction
    result = rs_db_sqlite_begin(db);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Insert users
    rs_db_sqlite_exec_cstr(db, "INSERT INTO users (name, age) VALUES ('Alice', 30)");
    rs_i64 alice_id = rs_db_sqlite_last_insers_id(db);
    TEST_ASSERT_EQUAL(1, alice_id);

    rs_db_sqlite_exec_cstr(db, "INSERT INTO users (name, age) VALUES ('Bob', 25)");
    rs_i64 bob_id = rs_db_sqlite_last_insers_id(db);
    TEST_ASSERT_EQUAL(2, bob_id);

    // Commit transaction
    result = rs_db_sqlite_commit(db);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Update Bob's age
    rs_db_sqlite_exec_cstr(db, "UPDATE users SET age = 26 WHERE id = 2");
    rs_i32 changes = rs_db_sqlite_changes(db);
    TEST_ASSERT_EQUAL(1, changes);

    // Create index
    rs_db_sqlite_exec_cstr(db, "CREATE INDEX idx_users_name ON users(name)");

    rs_db_sqlite_close(db);
}

void test_sqlite_multiple_databases(void)
{
    // Open two separate in-memory databases
    rs_db_sqlite_t *db1 = rs_db_sqlite_open_default(":memory:");
    rs_db_sqlite_t *db2 = rs_db_sqlite_open_default(":memory:");

    TEST_ASSERT_NOT_NULL(db1);
    TEST_ASSERT_NOT_NULL(db2);
    TEST_ASSERT_NOT_EQUAL(db1, db2);

    // Create different tables in each
    rs_db_sqlite_exec_cstr(db1, "CREATE TABLE db1_table (id INTEGER)");
    rs_db_sqlite_exec_cstr(db2, "CREATE TABLE db2_table (id INTEGER)");

    // Verify they're independent
    rs_result_t result = rs_db_sqlite_exec_cstr(db1, "INSERT INTO db2_table VALUES (1)");
    TEST_ASSERT_EQUAL(RS_ERR_DB, result); // Should fail - table doesn't exist

    rs_db_sqlite_close(db1);
    rs_db_sqlite_close(db2);
}

// ============================================================================
// Prepared Statement Tests
// ============================================================================

void test_sqlite_prepare_statement(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    // Create table
    rs_db_sqlite_exec_cstr(db, "CREATE TABLE test (id INTEGER, name TEXT)");

    // Prepare statement
    rs_db_sqlite_stmt_t *stmt = rs_db_sqlite_prepare_cstr(db, "INSERT INTO test (id, name) VALUES (?, ?)");
    TEST_ASSERT_NOT_NULL(stmt);

    // Bind values
    rs_result_t result = rs_db_sqlite_bind_int(stmt, 1, 42);
    TEST_ASSERT_EQUAL(RS_OK, result);

    result = rs_db_sqlite_bind_text_cstr(stmt, 2, "Alice");
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Execute
    result = rs_db_sqlite_stmt_step(stmt);
    TEST_ASSERT_EQUAL(RS_DONE, result);

    rs_db_sqlite_stmt_finalize(stmt);
    rs_db_sqlite_close(db);
}

void test_sqlite_prepare_select(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    // Create and populate table
    rs_db_sqlite_exec_cstr(db, "CREATE TABLE users (id INTEGER, name TEXT, age INTEGER)");
    rs_db_sqlite_exec_cstr(db, "INSERT INTO users VALUES (1, 'Alice', 30)");
    rs_db_sqlite_exec_cstr(db, "INSERT INTO users VALUES (2, 'Bob', 25)");

    // Prepare SELECT statement
    rs_db_sqlite_stmt_t *stmt = rs_db_sqlite_prepare_cstr(db, "SELECT id, name, age FROM users WHERE id = ?");
    TEST_ASSERT_NOT_NULL(stmt);

    // Bind parameter
    rs_db_sqlite_bind_int(stmt, 1, 1);

    // Execute and fetch
    rs_result_t result = rs_db_sqlite_stmt_step(stmt);
    TEST_ASSERT_EQUAL(RS_OK, result); // Row available

    // Get column count
    int col_count = rs_db_sqlite_stmt_column_count(stmt);
    TEST_ASSERT_EQUAL(3, col_count);

    // Get column values
    rs_i32 id = rs_db_sqlite_column_int(stmt, 0);
    TEST_ASSERT_EQUAL(1, id);

    rs_string_view_t name = rs_db_sqlite_column_text(stmt, 1);
    TEST_ASSERT_EQUAL_STRING("Alice", name.data);

    rs_i32 age = rs_db_sqlite_column_int(stmt, 2);
    TEST_ASSERT_EQUAL(30, age);

    // No more rows
    result = rs_db_sqlite_stmt_step(stmt);
    TEST_ASSERT_EQUAL(RS_DONE, result);

    rs_db_sqlite_stmt_finalize(stmt);
    rs_db_sqlite_close(db);
}

void test_sqlite_prepare_multiple_rows(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    // Create and populate table
    rs_db_sqlite_exec_cstr(db, "CREATE TABLE test (value INTEGER)");
    rs_db_sqlite_exec_cstr(db, "INSERT INTO test VALUES (10), (20), (30)");

    // Prepare SELECT
    rs_db_sqlite_stmt_t *stmt = rs_db_sqlite_prepare_cstr(db, "SELECT value FROM test ORDER BY value");
    TEST_ASSERT_NOT_NULL(stmt);

    // Fetch all rows
    rs_result_t result = rs_db_sqlite_stmt_step(stmt);
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(10, rs_db_sqlite_column_int(stmt, 0));

    result = rs_db_sqlite_stmt_step(stmt);
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(20, rs_db_sqlite_column_int(stmt, 0));

    result = rs_db_sqlite_stmt_step(stmt);
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(30, rs_db_sqlite_column_int(stmt, 0));

    result = rs_db_sqlite_stmt_step(stmt);
    TEST_ASSERT_EQUAL(RS_DONE, result);

    rs_db_sqlite_stmt_finalize(stmt);
    rs_db_sqlite_close(db);
}

void test_sqlite_stmt_reset(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    rs_db_sqlite_exec_cstr(db, "CREATE TABLE test (id INTEGER)");
    rs_db_sqlite_exec_cstr(db, "INSERT INTO test VALUES (1)");

    rs_db_sqlite_stmt_t *stmt = rs_db_sqlite_prepare_cstr(db, "SELECT * FROM test");
    TEST_ASSERT_NOT_NULL(stmt);

    // First execution
    rs_result_t result = rs_db_sqlite_stmt_step(stmt);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Reset
    result = rs_db_sqlite_stmt_reset(stmt);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Second execution
    result = rs_db_sqlite_stmt_step(stmt);
    TEST_ASSERT_EQUAL(RS_OK, result);

    rs_db_sqlite_stmt_finalize(stmt);
    rs_db_sqlite_close(db);
}

void test_sqlite_bind_types(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    rs_db_sqlite_exec_cstr(db, "CREATE TABLE test (i INTEGER, f REAL, t TEXT, b BLOB, n INTEGER)");

    rs_db_sqlite_stmt_t *stmt = rs_db_sqlite_prepare_cstr(db, "INSERT INTO test VALUES (?, ?, ?, ?, ?)");
    TEST_ASSERT_NOT_NULL(stmt);

    // Bind different types
    rs_db_sqlite_bind_int64(stmt, 1, 9223372036854775807LL);
    rs_db_sqlite_bind_double(stmt, 2, 3.14159);
    rs_db_sqlite_bind_text_cstr(stmt, 3, "Hello");

    rs_u8 blob_data[] = {0x01, 0x02, 0x03, 0x04};
    rs_db_sqlite_bind_blob(stmt, 4, blob_data, sizeof(blob_data));

    rs_db_sqlite_bind_null(stmt, 5);

    rs_result_t result = rs_db_sqlite_stmt_step(stmt);
    TEST_ASSERT_EQUAL(RS_DONE, result);

    rs_db_sqlite_stmt_finalize(stmt);

    // Verify data
    stmt = rs_db_sqlite_prepare_cstr(db, "SELECT * FROM test");
    result = rs_db_sqlite_stmt_step(stmt);
    TEST_ASSERT_EQUAL(RS_OK, result);

    rs_i64 i = rs_db_sqlite_column_int64(stmt, 0);
    TEST_ASSERT_EQUAL(9223372036854775807LL, i);

    double f = rs_db_sqlite_column_double(stmt, 1);
    TEST_ASSERT(f > 3.14158 && f < 3.14160);

    rs_string_view_t t = rs_db_sqlite_column_text(stmt, 2);
    TEST_ASSERT_EQUAL_STRING("Hello", t.data);

    rs_usize blob_size;
    const void *blob = rs_db_sqlite_column_blob(stmt, 3, &blob_size);
    TEST_ASSERT_EQUAL(4, blob_size);
    TEST_ASSERT_EQUAL_MEMORY(blob_data, blob, 4);

    rs_db_sqlite_column_type_t type = rs_db_sqlite_column_type(stmt, 4);
    TEST_ASSERT_EQUAL(RS_DB_SQLITE_TYPE_NULL, type);

    rs_db_sqlite_stmt_finalize(stmt);
    rs_db_sqlite_close(db);
}

void test_sqlite_open_with_flags(void)
{
    // Open with READWRITE | CREATE flags
    rs_db_sqlite_t *db =
        rs_db_sqlite_open_flags(":memory:", RS_DB_SQLITE_OPEN_READWRITE | RS_DB_SQLITE_OPEN_CREATE, NULL);
    TEST_ASSERT_NOT_NULL(db);

    // Should be able to create tables
    rs_result_t result = rs_db_sqlite_exec_cstr(db, "CREATE TABLE test (id INTEGER)");
    TEST_ASSERT_EQUAL(RS_OK, result);

    rs_db_sqlite_close(db);
}

void test_sqlite_column_names(void)
{
    rs_db_sqlite_t *db = rs_db_sqlite_open_default(":memory:");
    TEST_ASSERT_NOT_NULL(db);

    rs_db_sqlite_exec_cstr(db, "CREATE TABLE test (id INTEGER, name TEXT)");
    rs_db_sqlite_exec_cstr(db, "INSERT INTO test VALUES (1, 'Alice')");

    rs_db_sqlite_stmt_t *stmt = rs_db_sqlite_prepare_cstr(db, "SELECT id, name FROM test");
    TEST_ASSERT_NOT_NULL(stmt);

    // Get column names
    const char *col0 = rs_db_sqlite_stmt_column_name(stmt, 0);
    TEST_ASSERT_EQUAL_STRING("id", col0);

    const char *col1 = rs_db_sqlite_stmt_column_name(stmt, 1);
    TEST_ASSERT_EQUAL_STRING("name", col1);

    rs_db_sqlite_stmt_finalize(stmt);
    rs_db_sqlite_close(db);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // Basic Operations
    RUN_TEST(test_sqlite_open_memory);
    RUN_TEST(test_sqlite_open_default);
    RUN_TEST(test_sqlite_open_invalid_path);
    RUN_TEST(test_sqlite_close_null);
    RUN_TEST(test_sqlite_open_with_flags);

    // SQL Execution
    RUN_TEST(test_sqlite_exec_create_table);
    RUN_TEST(test_sqlite_exec_with_string_view);
    RUN_TEST(test_sqlite_exec_invalid_sql);
    RUN_TEST(test_sqlite_exec_null_db);
    RUN_TEST(test_sqlite_exec_null_sql);

    // Transactions
    RUN_TEST(test_sqlite_transaction_commit);
    RUN_TEST(test_sqlite_transaction_rollback);

    // Metadata
    RUN_TEST(test_sqlite_last_insers_id);
    RUN_TEST(test_sqlite_changes);

    // Prepared Statements
    RUN_TEST(test_sqlite_prepare_statement);
    RUN_TEST(test_sqlite_prepare_select);
    RUN_TEST(test_sqlite_prepare_multiple_rows);
    RUN_TEST(test_sqlite_stmt_reset);
    RUN_TEST(test_sqlite_bind_types);
    RUN_TEST(test_sqlite_column_names);

    // Integration
    RUN_TEST(test_sqlite_integration_workflow);
    RUN_TEST(test_sqlite_multiple_databases);

    return UNITY_END();
}
