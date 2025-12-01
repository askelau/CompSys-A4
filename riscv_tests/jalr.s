    .section .text
    .globl _start
_start:
    addi x1, x0, 0
    la   x2, target
    jalr x1, 0(x2)
    addi x3, x0, 1
    target:
    addi x3, x0, 5
    ecall
