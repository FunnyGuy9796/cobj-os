#include "idt.h"
#include "../multi/thread.h"
#include "../multi/process.h"
#include "../devices/kb.h"

#define IDT_ENTRIES 256

#define PIC1_DATA 0x21
#define PIC2_DATA 0xa1

#define LAPIC_ID 0x020
#define LAPIC_EOI 0x0b0
#define LAPIC_SPURIOUS 0x0f0
#define LAPIC_SIVR_ENABLE (1 << 8)
#define LAPIC_SPURIOUS_VECTOR 0xff
#define LAPIC_TPR 0x080
#define LAPIC_LVT_LINT0 0x350
#define LAPIC_LVT_LINT1 0x360
#define LAPIC_LVT_ERROR 0x370
#define LAPIC_LVT_MASKED (1 << 16)
#define LAPIC_LVT_TIMER 0x320
#define LAPIC_TIMER_ICR 0x380
#define LAPIC_TIMER_CCR 0x390
#define LAPIC_TIMER_DCR 0x3e0
#define LAPIC_TIMER_PERIODIC (1 << 17)
#define LAPIC_TIMER_VECTOR 0x20
#define PIT_CHANNEL2 0x42
#define PIT_CMD 0x43
#define PIT_CTRL 0x61
#define IOAPIC_BASE 0xfec00000
#define IOAPIC_REGSEL 0x00
#define IOAPIC_REGWIN 0x10
#define PS2_DATA_PORT 0x60
#define KEYBOARD_VECTOR (32 + 1)

static const char *exception_names[] = {
    "division by zero",
    "debug",
    "NMI",
    "breakpoint",
    "overflow",
    "bound range exceeded",
    "invalid opcode",
    "device not available",
    "double fault",
    "coprocessor segment overrun",
    "invalid TSS",
    "segment not present",
    "stack fault",
    "general protection fault",
    "page fault",
    "reserved",
    "x87 floating point",
    "alignment check",
    "machine check",
    "SIMD floating point",
};

static idt_entry_t idt[IDT_ENTRIES];
static idtr_t idtr;
static uint64_t lapic_base = 0;
static uint32_t lapic_timer_tpm = 0;
static uint64_t ioapic_base = 0;

volatile uint64_t lapic_timer_ticks = 0;

static inline uint32_t lapic_read(uint32_t reg) {
    return *((volatile uint32_t *)(lapic_base + reg));
}

static inline void lapic_write(uint32_t reg, uint32_t val) {
    *((volatile uint32_t *)(lapic_base + reg)) = val;
}

static void pic_disable() {
    outb(PIC1_DATA, 0xff);
    outb(PIC2_DATA, 0xff);
}

static void pit_sleep_ms(uint32_t ms) {
    uint32_t count = 1193 * ms;
    uint8_t ctrl = inb(PIT_CTRL);

    outb(PIT_CTRL, (ctrl & ~0x01));

    outb(PIT_CMD, 0b10110000);
    outb(PIT_CHANNEL2, count & 0xff);
    outb(PIT_CHANNEL2, (count >> 8) & 0xff);

    outb(PIT_CTRL, inb(PIT_CTRL) | 0x01);

    while (!(inb(PIT_CTRL) & 0x20));
}

static void lapic_timer_calibrate() {
    lapic_write(LAPIC_TIMER_DCR, 0x3);
    lapic_write(LAPIC_LVT_TIMER, LAPIC_LVT_MASKED);
    lapic_write(LAPIC_TIMER_ICR, 0xffffffff);

    pit_sleep_ms(10);

    uint32_t ticks = 0xffffffff - lapic_read(LAPIC_TIMER_CCR);

    lapic_timer_tpm = ticks / 10;
}

static void lapic_timer_start(uint32_t ms) {
    lapic_write(LAPIC_TIMER_DCR, 0x3);
    lapic_write(LAPIC_LVT_TIMER, LAPIC_TIMER_VECTOR | LAPIC_TIMER_PERIODIC);
    lapic_write(LAPIC_TIMER_ICR, lapic_timer_tpm * ms);
}

static void lapic_init() {
    uint32_t lo, hi;

    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0x1b));

    uint64_t phys = ((uint64_t)hi << 32 | lo) & ~0xfffULL;
    uint64_t virt = phys + hhdm_offset;

    vmm_map_page(&kernel_addr_space, virt, phys, PAGE_WRITE | PAGE_PWT | PAGE_PCD);

    lapic_base = virt;

    lo |= (1 << 11);
    __asm__ volatile ("wrmsr" :: "c"(0x1b), "a"(lo), "d"(hi));

    lapic_write(LAPIC_TPR, 0);
    lapic_write(LAPIC_LVT_LINT0, LAPIC_LVT_MASKED);
    lapic_write(LAPIC_LVT_LINT1, LAPIC_LVT_MASKED);
    lapic_write(LAPIC_LVT_ERROR, 0x20 | LAPIC_LVT_MASKED);
    lapic_write(LAPIC_SPURIOUS, lapic_read(LAPIC_SPURIOUS) | LAPIC_SIVR_ENABLE | 0xff);

    lapic_timer_calibrate();
    lapic_timer_start(10);
}

static void idt_set_entry(int i, uint64_t handler, uint8_t type_attr) {
    idt[i].offset_low = handler & 0xffff;
    idt[i].offset_mid = (handler >> 16) & 0xffff;
    idt[i].offset_high = (handler >> 32) & 0xffffffff;
    idt[i].selector = 0x08;
    idt[i].ist = 0;
    idt[i].type_attr = type_attr;
    idt[i].zero = 0;
}

static inline uint32_t ioapic_read(uint32_t reg) {
    *((volatile uint32_t *)(ioapic_base + IOAPIC_REGSEL)) = reg;

    return *((volatile uint32_t *)(ioapic_base + IOAPIC_REGWIN));
}

static inline void ioapic_write(uint32_t reg, uint32_t val) {
    *((volatile uint32_t *)(ioapic_base + IOAPIC_REGSEL)) = reg;
    *((volatile uint32_t *)(ioapic_base + IOAPIC_REGWIN)) = val;
}

static void ioapic_init() {
    uint64_t phys = IOAPIC_BASE;
    uint64_t virt = phys + hhdm_offset;

    vmm_map_page(&kernel_addr_space, virt, phys, PAGE_WRITE | PAGE_PWT | PAGE_PCD);

    ioapic_base = virt;
}

static void ioapic_set_irq(uint8_t irq, uint8_t vector, uint32_t lapic_id) {
    uint32_t low = vector;
    uint32_t high = lapic_id << 24;

    ioapic_write(0x10 + irq * 2, low);
    ioapic_write(0x10 + irq * 2 + 1, high);
}

extern uint64_t isr_table[32];
extern uint64_t irq_table[16];

void idt_init() {
    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)&idt;

    for (int i = 0; i < 32; i++)
        idt_set_entry(i, isr_table[i], 0x8e);

    for (int i = 0; i < 16; i++)
        idt_set_entry(32 + i, irq_table[i], 0x8e);

    __asm__ volatile ("lidt %0" : : "m"(idtr));
}

void idt_enable() {
    pic_disable();
    lapic_init();
    ioapic_init();
    ioapic_set_irq(1, 32 + 1, (lapic_read(LAPIC_ID) >> 24));

    __asm__ volatile ("sti");
}

void lapic_eoi() {
    lapic_write(LAPIC_EOI, 0);
}

void isr_handler(int_frame_t *frame) {
    if ((frame->cs & 3) == 3) {
        const char *name = frame->vector < 20 ? exception_names[frame->vector] : "unknown";

        fbcon_printf("[E] userspace exception: pid=%d (%s) rip=%p\n",
            curr_thread->process ? curr_thread->process->pid : -1, name, frame->rip);
        fbcon_printf("  cr4=%lx cr3=%lx cr2=%lx cr0=%lx\n  r15=%lx r14=%lx r13=%lx r12=%lx\n  r11=%lx r10=%lx r9=%lx r8=%lx\n  rbp=%lx rdi=%lx rsi=%lx rdx=%lx\n  rcx=%lx rbx=%lx rax=%lx rsp=%lx cs=%lx\n",
            frame->cr4, frame->cr3, frame->cr2, frame->cr0, frame->r15, frame->r14,
            frame->r13, frame->r12, frame->r11, frame->r10, frame->r9, frame->r8, frame->rbp, frame->rdi, frame->rsi,
            frame->rdx, frame->rcx, frame->rbx, frame->rax, frame->rsp, frame->cs);
        
        runqueue_remove(curr_thread);

        curr_thread->state = THREAD_DEAD;

        schedule();

        __builtin_unreachable();
    }

    fbcon_printf("[EXCEPTION] vector=%d error=%x rip=%p rflags=%p\n", frame->vector, frame->error, frame->rip, frame->rflags);
    fbcon_printf("  cr4=%lx cr3=%lx cr2=%lx cr0=%lx\n  r15=%lx r14=%lx r13=%lx r12=%lx\n  r11=%lx r10=%lx r9=%lx r8=%lx\n  rbp=%lx rdi=%lx rsi=%lx rdx=%lx\n  rcx=%lx rbx=%lx rax=%lx rsp=%lx cs=%lx\n",
        frame->cr4, frame->cr3, frame->cr2, frame->cr0, frame->r15, frame->r14,
        frame->r13, frame->r12, frame->r11, frame->r10, frame->r9, frame->r8, frame->rbp, frame->rdi, frame->rsi,
        frame->rdx, frame->rcx, frame->rbx, frame->rax, frame->rsp, frame->cs);

    for (;;)
        __asm__ volatile ("hlt");
}

void irq_handler(uint64_t vector) {
    if (vector == LAPIC_SPURIOUS_VECTOR) {
        fbcon_printf("[W] spurious IRQ received\n");
        
        return;
    }

    if (vector == LAPIC_TIMER_VECTOR) {
        lapic_eoi();

        lapic_timer_ticks++;

        if (lapic_timer_ticks % 6000 == 0) {
            boot_time = rtc_read();
            boot_epoch = rtc_to_epoch(&boot_time);
            boot_ticks = apic_timer_ticks();
        }
        
        if (proc_needs_cleanup) {
            proc_needs_cleanup = false;

            proc_cleanup();
        }

        if (threads_enabled)
            schedule();

        return;
    }

    if (vector == KEYBOARD_VECTOR) {
        uint8_t scancode = inb(PS2_DATA_PORT);

        kbd_handle_scancode(scancode);

        lapic_eoi();

        return;
    }

    fbcon_printf("[I] vector=%d\n", vector);

    lapic_eoi();
}

uint64_t apic_timer_ticks() {
    return lapic_timer_ticks;
}