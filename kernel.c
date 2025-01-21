#include <stdint.h>

#define VIDEO_MEMORY 0xB8000
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define WHITE_ON_BLACK 0x07
#define STACK_SIZE 1024

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
    // Настройка PIC и обработки прерываний
}

void kernel_main() {
    clear_screen();
    put_char('H');
    put_char('e');
    put_char('l');
    put_char('l');
    put_char('o');
    put_char(',');
    put_char(' ');
    put_char('O');
    put_char('S');
    put_char(' ');
    put_char('W');
    put_char('o');
    put_char('r');
    put_char('l');
    put_char('d');
    put_char('!');

    setup_timer();
    setup_interrupts();

    while (1) {
        switch_task();
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