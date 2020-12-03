#include <kernel/sched_robin.h>
#include <kernel/sys.h>
#include <kernel/timer.h>
#include <stdlib.h>

uint32_t flag;
/* Wraps a `process_t*` for round robin purposes.
 */
typedef struct _proc_node_t {
    process_t* process;
    struct _proc_node_t* next;
    uint32_t count;
} proc_node_t;

/* The round robin scheduler is simple and requires only a single circular list
 * containing candidate processes. By having a `sched_t` as the first member of
 * the struct, we allow casting `sched_robin_t*`s to `sched_t*`.
 */
typedef struct {
    sched_t sched;
    proc_node_t* processes1;
    proc_node_t* processes2;
    proc_node_t* processes3;
    uint32_t flag;
} sched_mlfq_t;

process_t* sched_mlfq_get_current(sched_t* sched) {
    sched_mlfq_t* sc = (sched_mlfq_t*) sched;


    if(flag==1){
        return sc->processes2->process;
    }
    else if (flag==2)
    {
        return sc->processes3->process;
    }
    else
    {
        return sc->processes1->process;
    }
    
    
}

void sched_mlfq_add(sched_t* sched, process_t* new_process) {
    sched_mlfq_t* sc = (sched_mlfq_t*) sched;
    proc_node_t* new = kmalloc(sizeof(proc_node_t));

    new->process = new_process;

    // Insert the proces in the ring, create it if empty
    if (!sc->processes1) {
        new->next = new;
        sc->processes1 = new;
        sc->processes1->count = 1; //count 1 for first process
    } else {
        proc_node_t* p = sc->processes1->next;
        sc->processes1->next = new;
        new->next = p;
        sc->processes1->count++; //increase count value as process add
    }
    
}
void sched_mlfq_add_priority(sched_t* sched,process_t* prio_process){
    sched_mlfq_t* sc = (sched_mlfq_t*) sched;
    proc_node_t* new = kmalloc(sizeof(proc_node_t));
    prio_process = sc->processes1->process;
    
    new->process = prio_process;
    if (flag==1) //adding process in circular list 2
    {
        
        if(!sc->processes2){
            new->next = new;
            sc->processes2 = new;
            sc->processes2->count = 1;
        }else{
            proc_node_t* p = sc->processes2->next;
            sc->processes2->next = new;
            new->next = p;
            sc->processes2->count++; //increase count value as process add
        }
    }
    else //adding process for circular list 3
    {
        if(!sc->processes3){
            new->next = new;
            sc->processes3 = new;
            sc->processes3->count = 1;
        }else{
            proc_node_t* p = sc->processes3->next;
            sc->processes3->next = new;
            new->next = p;
            sc->processes3->count++; //increase count value as process add
        }
    }
    
    

}

process_t* sched_mlfq_next(sched_t* sched) {
    sched_mlfq_t* sc = (sched_mlfq_t*) sched;

    proc_node_t* p = sc->processes1;

	// Avoid switching to a sleeping process if possible
	do {
		if (p->next->process->sleep_ticks > 0) {
			p->next->process->sleep_ticks--;
		} else {
			// We don't need to switch process
			if (p->next == sc->processes1) {
				return sc->processes1->process;
			}

			// We don't need to modify the process queue
			if (p->next == sc->processes1->next) {
				break;
			}

			// We insert the next process between the current one and the one
			// previously scheduled to be switched to.
			proc_node_t* previous = p;
			proc_node_t* next_proc = p->next;
			proc_node_t* moved = sc->processes1->next;

			previous->next = next_proc->next;
			next_proc->next = moved;
			sc->processes1->next = next_proc;

			break;
		}

		p = p->next;
	} while (p != sc->processes1);

    sc->processes1 = sc->processes1->next;

    return sc->processes1->process;
}

//next process for circular list 2
process_t* sched_mlfq_next2(sched_t* sched) { 
    sched_mlfq_t* sc = (sched_mlfq_t*) sched;

    proc_node_t* p = sc->processes2;

	// Avoid switching to a sleeping process if possible
	do {
		if (p->next->process->sleep_ticks > 0) {
			p->next->process->sleep_ticks--;
		} else {
			// We don't need to switch process
			if (p->next == sc->processes2) {
				return sc->processes2->process;
			}

			// We don't need to modify the process queue
			if (p->next == sc->processes2->next) {
				break;
			}

			// We insert the next process between the current one and the one
			// previously scheduled to be switched to.
			proc_node_t* previous = p;
			proc_node_t* next_proc = p->next;
			proc_node_t* moved = sc->processes2->next;

			previous->next = next_proc->next;
			next_proc->next = moved;
			sc->processes2->next = next_proc;

			break;
		}

		p = p->next;
	} while (p != sc->processes2);

    sc->processes2 = sc->processes2->next;

    return sc->processes2->process;
}

//next process for circular list 3
process_t* sched_mlfq_next3(sched_t* sched) { 
    sched_mlfq_t* sc = (sched_mlfq_t*) sched;

    proc_node_t* p = sc->processes3;

	// Avoid switching to a sleeping process if possible
	do {
		if (p->next->process->sleep_ticks > 0) {
			p->next->process->sleep_ticks--;
		} else {
			// We don't need to switch process
			if (p->next == sc->processes3) {
				return sc->processes3->process;
			}

			// We don't need to modify the process queue
			if (p->next == sc->processes3->next) {
				break;
			}

			// We insert the next process between the current one and the one
			// previously scheduled to be switched to.
			proc_node_t* previous = p;
			proc_node_t* next_proc = p->next;
			proc_node_t* moved = sc->processes3->next;

			previous->next = next_proc->next;
			next_proc->next = moved;
			sc->processes3->next = next_proc;

			break;
		}

		p = p->next;
	} while (p != sc->processes3);

    sc->processes3 = sc->processes3->next;

    return sc->processes3->process;
}

void sched_mlfq_exit(sched_t* sched, process_t* process) {
    sched_mlfq_t* sc = (sched_mlfq_t*) sched;
    proc_node_t* p = sc->processes1;
    proc_node_t* p2 = sc->processes2;
    proc_node_t* p3 = sc->processes3;

    
    if(flag==1){  //exit for processes in circular list 2
        while (p2->next->process != process) {
           p2 = p2->next;
        }

        proc_node_t* to_remove = p2->next;
        p2->next = p2->next->next;

        sc->processes1 = p2;

        kfree(to_remove);
    }
    else if(flag==2){    //exit for porcesses in circular list 3
        while (p3->next->process != process) {
           p3 = p3->next;
        }

        proc_node_t* to_remove = p3->next;
        p3->next = p3->next->next;

        sc->processes1 = p3;

        kfree(to_remove);
    }
    else{     //exit for processes in circular list 1
        while (p->next->process != process) {
           p = p->next;
        }

        proc_node_t* to_remove = p->next;
        p->next = p->next->next;

        sc->processes1 = p;

        kfree(to_remove);
    }
}

/* Allocates a mlfq with round robin scheduler.
 */
sched_t* sched_mlfq() {
    sched_mlfq_t* sched = kmalloc(sizeof(sched_mlfq_t));
    if (timer_get_tick() % 5 != 0) 
    {   
        for (size_t i = 0; i < sched->processes1->count; i++)
        {        
            sched->sched = (sched_t) 
            {
                .sched_get_current = sched_mlfq_get_current,
                .sched_add = sched_mlfq_add,
                .sched_next = sched_mlfq_next,
                .sched_exit = sched_mlfq_exit
            };
            sched->processes1 = NULL;
            if(sched_mlfq_get_current){
                flag=1;
                sched_mlfq_add_priority;
            }
        }
    }
    if (timer_get_tick() % 8 != 0) 
    {   
        for (size_t i = 0; i < sched->processes2->count; i++)
        {        
            sched->sched = (sched_t) 
            {
                .sched_get_current = sched_mlfq_get_current,
                .sched_add = sched_mlfq_add,
                .sched_next = sched_mlfq_next2,
                .sched_exit = sched_mlfq_exit
            };
            sched->processes1 = NULL;
            if(sched_mlfq_get_current){
                flag=2;
                sched_mlfq_add_priority;
            }
        }
    }
    if (timer_get_tick() % 12 != 0) 
    {   
        for (size_t i = 0; i < sched->processes3->count; i++)
        {        
            sched->sched = (sched_t) 
            {
                .sched_get_current = sched_mlfq_get_current,
                .sched_add = sched_mlfq_add,
                .sched_next = sched_mlfq_next3,
                .sched_exit = sched_mlfq_exit
            };
            sched->processes1 = NULL;
        }
    }
    
    return (sched_t*) sched;
}
