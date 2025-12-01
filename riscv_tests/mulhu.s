    .section .text
    .globl _start
_start:
    addi x5, x0, 7
    addi x6, x0, 3
    mulhu x7, x5, x6
    ecall
    