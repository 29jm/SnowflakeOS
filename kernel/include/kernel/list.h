#pragma once

#include <stdint.h>

typedef struct _list_node_t {
    void* data;
    struct _list_node_t* next;
    struct _list_node_t* prev;
} list_node_t;

typedef struct _list_t {
    uint32_t count;
    list_node_t* root;
} list_t;

list_t* list_new();
list_t* list_add(list_t* list, void* data);
list_t* list_add_front(list_t* list, void* data);
void* list_get_at(list_t* list, uint32_t index);
uint32_t list_get_index_of(list_t* list, void* data);
void* list_remove_at(list_t* list, uint32_t index);