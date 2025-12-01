    .section .text
    .globl _start
_start:
    addi x1, x0, 0b1100
    addi x2, x0, 0b1010
    and  x3, x1, x2
    ecall
