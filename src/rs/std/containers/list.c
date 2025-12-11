#include <rs/std/containers/list.h>

// ============================================================================
// Sorting (Bottom-up Merge Sort)
// ============================================================================

/**
 * Internal: Merge two sorted lists into one sorted list.
 *
 * @param a First sorted list (will be consumed)
 * @param b Second sorted list (will be consumed)
 * @param cmp Comparison function
 * @param user_data User context for comparison
 * @return Head of merged sorted list (linear, not circular)
 */
static rs_list_t *__rs_list_merge(rs_list_t *a, rs_list_t *b, rs_compare_fn cmp, void *user_data)
{
    rs_list_t dummy;
    rs_list_t *tail = &dummy;

    while (a && b) {
        if (cmp(a, b, user_data) <= 0) {
            tail->next = a;
            a->prev = tail;
            a = a->next;
        } else {
            tail->next = b;
            b->prev = tail;
            b = b->next;
        }
        tail = tail->next;
    }

    // Append remaining elements
    tail->next = a ? a : b;
    if (tail->next) {
        tail->next->prev = tail;
    }

    return dummy.next;
}

/**
 * Internal: Cut first n nodes from a linear list.
 *
 * @param head Start of the linear list
 * @param n Number of nodes to cut
 * @return Pointer to the rest of the list (or NULL if exhausted)
 *
 * After this call, head...head+n-1 form a list terminated by NULL.
 */
static rs_list_t *__rs_list_cut(rs_list_t *head, rs_size_t n)
{
    while (head && n > 1) {
        head = head->next;
        n--;
    }

    if (!head) {
        return NULL;
    }

    rs_list_t *rest = head->next;
    head->next = NULL;
    return rest;
}

void rs_list_sort(rs_list_t *head, rs_compare_fn cmp, void *user_data)
{
    // Empty or single element - already sorted
    if (rs_list_empty(head) || rs_list_is_singular(head)) {
        return;
    }

    // Convert circular list to linear list for sorting
    rs_list_t *first = head->next;
    rs_list_t *last = head->prev;

    // Break the circle
    first->prev = NULL;
    last->next = NULL;

    // Bottom-up merge sort
    rs_size_t list_size = 1;
    rs_size_t merge_count;

    do {
        merge_count = 0;
        rs_list_t *p = first;
        first = NULL;
        rs_list_t **tail = &first;

        while (p) {
            merge_count++;

            // Get first sublist of size list_size
            rs_list_t *a = p;
            rs_list_t *b = __rs_list_cut(p, list_size);

            // Get second sublist of size list_size
            p = b ? __rs_list_cut(b, list_size) : NULL;

            // Merge the two sublists
            rs_list_t *merged = __rs_list_merge(a, b, cmp, user_data);

            // Append to result
            *tail = merged;
            if (merged) {
                merged->prev = (tail == &first) ? NULL : RS_CONTAINER_OF(tail, rs_list_t, next);
            }

            // Find end of merged list
            while (*tail) {
                tail = &(*tail)->next;
            }
        }

        list_size *= 2;
    } while (merge_count > 1);

    // Restore circular structure
    head->next = first;
    if (first) {
        first->prev = head;

        // Find last element
        last = first;
        while (last->next) {
            last = last->next;
        }
        last->next = head;
        head->prev = last;
    } else {
        // List became empty (shouldn't happen)
        head->prev = head;
    }
}
