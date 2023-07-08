#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct list_t {
    void* data;
    struct list_t* next;
    struct list_t* prev;
} list_t;

#define LIST_HEAD_INIT(name) (list_t) { NULL, &(name), &(name) }

#define list_entry(ptr, type) \
    ((type*) ptr->data)

#define list_first_entry(list, type) \
    ((type*) (list)->next->data)

#define list_last_entry(list, type) \
    ((type*) (list)->prev->data)

/* Iterate over the list by (iter, pos) pair, where iter is a list_t* and pos
 * has the list's data type.
 * Useful when the list needs to be edited during iteration.
 */
#define list_for_each(iter, pos, list) \
    for (iter = (list)->next, pos = (typeof(pos)) iter->data; \
        iter != (list); iter = iter->next, pos = (typeof(pos)) iter->data)

/* Iterate over the list by element pos where pos has the list's data type.
 */
#define list_for_each_entry(pos, list) \
    pos = (typeof(pos)) (list)->next->data; \
    for (list_t* __iter = (list)->next; \
        __iter != (list); __iter = __iter->next, pos = (typeof(pos)) __iter->data)

#define list_for_each_safe(iter, n, list) \
    for (iter = (list)->next, n = iter->next; iter != (list); \
        iter = n, n = iter->next)

#define list_for_each_entry_rev(iter, pos, list) \
    for (iter = (list)->prev, pos = (typeof(pos)) iter->data; \
        iter != (list); iter = iter->prev, pos = (typeof(pos)) iter->data)

bool list_empty(list_t* list);
uint32_t list_count(list_t* list);
void __list_add(list_t* new, list_t* prev, list_t* next);
list_t* list_add(list_t* list, void* data);
list_t* list_add_front(list_t* list, void* data);
list_t* list_add_inplace(list_t* list, list_t* new);
list_t* list_add_front_inplace(list_t* list, list_t* item);
void __list_del(list_t* prev, list_t* next);
void list_del(list_t* entry);
void list_del(list_t* entry);
void list_splice(list_t *list, list_t *head);
void list_move(list_t* list, list_t* head);
void* list_first(list_t* list);
void* list_last(list_t* list);