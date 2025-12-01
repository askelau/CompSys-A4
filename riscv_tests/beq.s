    .section .text
    .globl _start
_start:
    addi x1, x0, 5
    addi x2, x0, 5
    beq  x1, x2, equal
    addi x3, x0, 1
    equal:
    addi x3, x0, 2
    ecall
