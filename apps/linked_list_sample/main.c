/**
 * @file main.c
 * @brief Sample application demonstrating the array-based linked list library
 * @author BACnet Stack Contributors
 * @copyright SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "bacnet/basic/sys/linked_list.h"

#define MAX_SUBSCRIPTIONS 10

/**
 * @brief User data structure
 * @note LINKED_LIST_NODE must be the first member for proper casting
 */
typedef struct {
    LINKED_LIST_NODE node; /* must be first member */
    int subscriber_id;
    char name[32];
    bool is_confirmed;
} SUBSCRIPTION;

/* Static array and context */
static SUBSCRIPTION subscriptions[MAX_SUBSCRIPTIONS];
static LINKED_LIST_CTX ctx;

/**
 * @brief Initialize subscription list
 */
void subscription_init(void)
{
    linked_list_init(
        &ctx, subscriptions, MAX_SUBSCRIPTIONS, sizeof(SUBSCRIPTION));
}

/**
 * @brief Add new subscription
 * @param subscriber_id Subscriber identifier
 * @param name Subscriber name
 * @param is_confirmed Whether subscription is confirmed
 * @return Pointer to added subscription, or NULL on failure
 */
SUBSCRIPTION *
subscription_add(int subscriber_id, const char *name, bool is_confirmed)
{
    /* Step 1: Allocate from free list */
    SUBSCRIPTION *sub = freelist_alloc(&ctx);
    if (sub == NULL) {
        printf("Error: No free slot available\n");
        return NULL;
    }

    /* Step 2: Set data (directly using the pointer!) */
    sub->subscriber_id = subscriber_id;
    strncpy(sub->name, name, sizeof(sub->name) - 1);
    sub->name[sizeof(sub->name) - 1] = '\0';
    sub->is_confirmed = is_confirmed;

    /* Step 3: Add to active list */
    activelist_add(&ctx, sub);

    int idx = linked_list_ptr_to_idx(&ctx, sub);
    printf("Added: [%d] id=%d, name=%s\n", idx, subscriber_id, name);
    return sub;
}

/**
 * @brief Remove subscription by pointer
 * @param sub Pointer to subscription to remove
 */
void subscription_remove(SUBSCRIPTION *sub)
{
    int idx = linked_list_ptr_to_idx(&ctx, sub);
    printf(
        "Removing: [%d] id=%d, name=%s\n", idx, sub->subscriber_id, sub->name);

    /* Remove from active list and return to free list */
    (void)activelist_remove_and_free(&ctx, sub);
}

/**
 * @brief Find subscription by subscriber_id
 * @param subscriber_id Subscriber identifier to search for
 * @return Pointer to found subscription, or NULL if not found
 */
SUBSCRIPTION *subscription_find(int subscriber_id)
{
    for (SUBSCRIPTION *sub = activelist_first(&ctx); sub != NULL;
         sub = activelist_next(&ctx, sub)) {
        if (sub->subscriber_id == subscriber_id) {
            return sub;
        }
    }
    return NULL; /* not found */
}

/**
 * @brief Print active list chain
 */
void print_active_list(void)
{
    printf("  Active List (count: %d): ", activelist_count(&ctx));
    if (activelist_is_empty(&ctx)) {
        printf("(empty)");
    } else {
        int first = 1;
        for (SUBSCRIPTION *sub = activelist_first(&ctx); sub != NULL;
             sub = activelist_next(&ctx, sub)) {
            if (!first) {
                printf(" -> ");
            }
            printf("[%d]", linked_list_ptr_to_idx(&ctx, sub));
            first = 0;
        }
    }
    printf("\n");
}

/**
 * @brief Print free list chain
 */
void print_free_list(void)
{
    printf("  Free List   (count: %d): ", freelist_count(&ctx));
    if (linked_list_is_full(&ctx)) {
        printf("(empty)");
    } else {
        int first = 1;
        for (SUBSCRIPTION *sub = freelist_first(&ctx); sub != NULL;
             sub = freelist_next(&ctx, sub)) {
            if (!first) {
                printf(" -> ");
            }
            printf("[%d]", linked_list_ptr_to_idx(&ctx, sub));
            first = 0;
        }
    }
    printf("\n");
}

/**
 * @brief Print both lists
 */
void print_lists(void)
{
    printf("\n");
    print_active_list();
    print_free_list();
    printf("\n");
}

/**
 * @brief Print all active subscriptions with details
 */
void subscription_print_all(void)
{
    printf(
        "\n=== Active Subscriptions (count: %d) ===\n", activelist_count(&ctx));
    for (SUBSCRIPTION *sub = activelist_first(&ctx); sub != NULL;
         sub = activelist_next(&ctx, sub)) {
        printf(
            "  [%d] id=%d, name=%s, confirmed=%s\n",
            linked_list_ptr_to_idx(&ctx, sub), sub->subscriber_id, sub->name,
            sub->is_confirmed ? "yes" : "no");
    }
    print_active_list();
    print_free_list();
    printf("=========================================\n\n");
}

/**
 * @brief Main entry point
 */
int main(void)
{
    printf("Linked List Library Sample (Pointer-based API)\n");
    printf("==============================================\n\n");

    /* Initialize */
    subscription_init();
    printf("Linked list initialized (size: %d)\n", MAX_SUBSCRIPTIONS);
    print_lists();

    /* Add subscriptions */
    printf("--- Adding 3 subscriptions ---\n");
    subscription_add(100, "Alice", true);
    subscription_add(200, "Bob", false);
    subscription_add(300, "Charlie", true);

    subscription_print_all();

    /* Find and remove Bob */
    printf("--- Finding and removing Bob (id=200) ---\n");
    SUBSCRIPTION *bob = subscription_find(200);
    if (bob != NULL) {
        subscription_remove(bob);
    }

    subscription_print_all();

    /* Add more subscriptions */
    printf("--- Adding 2 more subscriptions ---\n");
    subscription_add(400, "David", false);
    subscription_add(500, "Eve", true);

    subscription_print_all();

    /* Show status */
    printf("--- Status ---\n");
    printf("Is full: %s\n", linked_list_is_full(&ctx) ? "yes" : "no");
    printf("Is empty: %s\n", activelist_is_empty(&ctx) ? "yes" : "no");
    printf("Active count: %d\n\n", activelist_count(&ctx));

    /* Remove all subscriptions */
    printf("--- Removing all subscriptions ---\n");
    SUBSCRIPTION *sub = activelist_first(&ctx);
    while (sub != NULL) {
        int idx = linked_list_ptr_to_idx(&ctx, sub);
        printf("Removing: [%d] id=%d, name=%s\n", idx, sub->subscriber_id, sub->name);
        sub = activelist_remove_and_free(&ctx, sub);
    }

    subscription_print_all();

    printf("--- Final Status ---\n");
    printf("Is full: %s\n", linked_list_is_full(&ctx) ? "yes" : "no");
    printf("Is empty: %s\n", activelist_is_empty(&ctx) ? "yes" : "no");
    printf("Active count: %d\n", activelist_count(&ctx));

    return 0;
}
