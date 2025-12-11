#include <rs/std/containers/list.h>
#include <rs/std/logging/logging.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_list"

// Test structure with embedded list node
typedef struct {
    int id;
    char name[32];
    rs_list_t link;
} test_item_t;

// Test structure with multiple list memberships
typedef struct {
    int value;
    rs_list_t list_a;
    rs_list_t list_b;
} multi_list_item_t;

void setUp(void)
{
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
}

void tearDown(void)
{
    rs_log_shutdown();
}

// ============================================================================
// Initialization tests
// ============================================================================

void test_list_init(void)
{
    rs_list_t list;
    rs_list_init(&list);

    TEST_ASSERT_TRUE(rs_list_empty(&list));
    TEST_ASSERT_EQUAL_PTR(&list, list.next);
    TEST_ASSERT_EQUAL_PTR(&list, list.prev);
}

void test_list_init_multiple(void)
{
    rs_list_t list1, list2, list3;

    rs_list_init(&list1);
    rs_list_init(&list2);
    rs_list_init(&list3);

    TEST_ASSERT_TRUE(rs_list_empty(&list1));
    TEST_ASSERT_TRUE(rs_list_empty(&list2));
    TEST_ASSERT_TRUE(rs_list_empty(&list3));
}

// ============================================================================
// Basic operations tests
// ============================================================================

void test_list_add_single(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t item = {.id = 1};
    rs_list_add(&list, &item.link);

    TEST_ASSERT_FALSE(rs_list_empty(&list));
    TEST_ASSERT_TRUE(rs_list_is_singular(&list));
    TEST_ASSERT_EQUAL_PTR(&item.link, list.next);
    TEST_ASSERT_EQUAL_PTR(&item.link, list.prev);
}

void test_list_add_multiple(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[3] = {{.id = 1}, {.id = 2}, {.id = 3}};

    // Add at front: list will be [3, 2, 1]
    rs_list_add(&list, &items[0].link);
    rs_list_add(&list, &items[1].link);
    rs_list_add(&list, &items[2].link);

    TEST_ASSERT_FALSE(rs_list_empty(&list));
    TEST_ASSERT_FALSE(rs_list_is_singular(&list));

    // First should be items[2] (last added)
    test_item_t *first = rs_list_first_entry(&list, test_item_t, link);
    TEST_ASSERT_EQUAL(3, first->id);

    // Last should be items[0] (first added)
    test_item_t *last = rs_list_last_entry(&list, test_item_t, link);
    TEST_ASSERT_EQUAL(1, last->id);
}

void test_list_add_tail(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[3] = {{.id = 1}, {.id = 2}, {.id = 3}};

    // Add at tail: list will be [1, 2, 3]
    rs_list_add_tail(&list, &items[0].link);
    rs_list_add_tail(&list, &items[1].link);
    rs_list_add_tail(&list, &items[2].link);

    // First should be items[0]
    test_item_t *first = rs_list_first_entry(&list, test_item_t, link);
    TEST_ASSERT_EQUAL(1, first->id);

    // Last should be items[2]
    test_item_t *last = rs_list_last_entry(&list, test_item_t, link);
    TEST_ASSERT_EQUAL(3, last->id);
}

void test_list_del(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[3] = {{.id = 1}, {.id = 2}, {.id = 3}};

    rs_list_add_tail(&list, &items[0].link);
    rs_list_add_tail(&list, &items[1].link);
    rs_list_add_tail(&list, &items[2].link);

    // Remove middle element
    rs_list_del(&items[1].link);

    // List should now be [1, 3]
    test_item_t *first = rs_list_first_entry(&list, test_item_t, link);
    test_item_t *last = rs_list_last_entry(&list, test_item_t, link);

    TEST_ASSERT_EQUAL(1, first->id);
    TEST_ASSERT_EQUAL(3, last->id);

    // Verify next/prev links
    TEST_ASSERT_EQUAL_PTR(&items[2].link, items[0].link.next);
    TEST_ASSERT_EQUAL_PTR(&items[0].link, items[2].link.prev);
}

void test_list_del_init(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t item = {.id = 1};
    rs_list_add(&list, &item.link);

    rs_list_del_init(&item.link);

    TEST_ASSERT_TRUE(rs_list_empty(&list));
    // Node should be reinitialized (points to itself)
    TEST_ASSERT_EQUAL_PTR(&item.link, item.link.next);
    TEST_ASSERT_EQUAL_PTR(&item.link, item.link.prev);
}

void test_list_replace(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t item1 = {.id = 1};
    test_item_t item2 = {.id = 2};
    test_item_t replacement = {.id = 99};

    rs_list_add_tail(&list, &item1.link);
    rs_list_add_tail(&list, &item2.link);

    // Replace item1 with replacement
    rs_list_replace(&item1.link, &replacement.link);

    // First should now be replacement
    test_item_t *first = rs_list_first_entry(&list, test_item_t, link);
    TEST_ASSERT_EQUAL(99, first->id);

    // Second should still be item2
    test_item_t *second = rs_list_next_entry(first, link);
    TEST_ASSERT_EQUAL(2, second->id);
}

// ============================================================================
// Query function tests
// ============================================================================

void test_list_empty(void)
{
    rs_list_t list;
    rs_list_init(&list);

    TEST_ASSERT_TRUE(rs_list_empty(&list));

    test_item_t item = {.id = 1};
    rs_list_add(&list, &item.link);

    TEST_ASSERT_FALSE(rs_list_empty(&list));

    rs_list_del(&item.link);

    TEST_ASSERT_TRUE(rs_list_empty(&list));
}

void test_list_is_first(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[3] = {{.id = 1}, {.id = 2}, {.id = 3}};

    rs_list_add_tail(&list, &items[0].link);
    rs_list_add_tail(&list, &items[1].link);
    rs_list_add_tail(&list, &items[2].link);

    TEST_ASSERT_TRUE(rs_list_is_first(&items[0].link, &list));
    TEST_ASSERT_FALSE(rs_list_is_first(&items[1].link, &list));
    TEST_ASSERT_FALSE(rs_list_is_first(&items[2].link, &list));
}

void test_list_is_last(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[3] = {{.id = 1}, {.id = 2}, {.id = 3}};

    rs_list_add_tail(&list, &items[0].link);
    rs_list_add_tail(&list, &items[1].link);
    rs_list_add_tail(&list, &items[2].link);

    TEST_ASSERT_FALSE(rs_list_is_last(&items[0].link, &list));
    TEST_ASSERT_FALSE(rs_list_is_last(&items[1].link, &list));
    TEST_ASSERT_TRUE(rs_list_is_last(&items[2].link, &list));
}

void test_list_is_singular(void)
{
    rs_list_t list;
    rs_list_init(&list);

    TEST_ASSERT_FALSE(rs_list_is_singular(&list)); // Empty

    test_item_t item1 = {.id = 1};
    rs_list_add(&list, &item1.link);

    TEST_ASSERT_TRUE(rs_list_is_singular(&list)); // One element

    test_item_t item2 = {.id = 2};
    rs_list_add(&list, &item2.link);

    TEST_ASSERT_FALSE(rs_list_is_singular(&list)); // Two elements
}

// ============================================================================
// Entry macro tests
// ============================================================================

void test_list_entry(void)
{
    test_item_t item = {.id = 42, .name = "test"};
    strcpy(item.name, "test");

    rs_list_t *node = &item.link;
    test_item_t *recovered = rs_list_entry(node, test_item_t, link);

    TEST_ASSERT_EQUAL_PTR(&item, recovered);
    TEST_ASSERT_EQUAL(42, recovered->id);
    TEST_ASSERT_EQUAL_STRING("test", recovered->name);
}

void test_list_first_entry(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[3] = {{.id = 1}, {.id = 2}, {.id = 3}};

    rs_list_add_tail(&list, &items[0].link);
    rs_list_add_tail(&list, &items[1].link);
    rs_list_add_tail(&list, &items[2].link);

    test_item_t *first = rs_list_first_entry(&list, test_item_t, link);
    TEST_ASSERT_EQUAL(1, first->id);
}

void test_list_last_entry(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[3] = {{.id = 1}, {.id = 2}, {.id = 3}};

    rs_list_add_tail(&list, &items[0].link);
    rs_list_add_tail(&list, &items[1].link);
    rs_list_add_tail(&list, &items[2].link);

    test_item_t *last = rs_list_last_entry(&list, test_item_t, link);
    TEST_ASSERT_EQUAL(3, last->id);
}

void test_list_first_entry_or_null(void)
{
    rs_list_t list;
    rs_list_init(&list);

    // Empty list should return NULL
    test_item_t *entry = rs_list_first_entry_or_null(&list, test_item_t, link);
    TEST_ASSERT_NULL(entry);

    // Non-empty list should return first entry
    test_item_t item = {.id = 42};
    rs_list_add(&list, &item.link);

    entry = rs_list_first_entry_or_null(&list, test_item_t, link);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL(42, entry->id);
}

void test_list_next_prev_entry(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[3] = {{.id = 1}, {.id = 2}, {.id = 3}};

    rs_list_add_tail(&list, &items[0].link);
    rs_list_add_tail(&list, &items[1].link);
    rs_list_add_tail(&list, &items[2].link);

    // Navigate forward
    test_item_t *current = rs_list_first_entry(&list, test_item_t, link);
    TEST_ASSERT_EQUAL(1, current->id);

    current = rs_list_next_entry(current, link);
    TEST_ASSERT_EQUAL(2, current->id);

    current = rs_list_next_entry(current, link);
    TEST_ASSERT_EQUAL(3, current->id);

    // Navigate backward
    current = rs_list_prev_entry(current, link);
    TEST_ASSERT_EQUAL(2, current->id);

    current = rs_list_prev_entry(current, link);
    TEST_ASSERT_EQUAL(1, current->id);
}

// ============================================================================
// Iteration tests
// ============================================================================

void test_list_foreach(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[5] = {{.id = 1}, {.id = 2}, {.id = 3}, {.id = 4}, {.id = 5}};

    for (int i = 0; i < 5; i++) {
        rs_list_add_tail(&list, &items[i].link);
    }

    // Iterate and count
    rs_list_t *pos;
    int count = 0;
    rs_list_foreach(pos, &list)
    {
        count++;
    }
    TEST_ASSERT_EQUAL(5, count);
}

void test_list_foreach_reverse(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[5] = {{.id = 1}, {.id = 2}, {.id = 3}, {.id = 4}, {.id = 5}};

    for (int i = 0; i < 5; i++) {
        rs_list_add_tail(&list, &items[i].link);
    }

    // Iterate in reverse and verify order
    rs_list_t *pos;
    int expected_id = 5;
    rs_list_foreach_reverse(pos, &list)
    {
        test_item_t *item = rs_list_entry(pos, test_item_t, link);
        TEST_ASSERT_EQUAL(expected_id, item->id);
        expected_id--;
    }
    TEST_ASSERT_EQUAL(0, expected_id);
}

void test_list_foreach_safe(void)
{
    rs_list_t list;
    rs_list_init(&list);

    // Allocate items on heap so we can free them
    test_item_t *items[5];
    for (int i = 0; i < 5; i++) {
        items[i] = malloc(sizeof(test_item_t));
        items[i]->id = i + 1;
        rs_list_add_tail(&list, &items[i]->link);
    }

    // Delete all items during iteration
    rs_list_t *pos, *tmp;
    int count = 0;
    rs_list_foreach_safe(pos, tmp, &list)
    {
        test_item_t *item = rs_list_entry(pos, test_item_t, link);
        rs_list_del(&item->link);
        free(item);
        count++;
    }

    TEST_ASSERT_EQUAL(5, count);
    TEST_ASSERT_TRUE(rs_list_empty(&list));
}

void test_list_foreach_entry(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[5] = {{.id = 1}, {.id = 2}, {.id = 3}, {.id = 4}, {.id = 5}};

    for (int i = 0; i < 5; i++) {
        rs_list_add_tail(&list, &items[i].link);
    }

    // Iterate and verify order
    test_item_t *pos;
    int expected_id = 1;
    rs_list_foreach_entry(pos, &list, link)
    {
        TEST_ASSERT_EQUAL(expected_id, pos->id);
        expected_id++;
    }
    TEST_ASSERT_EQUAL(6, expected_id);
}

void test_list_foreach_entry_reverse(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[5] = {{.id = 1}, {.id = 2}, {.id = 3}, {.id = 4}, {.id = 5}};

    for (int i = 0; i < 5; i++) {
        rs_list_add_tail(&list, &items[i].link);
    }

    // Iterate in reverse and verify order
    test_item_t *pos;
    int expected_id = 5;
    rs_list_foreach_entry_reverse(pos, &list, link)
    {
        TEST_ASSERT_EQUAL(expected_id, pos->id);
        expected_id--;
    }
    TEST_ASSERT_EQUAL(0, expected_id);
}

void test_list_foreach_entry_safe(void)
{
    rs_list_t list;
    rs_list_init(&list);

    // Allocate items on heap
    test_item_t *items[5];
    for (int i = 0; i < 5; i++) {
        items[i] = malloc(sizeof(test_item_t));
        items[i]->id = i + 1;
        rs_list_add_tail(&list, &items[i]->link);
    }

    // Delete even-numbered items during iteration
    test_item_t *pos, *tmp;
    rs_list_foreach_entry_safe(pos, tmp, &list, link)
    {
        if (pos->id % 2 == 0) {
            rs_list_del(&pos->link);
            free(pos);
        }
    }

    // Verify only odd items remain: [1, 3, 5]
    int remaining = 0;
    rs_list_foreach_entry(pos, &list, link)
    {
        TEST_ASSERT_TRUE(pos->id % 2 == 1);
        remaining++;
    }
    TEST_ASSERT_EQUAL(3, remaining);

    // Cleanup remaining items
    rs_list_foreach_entry_safe(pos, tmp, &list, link)
    {
        rs_list_del(&pos->link);
        free(pos);
    }
}

// ============================================================================
// Move operations tests
// ============================================================================

void test_list_move(void)
{
    rs_list_t list1, list2;
    rs_list_init(&list1);
    rs_list_init(&list2);

    test_item_t items[3] = {{.id = 1}, {.id = 2}, {.id = 3}};

    // Add all to list1
    rs_list_add_tail(&list1, &items[0].link);
    rs_list_add_tail(&list1, &items[1].link);
    rs_list_add_tail(&list1, &items[2].link);

    // Move item[1] to front of list2
    rs_list_move(&items[1].link, &list2);

    // list1 should be [1, 3]
    test_item_t *first = rs_list_first_entry(&list1, test_item_t, link);
    test_item_t *last = rs_list_last_entry(&list1, test_item_t, link);
    TEST_ASSERT_EQUAL(1, first->id);
    TEST_ASSERT_EQUAL(3, last->id);

    // list2 should be [2]
    TEST_ASSERT_TRUE(rs_list_is_singular(&list2));
    test_item_t *moved = rs_list_first_entry(&list2, test_item_t, link);
    TEST_ASSERT_EQUAL(2, moved->id);
}

void test_list_move_tail(void)
{
    rs_list_t list1, list2;
    rs_list_init(&list1);
    rs_list_init(&list2);

    test_item_t item1 = {.id = 1};
    test_item_t item2 = {.id = 2};

    rs_list_add(&list1, &item1.link);
    rs_list_add(&list2, &item2.link);

    // Move item1 to tail of list2
    rs_list_move_tail(&item1.link, &list2);

    TEST_ASSERT_TRUE(rs_list_empty(&list1));

    // list2 should be [2, 1]
    test_item_t *first = rs_list_first_entry(&list2, test_item_t, link);
    test_item_t *last = rs_list_last_entry(&list2, test_item_t, link);
    TEST_ASSERT_EQUAL(2, first->id);
    TEST_ASSERT_EQUAL(1, last->id);
}

// ============================================================================
// Splice operations tests
// ============================================================================

void test_list_splice(void)
{
    rs_list_t list1, list2;
    rs_list_init(&list1);
    rs_list_init(&list2);

    test_item_t items1[2] = {{.id = 1}, {.id = 2}};
    test_item_t items2[2] = {{.id = 3}, {.id = 4}};

    // list1: [1, 2]
    rs_list_add_tail(&list1, &items1[0].link);
    rs_list_add_tail(&list1, &items1[1].link);

    // list2: [3, 4]
    rs_list_add_tail(&list2, &items2[0].link);
    rs_list_add_tail(&list2, &items2[1].link);

    // Splice list2 at front of list1: [3, 4, 1, 2]
    rs_list_splice(&list2, &list1);

    // Verify order
    test_item_t *pos;
    int expected[] = {3, 4, 1, 2};
    int i = 0;
    rs_list_foreach_entry(pos, &list1, link)
    {
        TEST_ASSERT_EQUAL(expected[i], pos->id);
        i++;
    }
    TEST_ASSERT_EQUAL(4, i);
}

void test_list_splice_tail(void)
{
    rs_list_t list1, list2;
    rs_list_init(&list1);
    rs_list_init(&list2);

    test_item_t items1[2] = {{.id = 1}, {.id = 2}};
    test_item_t items2[2] = {{.id = 3}, {.id = 4}};

    // list1: [1, 2]
    rs_list_add_tail(&list1, &items1[0].link);
    rs_list_add_tail(&list1, &items1[1].link);

    // list2: [3, 4]
    rs_list_add_tail(&list2, &items2[0].link);
    rs_list_add_tail(&list2, &items2[1].link);

    // Splice list2 at tail of list1: [1, 2, 3, 4]
    rs_list_splice_tail(&list2, &list1);

    // Verify order
    test_item_t *pos;
    int expected[] = {1, 2, 3, 4};
    int i = 0;
    rs_list_foreach_entry(pos, &list1, link)
    {
        TEST_ASSERT_EQUAL(expected[i], pos->id);
        i++;
    }
    TEST_ASSERT_EQUAL(4, i);
}

void test_list_splice_init(void)
{
    rs_list_t list1, list2;
    rs_list_init(&list1);
    rs_list_init(&list2);

    test_item_t items[2] = {{.id = 1}, {.id = 2}};

    rs_list_add_tail(&list2, &items[0].link);
    rs_list_add_tail(&list2, &items[1].link);

    // Splice and reinitialize list2
    rs_list_splice_init(&list2, &list1);

    // list2 should be empty now
    TEST_ASSERT_TRUE(rs_list_empty(&list2));

    // list1 should have the items
    TEST_ASSERT_FALSE(rs_list_empty(&list1));
}

void test_list_splice_empty(void)
{
    rs_list_t list1, list2;
    rs_list_init(&list1);
    rs_list_init(&list2);

    test_item_t item = {.id = 1};
    rs_list_add(&list1, &item.link);

    // Splice empty list2 into list1 (should be no-op)
    rs_list_splice(&list2, &list1);

    // list1 should still have just item
    TEST_ASSERT_TRUE(rs_list_is_singular(&list1));
    test_item_t *first = rs_list_first_entry(&list1, test_item_t, link);
    TEST_ASSERT_EQUAL(1, first->id);
}

// ============================================================================
// Multi-list membership tests
// ============================================================================

void test_list_multi_membership(void)
{
    rs_list_t list_a, list_b;
    rs_list_init(&list_a);
    rs_list_init(&list_b);

    multi_list_item_t item = {.value = 42};

    // Add to both lists
    rs_list_add(&list_a, &item.list_a);
    rs_list_add(&list_b, &item.list_b);

    TEST_ASSERT_FALSE(rs_list_empty(&list_a));
    TEST_ASSERT_FALSE(rs_list_empty(&list_b));

    // Recover item from either list
    multi_list_item_t *from_a = rs_list_entry(list_a.next, multi_list_item_t, list_a);
    multi_list_item_t *from_b = rs_list_entry(list_b.next, multi_list_item_t, list_b);

    TEST_ASSERT_EQUAL_PTR(&item, from_a);
    TEST_ASSERT_EQUAL_PTR(&item, from_b);
    TEST_ASSERT_EQUAL(42, from_a->value);
    TEST_ASSERT_EQUAL(42, from_b->value);

    // Remove from list_a only
    rs_list_del(&item.list_a);

    TEST_ASSERT_TRUE(rs_list_empty(&list_a));
    TEST_ASSERT_FALSE(rs_list_empty(&list_b));
}

// ============================================================================
// Edge cases
// ============================================================================

void test_list_empty_operations(void)
{
    rs_list_t list;
    rs_list_init(&list);

    // Iterate empty list (should be no-op)
    rs_list_t *pos;
    int count = 0;
    rs_list_foreach(pos, &list)
    {
        count++;
    }
    TEST_ASSERT_EQUAL(0, count);

    // Splice empty into empty (should be no-op)
    rs_list_t empty_list;
    rs_list_init(&empty_list);
    rs_list_splice(&empty_list, &list);

    TEST_ASSERT_TRUE(rs_list_empty(&list));
}

void test_list_single_element_operations(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t item = {.id = 1};
    rs_list_add(&list, &item.link);

    // Single element checks
    TEST_ASSERT_TRUE(rs_list_is_singular(&list));
    TEST_ASSERT_TRUE(rs_list_is_first(&item.link, &list));
    TEST_ASSERT_TRUE(rs_list_is_last(&item.link, &list));

    // First and last should be the same
    test_item_t *first = rs_list_first_entry(&list, test_item_t, link);
    test_item_t *last = rs_list_last_entry(&list, test_item_t, link);
    TEST_ASSERT_EQUAL_PTR(first, last);

    // Delete the only element
    rs_list_del(&item.link);
    TEST_ASSERT_TRUE(rs_list_empty(&list));
}

void test_list_large_list(void)
{
    rs_list_t list;
    rs_list_init(&list);

    const int N = 1000;
    test_item_t *items = malloc(N * sizeof(test_item_t));

    // Add N items
    for (int i = 0; i < N; i++) {
        items[i].id = i;
        rs_list_add_tail(&list, &items[i].link);
    }

    // Verify count
    int count = 0;
    test_item_t *pos;
    rs_list_foreach_entry(pos, &list, link)
    {
        TEST_ASSERT_EQUAL(count, pos->id);
        count++;
    }
    TEST_ASSERT_EQUAL(N, count);

    // Verify reverse iteration
    count = N - 1;
    rs_list_foreach_entry_reverse(pos, &list, link)
    {
        TEST_ASSERT_EQUAL(count, pos->id);
        count--;
    }
    TEST_ASSERT_EQUAL(-1, count);

    free(items);
}

// ============================================================================
// Property-based testing helpers
// ============================================================================

// Simple xorshift PRNG for reproducible tests
static uint32_t prng_state = 12345;

static uint32_t prng_next(void)
{
    uint32_t x = prng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    prng_state = x;
    return x;
}

static void prng_seed(uint32_t seed)
{
    prng_state = seed ? seed : 1;
}

// Count items in list
static int list_count(rs_list_t *head)
{
    int count = 0;
    rs_list_t *pos;
    rs_list_foreach(pos, head)
    {
        count++;
    }
    return count;
}

// Verify list integrity (circular structure is valid)
static bool list_is_valid(rs_list_t *head)
{
    // Empty list check
    if (head->next == head && head->prev == head) {
        return true;
    }

    // Traverse forward and check back-links
    rs_list_t *pos = head->next;
    rs_list_t *prev = head;
    int count = 0;
    const int MAX_ITER = 100000; // Prevent infinite loop

    while (pos != head && count < MAX_ITER) {
        // Check back-link
        if (pos->prev != prev) {
            return false;
        }
        prev = pos;
        pos = pos->next;
        count++;
    }

    // Should have returned to head
    if (pos != head) {
        return false;
    }

    // Check head's prev points to last element
    if (head->prev != prev) {
        return false;
    }

    return true;
}

// ============================================================================
// Property-based tests
// ============================================================================

// Property: After N adds, list length == N
void test_property_add_length(void)
{
    prng_seed(42);

    for (int trial = 0; trial < 100; trial++) {
        rs_list_t list;
        rs_list_init(&list);

        int n = (prng_next() % 100) + 1; // 1 to 100 items
        test_item_t *items = malloc(n * sizeof(test_item_t));

        // Randomly choose add or add_tail
        for (int i = 0; i < n; i++) {
            items[i].id = i;
            if (prng_next() % 2) {
                rs_list_add(&list, &items[i].link);
            } else {
                rs_list_add_tail(&list, &items[i].link);
            }
        }

        TEST_ASSERT_EQUAL(n, list_count(&list));
        TEST_ASSERT_TRUE(list_is_valid(&list));

        free(items);
    }
}

// Property: After N adds and M removes (M <= N), length == N - M
void test_property_add_remove_length(void)
{
    prng_seed(123);

    for (int trial = 0; trial < 100; trial++) {
        rs_list_t list;
        rs_list_init(&list);

        int n = (prng_next() % 50) + 10; // 10 to 59 items
        int m = prng_next() % n;         // 0 to n-1 removals

        test_item_t *items = malloc(n * sizeof(test_item_t));

        // Add all items
        for (int i = 0; i < n; i++) {
            items[i].id = i;
            rs_list_add_tail(&list, &items[i].link);
        }

        // Remove m random items
        bool *removed = calloc(n, sizeof(bool));
        int removed_count = 0;
        while (removed_count < m) {
            int idx = prng_next() % n;
            if (!removed[idx]) {
                rs_list_del(&items[idx].link);
                removed[idx] = true;
                removed_count++;
            }
        }

        TEST_ASSERT_EQUAL(n - m, list_count(&list));
        TEST_ASSERT_TRUE(list_is_valid(&list));

        free(removed);
        free(items);
    }
}

// Property: add_tail preserves insertion order (FIFO)
void test_property_add_tail_order(void)
{
    prng_seed(456);

    for (int trial = 0; trial < 50; trial++) {
        rs_list_t list;
        rs_list_init(&list);

        int n = (prng_next() % 100) + 1;
        test_item_t *items = malloc(n * sizeof(test_item_t));

        // Add with sequential IDs
        for (int i = 0; i < n; i++) {
            items[i].id = i;
            rs_list_add_tail(&list, &items[i].link);
        }

        // Verify order
        int expected = 0;
        test_item_t *pos;
        rs_list_foreach_entry(pos, &list, link)
        {
            TEST_ASSERT_EQUAL(expected, pos->id);
            expected++;
        }
        TEST_ASSERT_EQUAL(n, expected);

        free(items);
    }
}

// Property: add (at front) reverses insertion order (LIFO)
void test_property_add_front_order(void)
{
    prng_seed(789);

    for (int trial = 0; trial < 50; trial++) {
        rs_list_t list;
        rs_list_init(&list);

        int n = (prng_next() % 100) + 1;
        test_item_t *items = malloc(n * sizeof(test_item_t));

        // Add with sequential IDs at front
        for (int i = 0; i < n; i++) {
            items[i].id = i;
            rs_list_add(&list, &items[i].link);
        }

        // Verify reverse order
        int expected = n - 1;
        test_item_t *pos;
        rs_list_foreach_entry(pos, &list, link)
        {
            TEST_ASSERT_EQUAL(expected, pos->id);
            expected--;
        }
        TEST_ASSERT_EQUAL(-1, expected);

        free(items);
    }
}

// Property: splice combines lists, total length preserved
void test_property_splice_length(void)
{
    prng_seed(321);

    for (int trial = 0; trial < 50; trial++) {
        rs_list_t list1, list2;
        rs_list_init(&list1);
        rs_list_init(&list2);

        int n1 = prng_next() % 50;
        int n2 = prng_next() % 50;

        test_item_t *items1 = malloc((n1 + 1) * sizeof(test_item_t));
        test_item_t *items2 = malloc((n2 + 1) * sizeof(test_item_t));

        for (int i = 0; i < n1; i++) {
            items1[i].id = i;
            rs_list_add_tail(&list1, &items1[i].link);
        }
        for (int i = 0; i < n2; i++) {
            items2[i].id = 1000 + i;
            rs_list_add_tail(&list2, &items2[i].link);
        }

        // Splice list2 into list1
        if (prng_next() % 2) {
            rs_list_splice_init(&list2, &list1);
        } else {
            rs_list_splice_tail_init(&list2, &list1);
        }

        TEST_ASSERT_EQUAL(n1 + n2, list_count(&list1));
        TEST_ASSERT_TRUE(rs_list_empty(&list2));
        TEST_ASSERT_TRUE(list_is_valid(&list1));
        TEST_ASSERT_TRUE(list_is_valid(&list2));

        free(items1);
        free(items2);
    }
}

// Property: move preserves total count across lists
void test_property_move_preserves_count(void)
{
    prng_seed(654);

    for (int trial = 0; trial < 50; trial++) {
        rs_list_t list1, list2;
        rs_list_init(&list1);
        rs_list_init(&list2);

        int n = (prng_next() % 50) + 10;
        test_item_t *items = malloc(n * sizeof(test_item_t));

        // Start with all in list1
        for (int i = 0; i < n; i++) {
            items[i].id = i;
            rs_list_add_tail(&list1, &items[i].link);
        }

        // Randomly move items between lists
        int moves = prng_next() % (n * 2);
        for (int i = 0; i < moves; i++) {
            int idx = prng_next() % n;
            if (prng_next() % 2) {
                rs_list_move(&items[idx].link, &list1);
            } else {
                rs_list_move_tail(&items[idx].link, &list2);
            }
        }

        // Total should still be n
        int total = list_count(&list1) + list_count(&list2);
        TEST_ASSERT_EQUAL(n, total);
        TEST_ASSERT_TRUE(list_is_valid(&list1));
        TEST_ASSERT_TRUE(list_is_valid(&list2));

        free(items);
    }
}

// Property: list always valid after random operations
void test_property_random_operations_valid(void)
{
    prng_seed(999);

    for (int trial = 0; trial < 20; trial++) {
        rs_list_t list;
        rs_list_init(&list);

        int max_items = 100;
        test_item_t *items = malloc(max_items * sizeof(test_item_t));
        bool *in_list = calloc(max_items, sizeof(bool));
        int current_count = 0;

        // Perform random operations
        int ops = 500;
        for (int op = 0; op < ops; op++) {
            int action = prng_next() % 4;

            switch (action) {
            case 0: // Add random item
                if (current_count < max_items) {
                    // Find an item not in list
                    for (int i = 0; i < max_items; i++) {
                        if (!in_list[i]) {
                            items[i].id = i;
                            if (prng_next() % 2) {
                                rs_list_add(&list, &items[i].link);
                            } else {
                                rs_list_add_tail(&list, &items[i].link);
                            }
                            in_list[i] = true;
                            current_count++;
                            break;
                        }
                    }
                }
                break;

            case 1: // Remove random item
                if (current_count > 0) {
                    // Find an item in list
                    for (int i = 0; i < max_items; i++) {
                        if (in_list[i]) {
                            rs_list_del(&items[i].link);
                            in_list[i] = false;
                            current_count--;
                            break;
                        }
                    }
                }
                break;

            case 2: // Replace if possible
                if (current_count > 0 && current_count < max_items) {
                    int old_idx = -1, new_idx = -1;
                    for (int i = 0; i < max_items; i++) {
                        if (in_list[i] && old_idx < 0)
                            old_idx = i;
                        if (!in_list[i] && new_idx < 0)
                            new_idx = i;
                        if (old_idx >= 0 && new_idx >= 0)
                            break;
                    }
                    if (old_idx >= 0 && new_idx >= 0) {
                        items[new_idx].id = new_idx;
                        rs_list_replace(&items[old_idx].link, &items[new_idx].link);
                        in_list[old_idx] = false;
                        in_list[new_idx] = true;
                    }
                }
                break;

            case 3: // Just verify
                break;
            }

            // Verify after each operation
            TEST_ASSERT_TRUE(list_is_valid(&list));
            TEST_ASSERT_EQUAL(current_count, list_count(&list));
        }

        free(in_list);
        free(items);
    }
}

// ============================================================================
// Sorting tests
// ============================================================================

// Comparison function for test_item_t by id (ascending)
static int cmp_by_id(const void *a, const void *b, void *user_data)
{
    RS_UNUSED(user_data);
    const test_item_t *ia = rs_list_entry(a, test_item_t, link);
    const test_item_t *ib = rs_list_entry(b, test_item_t, link);
    return ia->id - ib->id;
}

// Comparison function for test_item_t by id (descending)
static int cmp_by_id_desc(const void *a, const void *b, void *user_data)
{
    RS_UNUSED(user_data);
    const test_item_t *ia = rs_list_entry(a, test_item_t, link);
    const test_item_t *ib = rs_list_entry(b, test_item_t, link);
    return ib->id - ia->id;
}

void test_list_sort_empty(void)
{
    rs_list_t list;
    rs_list_init(&list);

    // Sort empty list (should be no-op)
    rs_list_sort(&list, cmp_by_id, NULL);

    TEST_ASSERT_TRUE(rs_list_empty(&list));
    TEST_ASSERT_TRUE(list_is_valid(&list));
}

void test_list_sort_single(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t item = {.id = 42};
    rs_list_add(&list, &item.link);

    // Sort single element (should be no-op)
    rs_list_sort(&list, cmp_by_id, NULL);

    TEST_ASSERT_TRUE(rs_list_is_singular(&list));
    TEST_ASSERT_TRUE(list_is_valid(&list));

    test_item_t *first = rs_list_first_entry(&list, test_item_t, link);
    TEST_ASSERT_EQUAL(42, first->id);
}

void test_list_sort_two_elements(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[2] = {{.id = 5}, {.id = 2}};
    rs_list_add_tail(&list, &items[0].link);
    rs_list_add_tail(&list, &items[1].link);

    // Sort: [5, 2] -> [2, 5]
    rs_list_sort(&list, cmp_by_id, NULL);

    TEST_ASSERT_TRUE(list_is_valid(&list));

    test_item_t *first = rs_list_first_entry(&list, test_item_t, link);
    test_item_t *last = rs_list_last_entry(&list, test_item_t, link);
    TEST_ASSERT_EQUAL(2, first->id);
    TEST_ASSERT_EQUAL(5, last->id);
}

void test_list_sort_already_sorted(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[5] = {{.id = 1}, {.id = 2}, {.id = 3}, {.id = 4}, {.id = 5}};
    for (int i = 0; i < 5; i++) {
        rs_list_add_tail(&list, &items[i].link);
    }

    rs_list_sort(&list, cmp_by_id, NULL);

    TEST_ASSERT_TRUE(list_is_valid(&list));

    // Verify order preserved
    test_item_t *pos;
    int expected = 1;
    rs_list_foreach_entry(pos, &list, link)
    {
        TEST_ASSERT_EQUAL(expected, pos->id);
        expected++;
    }
    TEST_ASSERT_EQUAL(6, expected);
}

void test_list_sort_reverse_sorted(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[5] = {{.id = 5}, {.id = 4}, {.id = 3}, {.id = 2}, {.id = 1}};
    for (int i = 0; i < 5; i++) {
        rs_list_add_tail(&list, &items[i].link);
    }

    rs_list_sort(&list, cmp_by_id, NULL);

    TEST_ASSERT_TRUE(list_is_valid(&list));

    // Verify sorted order
    test_item_t *pos;
    int expected = 1;
    rs_list_foreach_entry(pos, &list, link)
    {
        TEST_ASSERT_EQUAL(expected, pos->id);
        expected++;
    }
    TEST_ASSERT_EQUAL(6, expected);
}

void test_list_sort_random(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[10] = {{.id = 7}, {.id = 2}, {.id = 9}, {.id = 1}, {.id = 5},
                             {.id = 8}, {.id = 3}, {.id = 6}, {.id = 4}, {.id = 10}};

    for (int i = 0; i < 10; i++) {
        rs_list_add_tail(&list, &items[i].link);
    }

    rs_list_sort(&list, cmp_by_id, NULL);

    TEST_ASSERT_TRUE(list_is_valid(&list));

    // Verify sorted order
    test_item_t *pos;
    int expected = 1;
    rs_list_foreach_entry(pos, &list, link)
    {
        TEST_ASSERT_EQUAL(expected, pos->id);
        expected++;
    }
    TEST_ASSERT_EQUAL(11, expected);
}

void test_list_sort_descending(void)
{
    rs_list_t list;
    rs_list_init(&list);

    test_item_t items[5] = {{.id = 1}, {.id = 2}, {.id = 3}, {.id = 4}, {.id = 5}};
    for (int i = 0; i < 5; i++) {
        rs_list_add_tail(&list, &items[i].link);
    }

    // Sort descending
    rs_list_sort(&list, cmp_by_id_desc, NULL);

    TEST_ASSERT_TRUE(list_is_valid(&list));

    // Verify descending order
    test_item_t *pos;
    int expected = 5;
    rs_list_foreach_entry(pos, &list, link)
    {
        TEST_ASSERT_EQUAL(expected, pos->id);
        expected--;
    }
    TEST_ASSERT_EQUAL(0, expected);
}

// Test structure with extra field for stability testing
typedef struct {
    int key;
    int order; // Original insertion order
    rs_list_t link;
} stability_item_t;

static int cmp_by_key(const void *a, const void *b, void *user_data)
{
    RS_UNUSED(user_data);
    const stability_item_t *ia = rs_list_entry(a, stability_item_t, link);
    const stability_item_t *ib = rs_list_entry(b, stability_item_t, link);
    return ia->key - ib->key;
}

void test_list_sort_stability(void)
{
    rs_list_t list;
    rs_list_init(&list);

    // Create items with duplicate keys
    // Items with same key should maintain their relative order
    stability_item_t items[6] = {
        {.key = 2, .order = 0}, // First with key=2
        {.key = 1, .order = 1}, // First with key=1
        {.key = 2, .order = 2}, // Second with key=2
        {.key = 1, .order = 3}, // Second with key=1
        {.key = 3, .order = 4}, // Only with key=3
        {.key = 2, .order = 5}, // Third with key=2
    };

    for (int i = 0; i < 6; i++) {
        rs_list_add_tail(&list, &items[i].link);
    }

    rs_list_sort(&list, cmp_by_key, NULL);

    TEST_ASSERT_TRUE(list_is_valid(&list));

    // Expected order by key: 1, 1, 2, 2, 2, 3
    // Within each key group, original order should be preserved
    int expected_keys[] = {1, 1, 2, 2, 2, 3};
    int expected_orders[] = {1, 3, 0, 2, 5, 4};

    stability_item_t *pos;
    int i = 0;
    rs_list_foreach_entry(pos, &list, link)
    {
        TEST_ASSERT_EQUAL(expected_keys[i], pos->key);
        TEST_ASSERT_EQUAL(expected_orders[i], pos->order);
        i++;
    }
    TEST_ASSERT_EQUAL(6, i);
}

void test_list_sort_large(void)
{
    rs_list_t list;
    rs_list_init(&list);

    const int N = 1000;
    test_item_t *items = malloc(N * sizeof(test_item_t));

    // Create shuffled items
    prng_seed(12345);
    for (int i = 0; i < N; i++) {
        items[i].id = i;
    }
    // Fisher-Yates shuffle
    for (int i = N - 1; i > 0; i--) {
        int j = prng_next() % (i + 1);
        int tmp = items[i].id;
        items[i].id = items[j].id;
        items[j].id = tmp;
    }

    for (int i = 0; i < N; i++) {
        rs_list_add_tail(&list, &items[i].link);
    }

    rs_list_sort(&list, cmp_by_id, NULL);

    TEST_ASSERT_TRUE(list_is_valid(&list));

    // Verify sorted
    test_item_t *pos;
    int expected = 0;
    rs_list_foreach_entry(pos, &list, link)
    {
        TEST_ASSERT_EQUAL(expected, pos->id);
        expected++;
    }
    TEST_ASSERT_EQUAL(N, expected);

    free(items);
}

// Property: sorted output is always sorted
void test_property_sort_always_sorted(void)
{
    prng_seed(777);

    for (int trial = 0; trial < 50; trial++) {
        rs_list_t list;
        rs_list_init(&list);

        int n = (prng_next() % 100) + 1;
        test_item_t *items = malloc(n * sizeof(test_item_t));

        for (int i = 0; i < n; i++) {
            items[i].id = prng_next() % 1000; // Random values with possible duplicates
            rs_list_add_tail(&list, &items[i].link);
        }

        rs_list_sort(&list, cmp_by_id, NULL);

        TEST_ASSERT_TRUE(list_is_valid(&list));

        // Verify sorted
        int prev = -1;
        test_item_t *pos;
        rs_list_foreach_entry(pos, &list, link)
        {
            TEST_ASSERT_TRUE(pos->id >= prev);
            prev = pos->id;
        }

        free(items);
    }
}

// Property: sorted output has same elements as input
void test_property_sort_preserves_elements(void)
{
    prng_seed(888);

    for (int trial = 0; trial < 50; trial++) {
        rs_list_t list;
        rs_list_init(&list);

        int n = (prng_next() % 100) + 1;
        test_item_t *items = malloc(n * sizeof(test_item_t));
        int *original_ids = malloc(n * sizeof(int));

        for (int i = 0; i < n; i++) {
            items[i].id = prng_next() % 1000;
            original_ids[i] = items[i].id;
            rs_list_add_tail(&list, &items[i].link);
        }

        rs_list_sort(&list, cmp_by_id, NULL);

        // Sort original_ids for comparison
        for (int i = 0; i < n - 1; i++) {
            for (int j = i + 1; j < n; j++) {
                if (original_ids[i] > original_ids[j]) {
                    int tmp = original_ids[i];
                    original_ids[i] = original_ids[j];
                    original_ids[j] = tmp;
                }
            }
        }

        // Verify same elements
        test_item_t *pos;
        int i = 0;
        rs_list_foreach_entry(pos, &list, link)
        {
            TEST_ASSERT_EQUAL(original_ids[i], pos->id);
            i++;
        }
        TEST_ASSERT_EQUAL(n, i);

        free(original_ids);
        free(items);
    }
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // Initialization
    RUN_TEST(test_list_init);
    RUN_TEST(test_list_init_multiple);

    // Basic operations
    RUN_TEST(test_list_add_single);
    RUN_TEST(test_list_add_multiple);
    RUN_TEST(test_list_add_tail);
    RUN_TEST(test_list_del);
    RUN_TEST(test_list_del_init);
    RUN_TEST(test_list_replace);

    // Query functions
    RUN_TEST(test_list_empty);
    RUN_TEST(test_list_is_first);
    RUN_TEST(test_list_is_last);
    RUN_TEST(test_list_is_singular);

    // Entry macros
    RUN_TEST(test_list_entry);
    RUN_TEST(test_list_first_entry);
    RUN_TEST(test_list_last_entry);
    RUN_TEST(test_list_first_entry_or_null);
    RUN_TEST(test_list_next_prev_entry);

    // Iteration
    RUN_TEST(test_list_foreach);
    RUN_TEST(test_list_foreach_reverse);
    RUN_TEST(test_list_foreach_safe);
    RUN_TEST(test_list_foreach_entry);
    RUN_TEST(test_list_foreach_entry_reverse);
    RUN_TEST(test_list_foreach_entry_safe);

    // Move operations
    RUN_TEST(test_list_move);
    RUN_TEST(test_list_move_tail);

    // Splice operations
    RUN_TEST(test_list_splice);
    RUN_TEST(test_list_splice_tail);
    RUN_TEST(test_list_splice_init);
    RUN_TEST(test_list_splice_empty);

    // Multi-list membership
    RUN_TEST(test_list_multi_membership);

    // Edge cases
    RUN_TEST(test_list_empty_operations);
    RUN_TEST(test_list_single_element_operations);
    RUN_TEST(test_list_large_list);

    // Property-based tests
    RUN_TEST(test_property_add_length);
    RUN_TEST(test_property_add_remove_length);
    RUN_TEST(test_property_add_tail_order);
    RUN_TEST(test_property_add_front_order);
    RUN_TEST(test_property_splice_length);
    RUN_TEST(test_property_move_preserves_count);
    RUN_TEST(test_property_random_operations_valid);

    // Sorting tests
    RUN_TEST(test_list_sort_empty);
    RUN_TEST(test_list_sort_single);
    RUN_TEST(test_list_sort_two_elements);
    RUN_TEST(test_list_sort_already_sorted);
    RUN_TEST(test_list_sort_reverse_sorted);
    RUN_TEST(test_list_sort_random);
    RUN_TEST(test_list_sort_descending);
    RUN_TEST(test_list_sort_stability);
    RUN_TEST(test_list_sort_large);
    RUN_TEST(test_property_sort_always_sorted);
    RUN_TEST(test_property_sort_preserves_elements);

    return UNITY_END();
}
