// #include "user/uthread.h"
#include "kernel/types.h"
#include "uthread.h"
#include "user.h"

#define NULL 0

extern void uswtch(struct context*, struct context*);
struct uthread process_threads[MAX_UTHREADS+1];
struct uthread *currect_thread = &process_threads[MAX_UTHREADS];
int start = 0;

int uthread_create(void (*start_func)(), enum sched_priority priority){
    struct uthread *start;
    for (start = process_threads; start < &process_threads[MAX_UTHREADS]; start++)
    {
        if (start ->state == FREE)
        {
            start ->priority = priority;
            start -> state = RUNNABLE;
            
            // Set up the new thread's stack with a new function frame
            start->context.sp = (uint64)start->ustack + STACK_SIZE - sizeof(uint64);
            start->context.ra = (uint64)start_func;
            start->context.s0 = 0;
            start->context.s1 = 0;
            start->context.s2 = 0;
            start->context.s3 = 0;
            start->context.s4 = 0;
            start->context.s5 = 0;
            start->context.s6 = 0;
            start->context.s7 = 0;
            if (currect_thread == NULL) {
                currect_thread = start;
            }
            return 0;
        }
    }
    return -1;
}
void uthread_yield(){

    struct uthread *next_thread = NULL;
    enum sched_priority highest_priority = LOW;
    struct uthread *t;

    for (t = process_threads ; t < &process_threads[MAX_UTHREADS]; t++)
    {
        if (t->state == RUNNABLE && t->priority >= highest_priority) {
            next_thread = t;
            highest_priority = t->priority;
        }
    }

    if (next_thread != NULL && next_thread != currect_thread) {
    struct uthread *tmp = currect_thread;
    next_thread->state = RUNNING;
    currect_thread = next_thread;
    uswtch(&tmp->context , &next_thread->context);
} else {
    // No other threads are runnable, continue executing the current thread
    return;
}
    
}
void uthread_exit(){
    int left_to_run = 0;
    currect_thread -> state = FREE;
    struct uthread *t;
    for (t = process_threads; t < &process_threads[MAX_UTHREADS]; t++)
    {  
        if(t != currect_thread && t -> state != FREE){
            left_to_run = 1;
            break;
        }
    }
    if(left_to_run){
        uthread_yield();
    }
    else{
        exit(0);
    }
}


int uthread_start_all(){
    if (start)
        return -1;

    start = 1;
    uthread_yield();

    while (1) {}
}


enum sched_priority uthread_set_priority(enum sched_priority priority){
    enum sched_priority p = currect_thread -> priority;
    currect_thread -> priority = priority;
    return p;

}
enum sched_priority uthread_get_priority(){
    return currect_thread -> priority;
}

struct uthread* uthread_self(){
    return currect_thread;
}