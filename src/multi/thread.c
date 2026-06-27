#include "thread.h"
#include "process.h"
#include "../userspace/cpu.h"
#include "../arch/tss.h"
#include "../arch/idt.h"

#define KERNEL_STACK_SIZE (4096 * 4)

volatile bool threads_enabled = false;
thread_t *curr_thread;
thread_t *idle_thread;

static uint64_t next_thread_id = 0;
static thread_t *runqueue_head = NULL;

extern void thread_idle(thread_t *thread);
extern void jump_userspace(uint64_t rip, uint64_t rsp);

void thread_init(void (*idle_entry)()) {
    idle_thread = thread_create(idle_entry);

    runqueue_push(idle_thread);

    curr_thread = idle_thread;
    idle_thread->state = THREAD_RUNNING;

    thread_idle(curr_thread);
}

extern void thread_entry_trampoline();

thread_t *thread_create(void (*entry)()) {
    thread_t *thread = kmalloc(sizeof(thread_t));

    if (!thread)
        panic("thread.c: thread_create() -> failed to allocate thread\n");
    
    uint64_t stack_phys = pmm_alloc_pages(4);

    if (!stack_phys)
        panic("thread.c: thread_create() -> failed to allocate stack\n");

    uint8_t *stack = (uint8_t *)(stack_phys + hhdm_offset);
    
    memset(stack, 0, KERNEL_STACK_SIZE);

    uint64_t *sp = (uint64_t *)(stack + KERNEL_STACK_SIZE);

    *--sp = 0x10;
    *--sp = (uint64_t)sp + 8;
    *--sp = 0x202;
    *--sp = 0x08;
    *--sp = (uint64_t)entry;

    *--sp = (uint64_t)thread_entry_trampoline;

    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;

    thread->rsp = (uint64_t)sp;
    thread->kernel_stack = (uint64_t *)stack;
    thread->kernel_rsp_top = (uint64_t)(stack + KERNEL_STACK_SIZE);
    thread->stack_size = KERNEL_STACK_SIZE;
    thread->state = THREAD_READY;
    thread->id = next_thread_id++;
    thread->process = NULL;
    thread->waiting_for = 0;
    thread->sleep_until = 0;
    thread->next = NULL;

    return thread;
}

void runqueue_push(thread_t *thread) {
    if (!thread)
        panic("thread.c: runqueue_push() -> thread is NULL\n");

    thread->state = THREAD_READY;

    if (!runqueue_head) {
        runqueue_head = thread;
        thread->next = thread;

        return;
    }

    thread_t *tail = runqueue_head;

    while (tail->next != runqueue_head)
        tail = tail->next;

    tail->next = thread;
    thread->next = runqueue_head;
}

thread_t *runqueue_next() {
    if (!runqueue_head)
        return curr_thread;

    thread_t *candidate = runqueue_head;

    do {
        if (candidate->state == THREAD_BLOCKED && candidate->sleep_until > 0 && apic_timer_ticks() >= candidate->sleep_until) {
            candidate->state = THREAD_READY;
            candidate->sleep_until = 0;
        }

        if (candidate->state == THREAD_READY) {
            runqueue_head = candidate->next;

            return candidate;
        }

        candidate = candidate->next;
    } while (candidate != runqueue_head);

    return idle_thread;
}

void runqueue_remove(thread_t *thread) {
    if (!runqueue_head)
        return;

    if (runqueue_head == thread && runqueue_head->next == runqueue_head) {
        runqueue_head = NULL;

        return;
    }

    thread_t *prev = runqueue_head;

    while (prev->next != thread) {
        prev = prev->next;
        
        if (prev == runqueue_head)
            return;
    }

    prev->next = thread->next;

    if (runqueue_head == thread)
        runqueue_head = thread->next;

    thread->next = NULL;
}

extern void thread_switch(thread_t *current, thread_t *next);

void schedule() {
    thread_t *next = runqueue_next();

    if (next == curr_thread)
        return;

    thread_t *prev = curr_thread;

    if (prev->state == THREAD_RUNNING)
        prev->state = THREAD_READY;

    curr_thread = next;
    next->state = THREAD_RUNNING;
    cpu_local.kernel_rsp = next->kernel_rsp_top;

    tss_set_rsp0(next->kernel_rsp_top);

    if (next->process)
        vmm_switch_space(next->process->addr_space);
    else
        vmm_switch_space(&kernel_addr_space);

    thread_switch(prev, next);
}

void thread_userspace_enter() {
    cpu_local.kernel_rsp = curr_thread->kernel_rsp_top;

    tss_set_rsp0(curr_thread->kernel_rsp_top);
    vmm_switch_space(curr_thread->process->addr_space);
    jump_userspace(curr_thread->user_entry, curr_thread->user_rsp);
}

thread_t *thread_create_user(uint64_t entry, uint64_t user_rsp) {
    thread_t *thread = kmalloc(sizeof(thread_t));

    if (!thread)
        panic("thread_create_user: failed to allocate thread\n");

    uint64_t stack_phys = pmm_alloc_pages(4);

    if (!stack_phys)
        panic("thread_create_user: failed to allocate stack\n");

    uint8_t *stack = (uint8_t *)(stack_phys + hhdm_offset);

    memset(stack, 0, KERNEL_STACK_SIZE);

    uint64_t *sp = (uint64_t *)(stack + KERNEL_STACK_SIZE);

    *--sp = (uint64_t)thread_userspace_enter;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;

    thread->rsp = (uint64_t)sp;
    thread->kernel_stack = (uint64_t *)stack;
    thread->kernel_rsp_top = (uint64_t)(stack + KERNEL_STACK_SIZE);
    thread->stack_size = KERNEL_STACK_SIZE;
    thread->state = THREAD_READY;
    thread->id = next_thread_id++;
    thread->user_entry = entry;
    thread->user_rsp = user_rsp;
    thread->process = NULL;
    thread->waiting_for = (uint64_t)-1ULL;
    thread->sleep_until = 0;
    thread->next = NULL;

    return thread;
}

void thread_wake_pid(uint64_t pid) {
    if (!runqueue_head)
        return;

    thread_t *t = runqueue_head;

    do {
        if (t->state == THREAD_BLOCKED && t->waiting_for == pid) {
            t->state = THREAD_READY;
            t->waiting_for = -1ULL;
        }

        t = t->next;
    } while (t != runqueue_head);
}