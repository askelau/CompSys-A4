    .section .text
    .globl _start
_start:
    addi x5, x0, 20
    addi x6, x0, 7
    sub  x7, x5, x6
    ecall
