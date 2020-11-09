#include <list.h>

#include <stdlib.h>
#include <stdbool.h>

/* Allocates a node on the heap containing the given data.
 * Note: the node is uninitialized apart from its data.
 */
list_t* list_node_new(void* data) {
    list_t* node = (list_t*) malloc(sizeof(list_t));

    if (!node) {
        return NULL;
    }

    node->data = data;
    node->next = NULL;
    node->prev = NULL;

    return node;
}

bool list_empty(list_t* list) {
    return list == list->next;
}

void __list_add(list_t* new, list_t* prev, list_t* next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

/* Appends an element to the given list.
 */
list_t* list_add(list_t* list, void* data) {
    list_t* new = list_node_new(data);

    if (!new) {
        return NULL;
    }

    __list_add(new, list->prev, list);

    return list;
}

/* Preprends an element to the given list.
 */
list_t* list_add_front(list_t* list, void* data) {
    list_t* new = list_node_new(data);

    if (!new) {
        return NULL;
    }

    __list_add(new, list, list->next);

    return list;
}

void __list_del(list_t* prev, list_t* next) {
    next->prev = prev;
    prev->next = next;
}

void list_del(list_t* entry) {
    __list_del(entry->prev, entry->next);
    entry->next = NULL; // Safety first, TODO: remove
    entry->prev = NULL;
    free(entry);
}

/**
 * list_splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
void list_splice(list_t* list, list_t* head) {
    list_t *first = list->next;

    if (first != list) {
        list_t *last = list->prev;
        list_t *at = head->next;
        first->prev = head;
        head->next = first;
        last->next = at;
        at->prev = last;
    }
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
void list_move(list_t* list, list_t* head) {
    list_add_front(head, list->data);
    list_del(list);
}

void* list_first(list_t* list) {
    return list->next;
}

void* list_last(list_t* list) {
    return list->prev;
}