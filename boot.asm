[bits 16]
[org 0x7C00]

mov ah, 0x0E
mov al, 'H'
int 0x10

mov al, 'e'
int 0x10

mov al, 'l'
int 0x10

mov al, 'l'
int 0x10

mov al, 'o'
int 0x10

jmp 0x0000:0x1000

[bits 32]
extern timer_interrupt_handler

timer_irq:
    pusha
    call timer_interrupt_handler
    popa
    iret
