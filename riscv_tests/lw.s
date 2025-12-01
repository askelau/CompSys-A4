    .section .text
    .globl _start
_start:
    .data
    value: .word 1234
    .text
    la   x1, value
    lw   x2, 0(x1)
    ecall
