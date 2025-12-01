    .section .text
    .globl _start
_start:
    jal  x1, target
    addi x2, x0, 1
    target:
    addi x2, x0, 7
    ecall
