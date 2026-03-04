/**
 * @file linked_list.c
 * @brief Array-based linked list implementation
 *
 * This module provides O(1) allocation/deallocation using a static array
 * without dynamic memory allocation. Suitable for embedded systems.
 *
 * @author BACnet Stack Contributors
 * @copyright SPDX-License-Identifier: MIT
 */

#include "linked_list.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Get node pointer at given index (internal helper)
 */
static LINKED_LIST_NODE *get_node_by_idx(LINKED_LIST_CTX *ctx, int idx)
{
    if (idx < 0 || idx >= ctx->pool_size) {
        return NULL;
    }
    return (LINKED_LIST_NODE *)((uint8_t *)ctx->pool +
                                (idx * ctx->element_size));
}

/**
 * @brief Get index from pointer (internal helper)
 */
static int get_idx_by_ptr(LINKED_LIST_CTX *ctx, void *ptr)
{
    if (ptr == NULL || ptr < ctx->pool) {
        return -1;
    }
    int idx =
        (int)(((uint8_t *)ptr - (uint8_t *)ctx->pool) / ctx->element_size);
    if (idx < 0 || idx >= ctx->pool_size) {
        return -1;
    }
    return idx;
}

/**
 * @brief Initialize the linked list
 */
void linked_list_init(
    LINKED_LIST_CTX *ctx, void *pool, int pool_size, int element_size)
{
    ctx->pool = pool;
    ctx->pool_size = pool_size;
    ctx->element_size = element_size;
    ctx->active_head_idx = -1;
    ctx->free_head_idx = 0;
    ctx->active_node_cnt = 0;

    /* Build free list chain: 0 -> 1 -> 2 -> ... -> (pool_size-1) -> -1 */
    for (int i = 0; i < pool_size - 1; i++) {
        LINKED_LIST_NODE *node = get_node_by_idx(ctx, i);
        node->next_index = i + 1;
        node->prev_index = -1;
    }
    LINKED_LIST_NODE *last = get_node_by_idx(ctx, pool_size - 1);
    last->next_index = -1;
    last->prev_index = -1;
}

/**
 * @brief Allocate a slot from free list
 */
void *freelist_alloc(LINKED_LIST_CTX *ctx)
{
    int idx = ctx->free_head_idx;
    if (idx == -1) {
        return NULL;
    }

    LINKED_LIST_NODE *node = get_node_by_idx(ctx, idx);
    ctx->free_head_idx = node->next_index;

    return (void *)node;
}

/**
 * @brief Return a slot to free list
 */
void freelist_free(LINKED_LIST_CTX *ctx, void *ptr)
{
    int idx = get_idx_by_ptr(ctx, ptr);
    if (idx < 0) {
        return;
    }

    LINKED_LIST_NODE *node = (LINKED_LIST_NODE *)ptr;
    node->next_index = ctx->free_head_idx;
    node->prev_index = -1;
    ctx->free_head_idx = idx;
}

/**
 * @brief Add a node to active list (at head)
 */
void activelist_add(LINKED_LIST_CTX *ctx, void *ptr)
{
    int idx = get_idx_by_ptr(ctx, ptr);
    if (idx < 0) {
        return;
    }

    int old_head = ctx->active_head_idx;
    LINKED_LIST_NODE *node = (LINKED_LIST_NODE *)ptr;

    /* Set new node's links */
    node->next_index = old_head;
    node->prev_index = -1;

    /* Update old head's prev pointer */
    if (old_head != -1) {
        LINKED_LIST_NODE *old_head_node = get_node_by_idx(ctx, old_head);
        old_head_node->prev_index = idx;
    }

    /* Update head and count */
    ctx->active_head_idx = idx;
    ctx->active_node_cnt++;
}

/**
 * @brief Remove a node from active list
 */
void activelist_remove(LINKED_LIST_CTX *ctx, void *ptr)
{
    int idx = get_idx_by_ptr(ctx, ptr);
    if (idx < 0) {
        return;
    }

    LINKED_LIST_NODE *node = (LINKED_LIST_NODE *)ptr;
    int prev = node->prev_index;
    int next = node->next_index;

    /* Unlink: update previous node's next pointer */
    if (prev == -1) {
        /* Node is head */
        ctx->active_head_idx = next;
    } else {
        LINKED_LIST_NODE *prev_node = get_node_by_idx(ctx, prev);
        prev_node->next_index = next;
    }

    /* Unlink: update next node's prev pointer */
    if (next != -1) {
        LINKED_LIST_NODE *next_node = get_node_by_idx(ctx, next);
        next_node->prev_index = prev;
    }

    /* Clear removed node's links */
    node->next_index = -1;
    node->prev_index = -1;

    ctx->active_node_cnt--;
}

/**
 * @brief Remove a node from active list and return to free list
 */
void *activelist_remove_and_free(LINKED_LIST_CTX *ctx, void *ptr)
{
    int idx = get_idx_by_ptr(ctx, ptr);
    if (idx < 0) {
        return NULL;
    }

    LINKED_LIST_NODE *node = (LINKED_LIST_NODE *)ptr;
    int prev = node->prev_index;
    int next = node->next_index;

    /* Unlink: update previous node's next pointer */
    if (prev == -1) {
        /* Node is head */
        ctx->active_head_idx = next;
    } else {
        LINKED_LIST_NODE *prev_node = get_node_by_idx(ctx, prev);
        prev_node->next_index = next;
    }

    /* Unlink: update next node's prev pointer */
    if (next != -1) {
        LINKED_LIST_NODE *next_node = get_node_by_idx(ctx, next);
        next_node->prev_index = prev;
    }

    ctx->active_node_cnt--;

    /* Add to free list head */
    node->next_index = ctx->free_head_idx;
    node->prev_index = -1;
    ctx->free_head_idx = idx;

    /* Return next node for safe iteration */
    if (next != -1) {
        return get_node_by_idx(ctx, next);
    }
    return NULL;
}

/**
 * @brief Get first element of active list for iteration
 */
void *activelist_first(LINKED_LIST_CTX *ctx)
{
    if (ctx->active_head_idx == -1) {
        return NULL;
    }
    return get_node_by_idx(ctx, ctx->active_head_idx);
}

/**
 * @brief Get next element in active list for iteration
 */
void *activelist_next(LINKED_LIST_CTX *ctx, void *ptr)
{
    LINKED_LIST_NODE *node = (LINKED_LIST_NODE *)ptr;
    if (node->next_index == -1) {
        return NULL;
    }
    return get_node_by_idx(ctx, node->next_index);
}

/**
 * @brief Get first element of free list for iteration
 */
void *freelist_first(LINKED_LIST_CTX *ctx)
{
    if (ctx->free_head_idx == -1) {
        return NULL;
    }
    return get_node_by_idx(ctx, ctx->free_head_idx);
}

/**
 * @brief Get next element in free list for iteration
 */
void *freelist_next(LINKED_LIST_CTX *ctx, void *ptr)
{
    LINKED_LIST_NODE *node = (LINKED_LIST_NODE *)ptr;
    if (node->next_index == -1) {
        return NULL;
    }
    return get_node_by_idx(ctx, node->next_index);
}

/**
 * @brief Get number of free nodes
 */
int freelist_count(LINKED_LIST_CTX *ctx)
{
    return ctx->pool_size - ctx->active_node_cnt;
}

/**
 * @brief Get number of active nodes
 */
int activelist_count(LINKED_LIST_CTX *ctx)
{
    return ctx->active_node_cnt;
}

/**
 * @brief Check if list is full (no free slots)
 */
bool linked_list_is_full(LINKED_LIST_CTX *ctx)
{
    return ctx->free_head_idx == -1;
}

/**
 * @brief Check if active list is empty
 */
bool activelist_is_empty(LINKED_LIST_CTX *ctx)
{
    return ctx->active_head_idx == -1;
}

/**
 * @brief Convert pointer to index
 */
int linked_list_ptr_to_idx(LINKED_LIST_CTX *ctx, void *ptr)
{
    return get_idx_by_ptr(ctx, ptr);
}

/**
 * @brief Convert index to pointer
 */
void *linked_list_idx_to_ptr(LINKED_LIST_CTX *ctx, int idx)
{
    return get_node_by_idx(ctx, idx);
}
