#include <stdint.h>

#define VIDEO_MEMORY 0xB8000
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define WHITE_ON_BLACK 0x07
#define STACK_SIZE 1024

#define IDT_ENTRIES 256
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

typedef struct {
    uint16_t offset_low;    // The lower 16 bits of the handler's address
    uint16_t selector;      // Kernel segment selector
    uint8_t zero;           // This must be zero
    uint8_t type_attr;      // Type and attributes
    uint16_t offset_high;   // The higher 16 bits of the handler's address
} __attribute__((packed)) IDT_ENTRY;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) IDT_PTR;

typedef struct {
    uint32_t *stack_pointer;
    uint32_t *stack;
    uint32_t stack_size;
} Task;

Task *current_task = 0;
char *video_memory = (char *) VIDEO_MEMORY;
int cursor_x = 0;
int cursor_y = 0;

Task task1, task2;

IDT_ENTRY idt_entries[IDT_ENTRIES];
IDT_PTR idt_ptr;

extern void timer_irq(); // Defined in boot.asm

void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void clear_screen() {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        video_memory[i * 2] = ' ';
        video_memory[i * 2 + 1] = WHITE_ON_BLACK;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void put_char(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        video_memory[(cursor_y * SCREEN_WIDTH + cursor_x) * 2] = c;
        video_memory[(cursor_y * SCREEN_WIDTH + cursor_x) * 2 + 1] = WHITE_ON_BLACK;
        cursor_x++;
    }

    if (cursor_x >= SCREEN_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= SCREEN_HEIGHT) {
        cursor_y = SCREEN_HEIGHT - 1;
        for (int i = 0; i < SCREEN_WIDTH * (SCREEN_HEIGHT - 1); i++) {
            video_memory[i * 2] = video_memory[(i + SCREEN_WIDTH) * 2];
            video_memory[i * 2 + 1] = video_memory[(i + SCREEN_WIDTH) * 2 + 1];
        }
        for (int i = 0; i < SCREEN_WIDTH; i++) {
            video_memory[(SCREEN_HEIGHT - 1) * SCREEN_WIDTH * 2 + i * 2] = ' ';
            video_memory[(SCREEN_HEIGHT - 1) * SCREEN_WIDTH * 2 + i * 2 + 1] = WHITE_ON_BLACK;
        }
    }
}

void print_string(const char* str) {
    while (*str) {
        put_char(*str++);
    }
}

void init_task(Task *task, uint32_t *stack, uint32_t stack_size) {
    task->stack = stack;
    task->stack_size = stack_size;
    task->stack_pointer = stack + stack_size / sizeof(uint32_t) - 1;
}

void switch_task() {
    __asm__ volatile (
        "mov %%esp, %0\n"
        : "=r" (current_task->stack_pointer)
    );

    current_task = current_task == &task1 ? &task2 : &task1;

    __asm__ volatile (
        "mov %0, %%esp\n"
        :
        : "r" (current_task->stack_pointer)
    );
}

void setup_timer() {
    uint32_t divisor = 1193180 / 100;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

void setup_interrupts() {
    idt_init();
    idt_set_gate(0x20, (uint32_t)timer_irq, 0x08, 0x8E);
}

void kernel_main() {
    clear_screen();
    print_string("Hello, OS World!\n");

    setup_timer();
    setup_interrupts();

    while (1) {
        // switch_task(); // Handled by timer interrupt
    }
}

void *malloc(size_t size) {
    static uint32_t heap_start = 0x100000;
    uint32_t *ptr = (uint32_t *)heap_start;
    heap_start += size;
    return ptr;
}

void free(void *ptr) {
    // Реализация освобождения памяти (по необходимости)
}

void interrupt_handler() {
    // Обработка прерываний
}

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].offset_low = base & 0xFFFF;
    idt_entries[num].offset_high = (base >> 16) & 0xFFFF;

    idt_entries[num].selector = sel;
    idt_entries[num].zero = 0;
    idt_entries[num].type_attr = flags;
}

void idt_init() {
    idt_ptr.limit = sizeof(IDT_ENTRY) * IDT_ENTRIES - 1;
    idt_ptr.base = (uint32_t)&idt_entries;

    // Null out all 256 IDT entries first
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // Remap the PIC
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC1_DATA, 0x20); // PIC1_DATA offset of 0x20 in IDT
    outb(PIC2_DATA, 0x28); // PIC2_DATA offset of 0x28 in IDT
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    outb(PIC1_DATA, 0x0);
    outb(PIC2_DATA, 0x0);

    __asm__ volatile("lidt %0" : : "memory"(&idt_ptr));
    __asm__ volatile("sti");
}

void timer_interrupt_handler() {
    print_string("Tick!\n");
    switch_task();
    outb(PIC1_COMMAND, 0x20); // End of Interrupt
}
