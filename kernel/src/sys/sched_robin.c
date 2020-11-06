#include <kernel/sched_robin.h>
#include <kernel/sys.h>

#include <stdlib.h>

/* Wraps a `process_t*` for round robin purposes.
 */
typedef struct _proc_node_t {
    process_t* process;
    struct _proc_node_t* next;
} proc_node_t;

/* The round robin scheduler is simple and requires only a single circular list
 * containing candidate processes. By having a `sched_t` as the first member of
 * the struct, we allow casting `sched_robin_t*`s to `sched_t*`.
 */
typedef struct {
    sched_t sched;
    proc_node_t* processes;
} sched_robin_t;

process_t* sched_robin_get_current(sched_t* sched) {
    sched_robin_t* sc = (sched_robin_t*) sched;

    if (sc->processes) {
        return sc->processes->process;
    }

    return NULL;
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
    proc_node_t* p = sc->processes;

    // Avoid switching to a sleeping process if possible
    do {
        if (p->next->process->sleep_ticks > 0) {
            p->next->process->sleep_ticks--;
        } else {
            // We don't need to switch process
            if (p->next == sc->processes) {
                return sc->processes->process;
            }

            // We don't need to modify the process queue
            if (p->next == sc->processes->next) {
                break;
            }

            // We insert the next process between the current one and the one
            // previously scheduled to be switched to.
            proc_node_t* previous = p;
            proc_node_t* next_proc = p->next;
            proc_node_t* moved = sc->processes->next;

            previous->next = next_proc->next;
            next_proc->next = moved;
            sc->processes->next = next_proc;

            break;
        }

        p = p->next;
    } while (p != sc->processes);

    sc->processes = sc->processes->next;

    return sc->processes->process;
}

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

/* Allocates a round robin scheduler.
 */
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