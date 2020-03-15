#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct _list_node_t {
    void* data;
    struct _list_node_t* next;
    struct _list_node_t* prev;
} list_node_t;

typedef struct _list_t {
    uint32_t count;
    list_node_t* root;
} list_t;

typedef bool (filter_t)(void*);

list_t* list_new();
list_t* list_add(list_t* list, void* data);
list_t* list_add_front(list_t* list, void* data);
void* list_get_at(list_t* list, uint32_t index);
void* list_get_with(list_t* list, filter_t filter, uint32_t* index);
uint32_t list_get_index_of(list_t* list, void* data);
void* list_remove_at(list_t* list, uint32_t index);

// Private
list_node_t* list_node_new(void* data);