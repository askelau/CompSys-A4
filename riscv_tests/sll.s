    .section .text
    .globl _start
_start:
    addi x1, x0, 3
    addi x2, x0, 2
    sll  x3, x1, x2
    ecall
