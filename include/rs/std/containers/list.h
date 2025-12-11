#pragma once

#include <rs/std/internal/api.h>
#include <rs/std/macros.h>
#include <rs/std/types.h>
#include <stdbool.h>

RS_EXTERN_C_BEGIN

/**
 * Intrusive doubly linked circular list.
 *
 * This is a high-performance linked list implementation following the
 * Linux kernel list.h design. The list is "intrusive" meaning:
 * - Users embed rs_list_t in their own structs
 * - The list does not allocate memory - users manage their own memory
 * - Use rs_list_entry() to get back to the containing struct
 *
 * The list is circular with a sentinel node:
 * - Empty: head->next == head && head->prev == head
 * - head->next points to first element
 * - head->prev points to last element
 *
 * Example:
 *   typedef struct {
 *       int id;
 *       char name[64];
 *       rs_list_t link;
 *   } my_item_t;
 *
 *   rs_list_t my_list;
 *   rs_list_init(&my_list);
 *
 *   my_item_t *item = malloc(sizeof(my_item_t));
 *   item->id = 42;
 *   rs_list_add(&my_list, &item->link);
 *
 *   // Iterate
 *   my_item_t *pos;
 *   rs_list_foreach_entry(pos, &my_list, link) {
 *       printf("id: %d\n", pos->id);
 *   }
 *
 *   // Cleanup
 *   rs_list_del(&item->link);
 *   free(item);
 */
typedef struct rs_list {
    struct rs_list *next;
    struct rs_list *prev;
} rs_list_t;

// ============================================================================
// Initialization
// ============================================================================

/**
 * Initialize a list head (sentinel).
 *
 * @param list Pointer to the list head to initialize
 */
static inline void rs_list_init(rs_list_t *list)
{
    list->next = list;
    list->prev = list;
}

// ============================================================================
// Basic Operations
// ============================================================================

/**
 * Internal: Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know the prev/next
 * entries already.
 */
static inline void __rs_list_add(rs_list_t *node, rs_list_t *prev, rs_list_t *next)
{
    next->prev = node;
    node->next = next;
    node->prev = prev;
    prev->next = node;
}

/**
 * Add a new entry after the specified head (insert at front).
 *
 * @param head List head to add after
 * @param node New entry to add
 *
 * Use this for stack-like (LIFO) behavior.
 */
static inline void rs_list_add(rs_list_t *head, rs_list_t *node)
{
    __rs_list_add(node, head, head->next);
}

/**
 * Add a new entry before the specified head (insert at back).
 *
 * @param head List head to add before
 * @param node New entry to add
 *
 * Use this for queue-like (FIFO) behavior.
 */
static inline void rs_list_add_tail(rs_list_t *head, rs_list_t *node)
{
    __rs_list_add(node, head->prev, head);
}

/**
 * Internal: Delete a list entry by making prev/next point to each other.
 */
static inline void __rs_list_del(rs_list_t *prev, rs_list_t *next)
{
    next->prev = prev;
    prev->next = next;
}

/**
 * Delete entry from list.
 *
 * @param node Entry to delete
 *
 * Note: The entry's prev/next pointers are left in an undefined state.
 * If you need to check if a node is in a list later, use rs_list_del_init().
 */
static inline void rs_list_del(rs_list_t *node)
{
    __rs_list_del(node->prev, node->next);
}

/**
 * Delete entry from list and reinitialize it.
 *
 * @param node Entry to delete
 *
 * After this call, the node points to itself (like an empty list head).
 * This allows checking if the node is in a list via rs_list_empty().
 */
static inline void rs_list_del_init(rs_list_t *node)
{
    __rs_list_del(node->prev, node->next);
    rs_list_init(node);
}

/**
 * Replace an old entry with a new one.
 *
 * @param old Entry to be replaced
 * @param node New entry to insert
 *
 * Note: The old entry's pointers are left in an undefined state.
 */
static inline void rs_list_replace(rs_list_t *old, rs_list_t *node)
{
    node->next = old->next;
    node->next->prev = node;
    node->prev = old->prev;
    node->prev->next = node;
}

/**
 * Replace an old entry with a new one and reinitialize the old.
 *
 * @param old Entry to be replaced
 * @param node New entry to insert
 */
static inline void rs_list_replace_init(rs_list_t *old, rs_list_t *node)
{
    rs_list_replace(old, node);
    rs_list_init(old);
}

// ============================================================================
// Query Functions
// ============================================================================

/**
 * Check if a list is empty.
 *
 * @param head List head to check
 * @return true if list is empty, false otherwise
 */
static inline bool rs_list_empty(const rs_list_t *head)
{
    return head->next == head;
}

/**
 * Check if a node is the first entry in a list.
 *
 * @param node Entry to check
 * @param head List head
 * @return true if node is first, false otherwise
 */
static inline bool rs_list_is_first(const rs_list_t *node, const rs_list_t *head)
{
    return node->prev == head;
}

/**
 * Check if a node is the last entry in a list.
 *
 * @param node Entry to check
 * @param head List head
 * @return true if node is last, false otherwise
 */
static inline bool rs_list_is_last(const rs_list_t *node, const rs_list_t *head)
{
    return node->next == head;
}

/**
 * Check if a list has exactly one element.
 *
 * @param head List head to check
 * @return true if list has exactly one element, false otherwise
 */
static inline bool rs_list_is_singular(const rs_list_t *head)
{
    return !rs_list_empty(head) && (head->next == head->prev);
}

// ============================================================================
// Move Operations
// ============================================================================

/**
 * Move a node from one list to the front of another.
 *
 * @param node Entry to move
 * @param head Destination list head
 */
static inline void rs_list_move(rs_list_t *node, rs_list_t *head)
{
    __rs_list_del(node->prev, node->next);
    rs_list_add(head, node);
}

/**
 * Move a node from one list to the back of another.
 *
 * @param node Entry to move
 * @param head Destination list head
 */
static inline void rs_list_move_tail(rs_list_t *node, rs_list_t *head)
{
    __rs_list_del(node->prev, node->next);
    rs_list_add_tail(head, node);
}

// ============================================================================
// Splice Operations
// ============================================================================

/**
 * Internal: Join two lists.
 */
static inline void __rs_list_splice(const rs_list_t *list, rs_list_t *prev, rs_list_t *next)
{
    rs_list_t *first = list->next;
    rs_list_t *last = list->prev;

    first->prev = prev;
    prev->next = first;

    last->next = next;
    next->prev = last;
}

/**
 * Join two lists at the front.
 *
 * @param list The list to splice (will be emptied conceptually)
 * @param head The place to add it in the destination list
 *
 * All entries from @list are added after @head.
 * Note: @list is NOT reinitialized - caller should do that if needed.
 */
static inline void rs_list_splice(rs_list_t *list, rs_list_t *head)
{
    if (!rs_list_empty(list)) {
        __rs_list_splice(list, head, head->next);
    }
}

/**
 * Join two lists at the back.
 *
 * @param list The list to splice (will be emptied conceptually)
 * @param head The place to add it in the destination list
 *
 * All entries from @list are added before @head (at the tail).
 * Note: @list is NOT reinitialized - caller should do that if needed.
 */
static inline void rs_list_splice_tail(rs_list_t *list, rs_list_t *head)
{
    if (!rs_list_empty(list)) {
        __rs_list_splice(list, head->prev, head);
    }
}

/**
 * Join two lists and reinitialize the source.
 *
 * @param list The list to splice (will be reinitialized)
 * @param head The place to add it in the destination list
 */
static inline void rs_list_splice_init(rs_list_t *list, rs_list_t *head)
{
    if (!rs_list_empty(list)) {
        __rs_list_splice(list, head, head->next);
        rs_list_init(list);
    }
}

/**
 * Join two lists at the back and reinitialize the source.
 *
 * @param list The list to splice (will be reinitialized)
 * @param head The place to add it in the destination list
 */
static inline void rs_list_splice_tail_init(rs_list_t *list, rs_list_t *head)
{
    if (!rs_list_empty(list)) {
        __rs_list_splice(list, head->prev, head);
        rs_list_init(list);
    }
}

// ============================================================================
// Sorting
// ============================================================================

/**
 * Sort a list using merge sort.
 *
 * @param head List head to sort
 * @param cmp Comparison function (compares rs_list_t* pointers)
 * @param user_data User context passed to comparison function
 *
 * Time: O(n log n), Space: O(1), Stable: Yes
 *
 * The comparison function receives pointers to rs_list_t nodes.
 * Use rs_list_entry() to get the containing struct for comparison.
 *
 * Example:
 *   int cmp_by_id(const void *a, const void *b, void *ctx) {
 *       const my_item_t *ia = rs_list_entry(a, my_item_t, link);
 *       const my_item_t *ib = rs_list_entry(b, my_item_t, link);
 *       return ia->id - ib->id;
 *   }
 *   rs_list_sort(&my_list, cmp_by_id, NULL);
 */
RS_STD_API void rs_list_sort(rs_list_t *head, rs_compare_fn cmp, void *user_data);

// ============================================================================
// Entry Macros
// ============================================================================

/**
 * Get the struct for this entry.
 *
 * @param ptr    Pointer to the rs_list_t member
 * @param type   Type of the struct containing the rs_list_t
 * @param member Name of the rs_list_t member within the struct
 */
#define rs_list_entry(ptr, type, member) RS_CONTAINER_OF(ptr, type, member)

/**
 * Get the first entry from a list.
 *
 * @param head   List head
 * @param type   Type of the struct containing the rs_list_t
 * @param member Name of the rs_list_t member within the struct
 *
 * Note: List must not be empty!
 */
#define rs_list_first_entry(head, type, member) rs_list_entry((head)->next, type, member)

/**
 * Get the last entry from a list.
 *
 * @param head   List head
 * @param type   Type of the struct containing the rs_list_t
 * @param member Name of the rs_list_t member within the struct
 *
 * Note: List must not be empty!
 */
#define rs_list_last_entry(head, type, member) rs_list_entry((head)->prev, type, member)

/**
 * Get the first entry from a list, or NULL if empty.
 *
 * @param head   List head
 * @param type   Type of the struct containing the rs_list_t
 * @param member Name of the rs_list_t member within the struct
 */
#define rs_list_first_entry_or_null(head, type, member)                                                                \
    (rs_list_empty(head) ? NULL : rs_list_first_entry(head, type, member))

/**
 * Get the next entry in the list.
 *
 * @param pos    Current entry pointer
 * @param member Name of the rs_list_t member within the struct
 */
#define rs_list_next_entry(pos, member) rs_list_entry((pos)->member.next, __typeof__(*(pos)), member)

/**
 * Get the previous entry in the list.
 *
 * @param pos    Current entry pointer
 * @param member Name of the rs_list_t member within the struct
 */
#define rs_list_prev_entry(pos, member) rs_list_entry((pos)->member.prev, __typeof__(*(pos)), member)

// ============================================================================
// Iteration Macros
// ============================================================================

/**
 * Iterate over list nodes.
 *
 * @param pos  Loop cursor (rs_list_t *)
 * @param head List head
 */
#define rs_list_foreach(pos, head) for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * Iterate over list nodes in reverse.
 *
 * @param pos  Loop cursor (rs_list_t *)
 * @param head List head
 */
#define rs_list_foreach_reverse(pos, head) for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * Iterate over list nodes (safe for deletion).
 *
 * @param pos  Loop cursor (rs_list_t *)
 * @param tmp  Temporary storage (rs_list_t *)
 * @param head List head
 */
#define rs_list_foreach_safe(pos, tmp, head)                                                                           \
    for (pos = (head)->next, tmp = pos->next; pos != (head); pos = tmp, tmp = pos->next)

/**
 * Iterate over list entries.
 *
 * @param pos    Loop cursor (pointer to containing struct)
 * @param head   List head
 * @param member Name of the rs_list_t member within the struct
 */
#define rs_list_foreach_entry(pos, head, member)                                                                       \
    for (pos = rs_list_first_entry(head, __typeof__(*pos), member); &pos->member != (head);                            \
         pos = rs_list_next_entry(pos, member))

/**
 * Iterate over list entries in reverse.
 *
 * @param pos    Loop cursor (pointer to containing struct)
 * @param head   List head
 * @param member Name of the rs_list_t member within the struct
 */
#define rs_list_foreach_entry_reverse(pos, head, member)                                                               \
    for (pos = rs_list_last_entry(head, __typeof__(*pos), member); &pos->member != (head);                             \
         pos = rs_list_prev_entry(pos, member))

/**
 * Iterate over list entries (safe for deletion).
 *
 * @param pos    Loop cursor (pointer to containing struct)
 * @param tmp    Temporary storage (same type as pos)
 * @param head   List head
 * @param member Name of the rs_list_t member within the struct
 */
#define rs_list_foreach_entry_safe(pos, tmp, head, member)                                                             \
    for (pos = rs_list_first_entry(head, __typeof__(*pos), member), tmp = rs_list_next_entry(pos, member);             \
         &pos->member != (head); pos = tmp, tmp = rs_list_next_entry(tmp, member))

/**
 * Iterate over list entries in reverse (safe for deletion).
 *
 * @param pos    Loop cursor (pointer to containing struct)
 * @param tmp    Temporary storage (same type as pos)
 * @param head   List head
 * @param member Name of the rs_list_t member within the struct
 */
#define rs_list_foreach_entry_safe_reverse(pos, tmp, head, member)                                                     \
    for (pos = rs_list_last_entry(head, __typeof__(*pos), member), tmp = rs_list_prev_entry(pos, member);              \
         &pos->member != (head); pos = tmp, tmp = rs_list_prev_entry(tmp, member))

RS_EXTERN_C_END
