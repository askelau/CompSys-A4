# include "disassemble.h"
# include <stdio.h>
# include <stdint.h>
# include <stddef.h>
# include <string.h>

// Register names from standard RISC-V with ABI-name
static const char *regname[32] = {
    "zero","ra","sp","gp","tp","t0","t1","t2",
    "s0","s1","a0","a1","a2","a3","a4","a5",
    "a6","a7","s2","s3","s4","s5","s6","s7",
    "s8","s9","s10","s11","t3","t4","t5","t6"
};

// Helper for n-bit immediate in lower bits of value
static inline int32_t sign_extend(uint32_t v, int bits){
    uint32_t m = 1u << (bits - 1);
    int32_t result = (int32_t)v;
    if (v & m){
        result -= (int32_t)(2 * m);  // Subtracting 2*m to get negative value
    }
    return result;
}

// Immediate extractors
static inline int32_t imm_I(uint32_t instruct){
    return sign_extend(instruct >> 20, 12);
}

static inline int32_t imm_S(uint32_t instruct){
    uint32_t imm = ((instruct >> 25) << 5) | ((instruct >> 7) & 0x1f);
    return sign_extend(imm, 12); // 12 bits including sign bit
}

static inline int32_t imm_B(uint32_t instruct){
    uint32_t imm = ((instruct >> 31) & 0x1) << 12;
    imm |= (((instruct >> 25) & 0x3f) << 5);
    imm |= (((instruct >> 8) & 0xf) << 1);
    imm |= (((instruct >> 7) & 0x1) << 11);
    return sign_extend(imm, 13); // 13 bits including sign bit
}

static inline int32_t imm_U(uint32_t instruct){
    return (int32_t)(instruct & 0xfffff000u);
}

static inline int32_t imm_J(uint32_t instruct){
    uint32_t imm = ((instruct >> 31) & 0x3ff) << 20;
    imm |= (((instruct >> 21) & 0x3ff) << 1);
    imm |= (((instruct >> 20) & 0x1) << 11);
    imm |= (((instruct >> 12) & 0xff) << 12);
    return sign_extend(imm, 21); // 21 bits including sign bit
}

// Write safe formatted string into result buffer