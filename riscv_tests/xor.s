    .section .text
    .globl _start
_start:
    addi x1, x0, 0b1010
    addi x2, x0, 0b1100
    xor  x3, x1, x2
    ecall
