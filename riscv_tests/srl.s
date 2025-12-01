    .section .text
    .globl _start
_start:
    addi x1, x0, 16
    addi x2, x0, 2
    srl  x3, x1, x2
    ecall
