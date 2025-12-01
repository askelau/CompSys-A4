    .section .text
    .globl _start
_start:
.data
    buf: .word 0
    .text
    la   x1, buf
    addi x2, x0, 99
    sw   x2, 0(x1)
    lw   x3, 0(x1)
    ecall
