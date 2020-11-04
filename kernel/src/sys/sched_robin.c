#include <kernel/sched_robin.h>

#include <stdlib.h>
#include <stdio.h>

typedef struct _proc_node_t {
    process_t* process;
    struct _proc_node_t* next;
} proc_node_t;

typedef struct {
    sched_t sched;
    proc_node_t* processes;
} sched_robin_t;

process_t* sched_robin_get_current(sched_t* sched) {
    sched_robin_t* sc = (sched_robin_t*) sched;

    return sc->processes->process;
}

void sched_robin_add(sched_t* sched, process_t* new_process) {
    sched_robin_t* sc = (sched_robin_t*) sched;
    proc_node_t* new = kmalloc(sizeof(proc_node_t));

    new->process = new_process;

    // Insert the process in the ring, create it if empty
    if (!sc->processes) {
        new->next = new;
        sc->processes = new;
    } else {
        proc_node_t* p = sc->processes->next;
        sc->processes->next = new;
        new->next = p;
    }
}

process_t* sched_robin_next(sched_t* sched) {
    sched_robin_t* sc = (sched_robin_t*) sched;

    // TODO: allow sleeping again
    sc->processes = sc->processes->next;

    return sc->processes->process;
}

/* Make sure that when exiting the current process,
 */
void sched_robin_exit(sched_t* sched, process_t* process) {
    sched_robin_t* sc = (sched_robin_t*) sched;
    proc_node_t* p = sc->processes;

    while (p->next->process != process) {
        p = p->next;
    }

    proc_node_t* to_remove = p->next;
    p->next = p->next->next;

    sc->processes = p;

    kfree(to_remove);
}

sched_t* sched_robin() {
    sched_robin_t* sched = kmalloc(sizeof(sched_robin_t));

    sched->sched = (sched_t) {
        .sched_get_current = sched_robin_get_current,
        .sched_add = sched_robin_add,
        .sched_next = sched_robin_next,
        .sched_exit = sched_robin_exit
    };

    sched->processes = NULL;

    return (sched_t*) sched;
}