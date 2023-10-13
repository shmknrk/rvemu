    .equ TOHOST_ADDR, 0x40008000
    .equ CMD_PRINT_CHAR, 0b01
    .equ CMD_POWER_OFF, 0b10
    .section .text.init
    .globl _start
_start:
    nop
    li x1, 0
    li x2, 0
    li x3, 0
    li x4, 0
    li x5, 0
    li x6, 0
    li x7, 0
    li x8, 0
    li x9, 0
    li x10, 0
    li x11, 0
    li x12, 0
    li x13, 0
    li x14, 0
    li x15, 0
    li x16, 0
    li x17, 0
    li x18, 0
    li x19, 0
    li x20, 0
    li x21, 0
    li x22, 0
    li x23, 0
    li x24, 0
    li x25, 0
    li x26, 0
    li x27, 0
    li x28, 0
    li x29, 0
    li x30, 0
    li x31, 0

    # set the stack pointer 128 KiB
    li sp, 0x20000
    jal main

terminate:
    li t0, TOHOST_ADDR
    li t1, CMD_POWER_OFF << 16
    sw t1, 0(t0)
1:  j 1b

uart_puts:
    li t0, TOHOST_ADDR
    li t1, CMD_PRINT_CHAR << 16
1:  lbu t2, (a0)
    beqz t2, 1f
    or t2, t1, t2
    sw t2, 0(t0)
    addi a0, a0, 1
    j 1b
1:  ret

    .globl start_trigger
start_trigger:
    la a0, str_start
    j uart_puts
    .globl stop_trigger
stop_trigger:
    la a0, str_stop
    j uart_puts

    .section .rodata
    .align 4
str_start:
    .string "start!\n"
    .align 4
str_stop:
    .string "stop!\n"
    .align 4

.end _start
