    .section .text
    .globl _start
_start:
    addi x1, x0, 5
    addi x2, x0, 4
    bne  x1, x2, neq
    addi x3, x0, 1
    neq:
    addi x3, x0, 9
    ecall
