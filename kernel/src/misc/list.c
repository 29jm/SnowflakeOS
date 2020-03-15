#include <kernel/list.h>
#include <stdlib.h>

list_t* list_new() {
    list_t* list = (list_t*) kmalloc(sizeof(list_t));

    if (!list) {
        return NULL;
    }

    list->count = 0;
    list->root = NULL;

    return list;
}

list_t* list_add(list_t* list, void* data) {
    list_node_t* node = list_node_new(data);

    if (!node) {
        return NULL;
    }

    if (!list->root) {
        list->root = node;
    } else {
        list_node_t* n = list->root;

        while (n->next) {
            n = n->next;
        }

        n->next = node;
        node->prev = n;
    }

    list->count++;

    return list;
}

void* list_get_at(list_t* list, uint32_t index) {
    if (index >= list->count) {
        return NULL;
    }

    list_node_t* node = list->root;

    for (uint32_t i = 0; i < index; i++) {
        node = node->next;
    }

    return node->data;
}

void* list_remove_at(list_t* list, uint32_t index) {
    if (index >= list->count) {
        return NULL;
    }

    list_node_t* node = list->root;

    for (uint32_t i = 0; i < index; i++) {
        node = node->next;
    }

    void* data = node->data;

    if (node->prev) {
        node->prev->next = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    }

    if (index == 0) {
        list->root = node->next;
    }

    kfree(node);
    list->count--;

    return data;
}

// Private
list_node_t* list_node_new(void* data) {
    list_node_t* node = (list_node_t*) kmalloc(sizeof(list_node_t));

    if (!node) {
        return NULL;
    }

    node->data = data;
    node->next = NULL;
    node->prev = NULL;

    return node;
}