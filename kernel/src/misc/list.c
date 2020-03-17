#include <kernel/list.h>

#include <stdlib.h>

list_node_t* list_node_new(void* data);

/* Returns an empty, `kfree`-able list.
 */
list_t* list_new() {
    list_t* list = (list_t*) kmalloc(sizeof(list_t));

    if (!list) {
        return NULL;
    }

    list->count = 0;
    list->root = NULL;

    return list;
}

/* Appends an element to the given list.
 */
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

/* Preprends an element to the given list.
 */
list_t* list_add_front(list_t* list, void* data) {
    list_node_t* node = list_node_new(data);

    if (!node) {
        return NULL;
    }

    if (!list->root) {
        list->root = node;
    } else {
        list->root->prev = node;
        node->next = list->root;
        list->root = node;
    }

    list->count++;

    return list;
}

/* Returns the `index`th element of the given list.
 */
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

/* Returns the index of an element in the list. If the element isn't in the
 * list, return the index one past the end of the list.
 */
uint32_t list_get_index_of(list_t* list, void* data) {
    list_node_t* node = list->root;

    for (uint32_t i = 0; i < list->count; i++) {
        if (node->data == data) {
            return i;
        }

        node = node->next;
    }

    return list->count;
}

/* Removes the specified element from the list and returns it.
 */
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

/* Allocates a node on the heap containing the given data.
 * Note: the node is uninitialized apart from its data.
 */
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