#ifndef __SIMULATE_H__
#define __SIMULATE_H__

#include "memory.h"
#include "read_elf.h"
#include <stdio.h>

// Simuler RISC-V program i givet lager og fra given start adresse
struct Stat {
    long int insns;

    // NT predictor
    long int nt_predictions;
    long int nt_mispredictions;

    // BTFNT predictor
    long int btfnt_predictions;
    long int btfnt_mispredictions;

    long bimodal_predictions[4];
    long bimodal_mispredictions[4];

    long gshare_predictions[4];
    long gshare_mispredictions[4];
};

// NOTE: Use of symbols provide for nicer disassembly, but is not required for A4.
// Feel free to remove this parameter or pass in a NULL pointer and ignore it.

struct Stat simulate(struct memory *mem, int start_addr, FILE *log_file, struct symbols* symbols);

#endif
