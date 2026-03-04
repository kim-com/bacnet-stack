/**
 * @file linked_list.h
 * @brief Array-based linked list with Free List and Active List management
 *
 * This module provides O(1) allocation/deallocation using a static array
 * without dynamic memory allocation. Suitable for embedded systems.
 *
 * Features:
 * - Static array based (no malloc/free)
 * - O(1) allocation from free list
 * - O(1) return to free list
 * - O(1) add/remove from active list (doubly linked)
 * - O(n) iteration over active nodes only
 * - Pointer-based API for intuitive usage
 *
 * @author BACnet Stack Contributors
 * @copyright SPDX-License-Identifier: MIT
 */

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdbool.h>

/**
 * @brief Node structure for linked list management
 *
 * This structure must be the first member of user's data structure
 * to allow casting between user structure and LINKED_LIST_NODE.
 *
 * Example:
 * @code
 * typedef struct {
 *     LINKED_LIST_NODE node;  // must be first!
 *     int my_data;
 *     char my_name[32];
 * } MY_ENTRY;
 * @endcode
 */
typedef struct {
    int next_index; /**< Next node index (-1 if end) */
    int prev_index; /**< Previous node index (-1 if head or free) */
} LINKED_LIST_NODE;

/**
 * @brief Linked list context structure
 *
 * Holds the state of the list including pointers to both
 * the free list and active list.
 */
typedef struct {
    void *pool; /**< Pointer to external array (as void* for stride-based
                   access) */
    int pool_size; /**< Number of elements in the array */
    int element_size; /**< Size of each element in bytes */
    int active_head_idx; /**< Active List HEAD (-1 if empty) */
    int free_head_idx; /**< Free List HEAD (-1 if full) */
    int active_node_cnt; /**< Number of active nodes */
} LINKED_LIST_CTX;

/**
 * @brief Initialize the linked list
 *
 * Sets up the list context and builds the free list chain.
 * After initialization:
 * - Active list is empty (active_head_idx = -1)
 * - Free list contains all nodes (0 -> 1 -> 2 -> ... -> -1)
 * - active_node_cnt = 0
 *
 * @param ctx List context to initialize
 * @param pool Pointer to external array
 * @param pool_size Number of elements in the array
 * @param element_size Size of each element in bytes (use sizeof(YOUR_STRUCT))
 */
void linked_list_init(
    LINKED_LIST_CTX *ctx, void *pool, int pool_size, int element_size);

/**
 * @brief Allocate a slot from free list
 *
 * Removes a node from the head of free list and returns pointer to it.
 * The node is NOT added to active list automatically.
 *
 * @param ctx List context
 * @return Pointer to allocated slot, or NULL if no free slot available
 */
void *freelist_alloc(LINKED_LIST_CTX *ctx);

/**
 * @brief Return a slot to free list
 *
 * Adds a node to the head of free list.
 * The node should be removed from active list before calling this.
 *
 * @param ctx List context
 * @param ptr Pointer to slot to return
 */
void freelist_free(LINKED_LIST_CTX *ctx, void *ptr);

/**
 * @brief Add a node to active list (at head)
 *
 * Inserts a node at the head of active list.
 * The node should be allocated from free list before calling this.
 *
 * @param ctx List context
 * @param ptr Pointer to node to add
 */
void activelist_add(LINKED_LIST_CTX *ctx, void *ptr);

/**
 * @brief Remove a node from active list
 *
 * Removes a node from active list by unlinking it.
 *
 * @note Does NOT return to free list automatically.
 *       Call freelist_free() separately after this.
 *
 * @param ctx List context
 * @param ptr Pointer to node to remove
 */
void activelist_remove(LINKED_LIST_CTX *ctx, void *ptr);

/**
 * @brief Remove a node from active list and return to free list
 *
 * Combines activelist_remove() and freelist_free() in one operation.
 * Returns the next node pointer for safe iteration during removal.
 *
 * @param ctx List context
 * @param ptr Pointer to node to remove
 * @return Pointer to next node in active list, or NULL if end of list
 */
void *activelist_remove_and_free(LINKED_LIST_CTX *ctx, void *ptr);

/**
 * @brief Get first element of active list for iteration
 *
 * @param ctx List context
 * @return Pointer to first element, or NULL if active list is empty
 */
void *activelist_first(LINKED_LIST_CTX *ctx);

/**
 * @brief Get next element in active list for iteration
 *
 * @param ctx List context
 * @param ptr Pointer to current element
 * @return Pointer to next element, or NULL if end of list
 */
void *activelist_next(LINKED_LIST_CTX *ctx, void *ptr);

/**
 * @brief Get first element of free list for iteration
 *
 * @param ctx List context
 * @return Pointer to first element, or NULL if free list is empty
 */
void *freelist_first(LINKED_LIST_CTX *ctx);

/**
 * @brief Get next element in free list for iteration
 *
 * @param ctx List context
 * @param ptr Pointer to current element
 * @return Pointer to next element, or NULL if end of list
 */
void *freelist_next(LINKED_LIST_CTX *ctx, void *ptr);

/**
 * @brief Get number of free nodes
 *
 * @param ctx List context
 * @return Number of nodes in free list
 */
int freelist_count(LINKED_LIST_CTX *ctx);

/**
 * @brief Get number of active nodes
 *
 * @param ctx List context
 * @return Number of nodes in active list
 */
int activelist_count(LINKED_LIST_CTX *ctx);

/**
 * @brief Check if list is full (no free slots)
 *
 * @param ctx List context
 * @return true if no free slots available, false otherwise
 */
bool linked_list_is_full(LINKED_LIST_CTX *ctx);

/**
 * @brief Check if active list is empty
 *
 * @param ctx List context
 * @return true if active list is empty, false otherwise
 */
bool activelist_is_empty(LINKED_LIST_CTX *ctx);

/**
 * @brief Convert pointer to index
 *
 * @param ctx List context
 * @param ptr Pointer to element
 * @return Index of element, or -1 if invalid pointer
 */
int linked_list_ptr_to_idx(LINKED_LIST_CTX *ctx, void *ptr);

/**
 * @brief Convert index to pointer
 *
 * @param ctx List context
 * @param idx Index of element
 * @return Pointer to element, or NULL if invalid index
 */
void *linked_list_idx_to_ptr(LINKED_LIST_CTX *ctx, int idx);

#endif /* LINKED_LIST_H */
