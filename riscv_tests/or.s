    .section .text
    .globl _start
_start:
    addi x1, x0, 0b1001
    addi x2, x0, 0b0011 
    or   x3, x1, x2
    ecall
