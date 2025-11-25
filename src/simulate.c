# include "simulate.h"
# include <stdio.h>

struct Stat simulate(struct memory *mem, int start_addr, 
                    FILE *log_file, struct symbols* symbols){
    
    struct Stat s;
    s.insns = 0;

    fprintf(stderr, "[Simulate] Not implemented yet\n");

    return s;
}



// For instructions on RISC-V instructions go to page 248 or 169 or 397 in COD.