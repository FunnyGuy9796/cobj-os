#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>
#include <stdbool.h>
#include "../misc/printf.h"
#include "../misc/util.h"
#include "../mm/vmm.h"
#include "../mm/heap.h"

struct process;
typedef struct process process_t;

typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_DEAD
} thread_state_t;

typedef struct thread {
    uint64_t rsp;
    uint64_t *kernel_stack;
    uint64_t kernel_rsp_top;
    uint64_t stack_size;
    thread_state_t state;
    uint64_t id;
    uint64_t user_entry;
    uint64_t user_rsp;
    uint64_t user_argc;
    uint64_t user_argv;
    process_t *process;
    uint64_t waiting_for;
    uint64_t sleep_until;
    struct thread *next;
} thread_t;

extern volatile bool threads_enabled;
extern thread_t *curr_thread;
extern thread_t *idle_thread;

void thread_init(void (*idle_entry)());
thread_t *thread_create(void (*entry)());
void runqueue_push(thread_t *thread);
void runqueue_remove(thread_t *thread);
void schedule();
thread_t *thread_create_user(uint64_t entry, uint64_t user_rsp);
void thread_wake_pid(uint64_t pid);

#endif