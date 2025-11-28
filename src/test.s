    .section .text
    .globl _start
_start:
    addi x1, x0, 42   /* write 42 to x1 */ 
    addi x2, x0, 7    /* write 7 to x2 */ 
    add x3, x1, x2    /* x3 = x1 + x2 */
    ecall              /* top simulation */
