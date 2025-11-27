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
    uint32_t imm = 0;
    imm |= (((instruct >> 31) & 0x1) << 20);
    imm |= (((instruct >> 21) & 0x3ff) << 1);
    imm |= (((instruct >> 20) & 0x1) << 11);
    imm |= (((instruct >> 12) & 0xff) << 12);
    return sign_extend(imm, 21); // 21 bits including sign bit
}

// Write safe formatted string into result buffer
static void safe_snprintf(char *buf, size_t buf_size, const char *format, ...){
    va_list argp;
    va_start(argp, format);
    if (buf_size > 0){
        vsnprintf(buf, buf_size, format, argp);
        buf[buf_size - 1] = '\0';
    }
    va_end(argp);
}

void disassemble(uint32_t addr, uint32_t instruction, char* result,
                size_t buf_size, struct symbols* symbols){
    
    (void)symbols;  // Don't really use them

    uint32_t opcode = instruction & 0x7f;
    uint32_t rd = (instruction >> 7) & 0x1f;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    uint32_t rs1 = (instruction >> 15) & 0x1f;
    uint32_t rs2 = (instruction >> 20) & 0x1f;
    uint32_t funct7 = (instruction >> 25) & 0x7f;

    // Tempoary buffer
    char buf[256];
    buf[0] = '\0';

    switch (opcode){
        case 0x33:   // This is an R-Type so add, sub, or, (and others)
            if (funct7 == 0x01){
                // RV32M (mul, div, and others)
                switch (funct3) {
                    case 0x0: snprintf(buf, sizeof(buf), "mul %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]); break;
                    case 0x1: snprintf(buf, sizeof(buf), "mulh %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]); break;
                    case 0x2: snprintf(buf, sizeof(buf), "mulhsu %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]); break;
                    case 0x3: snprintf(buf, sizeof(buf), "mulhu %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]); break;
                    case 0x4: snprintf(buf, sizeof(buf), "div %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]); break;
                    case 0x5: snprintf(buf, sizeof(buf), "divu %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]); break;
                    case 0x6: snprintf(buf, sizeof(buf), "rem %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]); break;
                    case 0x7: snprintf(buf, sizeof(buf), "remu %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]); break;
                    default: snprintf(buf, sizeof(buf), "unknown"); break;
                }
                } else {
                    // Standard R-type
                    if (funct3 == 0x0 && funct7 == 0x20){
                        snprintf(buf, sizeof(buf), "sub %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]);
                    } else if (funct3 == 0x0){
                        snprintf(buf, sizeof(buf), "add %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]);
                    } else if (funct3 == 0x1){
                        snprintf(buf, sizeof(buf), "sll %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]);
                    } else if (funct3 == 0x2){
                        snprintf(buf, sizeof(buf), "slt %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]);
                    } else if (funct3 == 0x3){
                        snprintf(buf, sizeof(buf), "sltu %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]);
                    } else if (funct3 == 0x4){
                        snprintf(buf, sizeof(buf), "xor %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]);
                    } else if (funct3 == 0x5 && funct7 == 0x20){
                        snprintf(buf, sizeof(buf), "sra %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]);
                    } else if (funct3 == 0x5){
                        snprintf(buf, sizeof(buf), "srl %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]);
                    } else if (funct3 == 0x6){
                         snprintf(buf, sizeof(buf), "or %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]);
                    } else if (funct3 == 0x7){
                         snprintf(buf, sizeof(buf), "and %s,%s,%s", regname[rd], 
                                        regname[rs1], regname[rs2]);
                    } else {
                         snprintf(buf, sizeof(buf), "unknown");
                    }
                }
                break;
            
        case 0x13: // I-type (addi, slti, xori, and others)
                if (funct3 == 0x1){
                    // For slli
                    uint32_t shift_amount = (instruction >> 20) & 0x1f;
                    snprintf(buf, sizeof(buf), "slli %s,%s,%u", regname[rd], regname[rs1], shift_amount);
                } else if (funct3 == 0x5){
                    uint32_t shift_amount = (instruction >> 20) & 0x1f;
                    if ((instruction >> 25) & 0x7f){
                        // srai
                        snprintf(buf, sizeof(buf), "srai %s,%s,%u", regname[rd], 
                                regname[rs1], shift_amount);
                    } else {
                        // srli
                        snprintf(buf, sizeof(buf), "srli %s,%s,%u", regname[rd], 
                                regname[rs1], shift_amount);
                    }
                } else {
                    int32_t imm = imm_I(instruction);
                    switch (funct3) {
                        case 0x0: snprintf(buf, sizeof(buf), "addi %s,%s,%d", regname[rd], 
                                            regname[rs1], imm); break;
                        case 0x2: snprintf(buf, sizeof(buf), "slti %s,%s,%d", regname[rd], 
                                            regname[rs1], imm); break;
                        case 0x3: snprintf(buf, sizeof(buf), "sltiu %s,%s,%d", regname[rd], 
                                            regname[rs1], imm); break;
                        case 0x4: snprintf(buf, sizeof(buf), "xori %s,%s,%d", regname[rd], 
                                            regname[rs1], imm); break;
                        case 0x6: snprintf(buf, sizeof(buf), "ori %s,%s,%d", regname[rd], 
                                            regname[rs1], imm); break;
                        case 0x7: snprintf(buf, sizeof(buf), "andi %s,%s,%d", regname[rd], 
                                            regname[rs1], imm); break;
                        default: snprintf(buf, sizeof(buf), "unknown"); break;
                } 
            } 
            break;
        
        
        case 0x03: {    // Loads: lb, lh, lw, lbu, lhu
            int32_t imm = imm_I(instruction);
            switch(funct3){
                case 0x0: snprintf(buf, sizeof(buf), "lb %s,%d(%s)", regname[rd], imm, 
                                    regname[rs1]); break;
                case 0x1: snprintf(buf, sizeof(buf), "lh %s,%d(%s)", regname[rd], imm,
                                    regname[rs1]); break;
                case 0x2: snprintf(buf, sizeof(buf), "lw %s,%d(%s)", regname[rd], imm, 
                                    regname[rs1]); break;
                case 0x4: snprintf(buf, sizeof(buf), "lbu %s,%d(%s)", regname[rd],  imm,
                                    regname[rs1]); break;
                case 0x5: snprintf(buf, sizeof(buf), "lhu %s,%d(%s)", regname[rd], imm,
                                    regname[rs1]); break;
                default: snprintf(buf, sizeof(buf), "unknown"); break;
            }
            break;
        }

        case 0x23: {    // Stores sb, sh, sw
            int32_t imm = imm_S(instruction);
            switch(funct3){
                case 0x0: snprintf(buf, sizeof(buf), "sb %s,%d(%s)", regname[rs2], imm, 
                                    regname[rs1]); break;
                case 0x1: snprintf(buf, sizeof(buf), "sh %s,%d(%s)", regname[rs2], imm,
                                    regname[rs1]); break;
                case 0x2: snprintf(buf, sizeof(buf), "sw %s,%d(%s)", regname[rs2], imm, 
                                    regname[rs1]); break;
                default: snprintf(buf, sizeof(buf), "unknown"); break;
            }
            break;
        }


        case 0x63: {    // Branches: beq, bne, blt, bge, bltu, bgeu
            int32_t imm = imm_B(instruction);
            uint32_t target = (uint32_t)((int32_t)addr + imm);
            switch(funct3){
                case 0x0: snprintf(buf, sizeof(buf), "beq %s,%s,0x%08x", regname[rs1], 
                                    regname[rs2], target); break;
                case 0x1: snprintf(buf, sizeof(buf), "bne %s,%s,0x%08x", regname[rs1],
                                    regname[rs2], target); break;
                case 0x4: snprintf(buf, sizeof(buf), "blt %s,%s,0x%08x", regname[rs1], 
                                    regname[rs2], target); break;
                case 0x5: snprintf(buf, sizeof(buf), "bge %s,%s,0x%08x", regname[rs1],
                                    regname[rs2], target); break;
                case 0x6: snprintf(buf, sizeof(buf), "bltu %s,%s,0x%08x", regname[rs1],
                                    regname[rs2], target); break;
                case 0x7: snprintf(buf, sizeof(buf), "bgeu %s,%s,0x%08x", regname[rs1],
                                    regname[rs2], target); break;
                default: snprintf(buf, sizeof(buf), "unknown"); break;
            }
            break;
        }

        case 0x6f: {    // jal
            int32_t imm = imm_J(instruction);
            uint32_t target = (uint32_t)((int32_t)addr + imm);
            snprintf(buf, sizeof(buf), "jal %s,0x%08x", regname[rd], target);
            break;
        }

        case 0x67: {    // jalr
            int32_t imm = imm_I(instruction);
            snprintf(buf, sizeof(buf), "jalr %s,%d(%s)", regname[rd], imm, regname[rs1]);
            break;
        }

        case 0x37: {    // lui
            int32_t imm = imm_U(instruction);
            snprintf(buf, sizeof(buf), "lui %s,%d", regname[rd], imm);
            break;
        }

        case 0x17: {    // auipc
            int32_t imm = imm_U(instruction);
            uint32_t target = (uint32_t)((int32_t)addr + imm);
            snprintf(buf, sizeof(buf), "auipc %s,0x%08x", regname[rd], target);
            break;
        }

        case 0x73: { // System: ecall and everything else is unknown
            if (instruction == 0x00000073){
                snprintf(buf, sizeof(buf), "ecall");
            } else {
                snprintf(buf, sizeof(buf), "unknown");
            }
            break;
        }  
        
        default:
            snprintf(buf, sizeof(buf), "unknown");
            break;
    }

    // Copy to user buffer so no overflow
    if (result && buf_size > 0){
        // Ensuring null-termination
        strncpy(result, buf, buf_size - 1);
        result[buf_size - 1] = '\0';
    }
}
