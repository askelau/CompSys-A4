# include "disassemble.h"
# include "simulate.h"
# include "memory.h"
# include <stdio.h>
# include <stdint.h>
# include <stddef.h>
# include <string.h>
# include <stdarg.h>


// Helper functions for logging events
static inline void log_reg_write(FILE *log, int rd, uint32_t value){
    // Log writes
    if (!log) {
        // Logging disabled
        return;
    }

    if (rd != 0){
        fprintf(log, " Register write: x%d = 0x%08X\n", rd, value);
    } else {
        fprintf(log, " Ignored write to x0\n");
    }
}

static inline void log_mem_write(FILE *log, uint32_t addr, uint32_t value){
    // Log memory writes
    if (log){
        fprintf(log, " Memory write: MEM[0x%08X] = 0x%08X\n", addr, value);
    }
}

// Sign-extend helpers
static inline int32_t sign_extend(uint32_t v, int bits){
    // Extend n-bit value to 32-bit signed integer
    uint32_t m = 1u << (bits - 1);
    int32_t result = (int32_t)v;
    if (v & m){
        result -= (int32_t)(2 * m);  // Subtracting 2*m to get negative value
    }
    return result;
}

// Immediate extraction helpers
static inline int32_t imm_I(uint32_t instruct){ // I-type
    return sign_extend(instruct >> 20, 12);
}

static inline int32_t imm_S(uint32_t instruct){ // S-type
    uint32_t imm = ((instruct >> 25) << 5) | ((instruct >> 7) & 0x1f);
    return sign_extend(imm, 12); // 12 bits including sign bit
}

static inline int32_t imm_B(uint32_t instruct){ // B-type
    uint32_t imm = ((instruct >> 31) & 0x1) << 12;
    imm |= (((instruct >> 25) & 0x3f) << 5);
    imm |= (((instruct >> 8) & 0xf) << 1);
    imm |= (((instruct >> 7) & 0x1) << 11);
    return sign_extend(imm, 13); // 13 bits including sign bit
}

static inline int32_t imm_U(uint32_t instruct){ // U-type
    return (int32_t)(instruct & 0xfffff000u);
}

static inline int32_t imm_J(uint32_t instruct){ // J-type
    uint32_t imm = 0;
    imm |= (((instruct >> 31) & 0x1) << 20);
    imm |= (((instruct >> 21) & 0x3ff) << 1);
    imm |= (((instruct >> 20) & 0x1) << 11);
    imm |= (((instruct >> 12) & 0xff) << 12);
    return sign_extend(imm, 21); // 21 bits including sign bit
}

// Formated logging helper
static void log_fmt(FILE *log, const char *format, ...){
    if (!log) return;
    va_list argp;
    va_start(argp, format);
    vfprintf(log, format, argp);
    va_end(argp);
}



// Division/remainder helpers
static int32_t div_s(int32_t a, int32_t b){
    if (b == 0){
        return -1;
    }
    if (a == INT32_MIN && b == -1){
        return a;   // Overflow case
    }
    return a / b;
}

static uint32_t div_u(uint32_t a, uint32_t b){
    if (b == 0){
        return UINT32_MAX;
    }
    return a / b;
}

// Remainders
static int32_t rem_s(int32_t a, int32_t b){
    if (b == 0){
        return a;
    }
    if (a == INT32_MIN && b == -1){
        return 0;   // Overflow case
    }
    return a % b;
}

static uint32_t rem_u(uint32_t a, uint32_t b){
    if (b == 0){
        return a;
    }
    return a % b;
}
//  RISC-V simulator
struct Stat simulate(struct memory *mem, int start_addr, FILE *log_file, 
                    struct symbols* symbols){

    // Initialize logging
    if (log_file) {
        fprintf(log_file, "Simulator logging enabled\n");
        fflush(log_file);
    } else {
        fprintf(stderr, "Simulator logging disabled\n");
    }
    
     // Initialize registers and PC
    uint32_t R[32];
    memset(R, 0, sizeof(R));
    uint32_t PC = (uint32_t)start_addr;
    long instr_count = 0;
    struct Stat stats;
    stats.insns = 0;

    stats.nt_predictions = 0;
    stats.nt_mispredictions = 0;

    stats.btfnt_predictions = 0;
    stats.btfnt_mispredictions = 0;

    // Buffer for disassembly when logging
    char disassem_buf[256];

    int stop = 0;
    while(!stop){
        uint32_t instruction = memory_rd_w(mem, PC);    // fetch
        uint32_t current_pc = PC;
        instr_count++;

        // Decodeing standard RISC-V fields
        uint32_t opcode = instruction & 0x7f;
        uint32_t rd = (instruction >> 7) & 0x1f;
        uint32_t funct3 = (instruction >> 12) & 0x7;
        uint32_t rs1 = (instruction >> 15) & 0x1f;
        uint32_t rs2 = (instruction >> 20) & 0x1f;
        uint32_t funct7 = (instruction >> 25) & 0x7f;

        uint32_t next_pc = PC + 4; // Default PC increment
        int branch_taken = 0;

        // Preparing disassembly string if logging
        if (log_file && symbols){
            disassemble(current_pc, instruction, disassem_buf, sizeof(disassem_buf), symbols);
        } else if (log_file){
            disassemble(current_pc, instruction, disassem_buf, sizeof(disassem_buf), NULL);
        } else {
            disassem_buf[0] = '\0';
        }

        // Execute instruction
        switch (opcode){
            // R-type
            case 0x33: {
                if (funct7 == 0x01) {
                    switch (funct3) {

                        case 0x0: { // mul
                            int64_t prod = (int64_t)(int32_t)R[rs1] *
                                        (int64_t)(int32_t)R[rs2];
                            R[rd] = (uint32_t)prod;
                            log_reg_write(log_file, rd, R[rd]);
                            break;
                        }

                        case 0x1: { // mulh
                            int64_t prod = (int64_t)(int32_t)R[rs1] *
                                        (int64_t)(int32_t)R[rs2];
                            R[rd] = (uint32_t)((uint64_t)prod >> 32);
                            log_reg_write(log_file, rd, R[rd]);
                            break;
                        }

                        case 0x2: { // mulhsu
                            int64_t a = (int64_t)(int32_t)R[rs1];
                            uint64_t b = (uint64_t)R[rs2];
                            __int128 prod = (__int128)a * (__int128)b;
                            R[rd] = (uint32_t)((uint64_t)prod >> 32);
                            log_reg_write(log_file, rd, R[rd]);
                            break;
                        }

                        case 0x3: { // mulhu
                            uint64_t prod = (uint64_t)R[rs1] * (uint64_t)R[rs2];
                            R[rd] = (uint32_t)(prod >> 32);
                            log_reg_write(log_file, rd, R[rd]);
                            break;
                        }

                        case 0x4: { // div
                            R[rd] = (uint32_t)div_s((int32_t)R[rs1],
                                                    (int32_t)R[rs2]);
                            log_reg_write(log_file, rd, R[rd]);
                            break;
                        }

                        case 0x5: { // divu
                            R[rd] = div_u(R[rs1], R[rs2]);
                            log_reg_write(log_file, rd, R[rd]);
                            break;
                        }

                        case 0x6: { // rem
                            R[rd] = rem_s((int32_t)R[rs1],
                                        (int32_t)R[rs2]);
                            log_reg_write(log_file, rd, R[rd]);
                            break;
                        }

                        case 0x7: { // remu
                            R[rd] = rem_u(R[rs1], R[rs2]);
                            log_reg_write(log_file, rd, R[rd]);
                            break;
                        }
                    }
                    break;
                }

                
                // Standard R-type
                switch (funct3) {

                    case 0x0: { // add / sub
                        if (funct7 == 0x00) {
                            R[rd] = (uint32_t)((int32_t)R[rs1] +
                                            (int32_t)R[rs2]);
                            log_reg_write(log_file, rd, R[rd]);
                        } else if (funct7 == 0x20) {    // sub
                            R[rd] = (uint32_t)((int32_t)R[rs1] -
                                            (int32_t)R[rs2]);
                            log_reg_write(log_file, rd, R[rd]);
                        } else {
                            fprintf(stderr,"Unknown funct7: 0x%x\n",funct7);
                        }
                        break;
                    }

                    case 0x1: { // sll
                        R[rd] = R[rs1] << (R[rs2] & 0x1f);
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }

                    case 0x2: { // slt
                        R[rd] = ((int32_t)R[rs1] < (int32_t)R[rs2]);
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }

                    case 0x3: { // sltu
                        R[rd] = (R[rs1] < R[rs2]);
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }

                    case 0x4: { // xor
                        R[rd] = R[rs1] ^ R[rs2];
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }

                    case 0x5: { // srl / sra
                        if (funct7 == 0x00) {
                            R[rd] = R[rs1] >> (R[rs2] & 0x1f);
                            log_reg_write(log_file, rd, R[rd]);
                        } else if (funct7 == 0x20) {
                            R[rd] = (uint32_t)((int32_t)R[rs1] >>
                                            (R[rs2] & 0x1f));
                            log_reg_write(log_file, rd, R[rd]);
                        }
                        break;
                    }

                    case 0x6: { // or
                        R[rd] = R[rs1] | R[rs2];
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }

                    case 0x7: { // and
                        R[rd] = R[rs1] & R[rs2];
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }
                }

                break;
            }

            // I - type
            case 0x13: {
                int32_t imm = imm_I(instruction);
                switch(funct3){
                    case 0x0: { // addi
                        R[rd] = (uint32_t)((int32_t)R[rs1] + imm);
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }
                    case 0x1: { // slli
                        uint32_t shift_amount = (instruction >> 20) & 0x1f;
                        R[rd] = R[rs1] << shift_amount;
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }
                     case 0x2: { // slti
                         if((int32_t)R[rs1] < imm){
                            R[rd] = 1;
                            log_reg_write(log_file, rd, R[rd]);
                         } else {
                            R[rd] = 0;
                            log_reg_write(log_file, rd, R[rd]);
                         }
                        break;
                    }
                     case 0x3: { // sltiu
                        if(R[rs1] < (uint32_t)imm){
                            R[rd] = 1;
                            log_reg_write(log_file, rd, R[rd]);
                         } else {
                            R[rd] = 0;
                            log_reg_write(log_file, rd, R[rd]);
                         }
                        break;
                    }
                     case 0x4: { // xori
                        R[rd] = R[rs1] ^ (uint32_t)imm;
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }
                     case 0x5: { // srli, srai
                        uint32_t shift_amount = (instruction >> 20) & 0x1f;
                        if (((instruction >> 25) & 0x7f) == 0x00) { 
                            R[rd] = R[rs1] >> shift_amount;
                            log_reg_write(log_file, rd, R[rd]);
                        } else { 
                            R[rd] = (uint32_t)((int32_t)R[rs1] >> shift_amount); // srai
                            log_reg_write(log_file, rd, R[rd]);
                        }
                        break;
                    }
                     case 0x6: { // ori
                        R[rd] = R[rs1] | (uint32_t)imm;
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }
                     case 0x7: { // andi
                        R[rd] = R[rs1] & (uint32_t)imm;
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }
                    default:
                        break;
                }
                break;
                
            }

            // Loads
            case 0x03: {
                int32_t imm = imm_I(instruction);
                uint32_t addr = (uint32_t)((int32_t)R[rs1] + imm);
                switch(funct3) {
                    case 0x0: { // lb
                        int32_t val = (int8_t)memory_rd_b(mem, addr);
                        R[rd] = (uint32_t)val;
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }
                    case 0x1: { // lh
                        int32_t val = (int16_t)memory_rd_h(mem, addr);
                        R[rd] = (uint32_t)val;
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }
                     case 0x2: { // lw
                        uint32_t val = memory_rd_w(mem, addr);
                        R[rd] = val;
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }
                     case 0x4: { // lbu 
                        uint32_t val = (uint32_t)memory_rd_b(mem, addr);
                        R[rd] = val;
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }
                     case 0x5: { // lhu
                        uint32_t val = (uint32_t)memory_rd_h(mem, addr);
                        R[rd] = val;
                        log_reg_write(log_file, rd, R[rd]);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }

            // Stores
            case 0x23: {
                int32_t imm = imm_S(instruction);
                uint32_t addr = (uint32_t)((int32_t)R[rs1] + imm);
                switch (funct3){
                    case 0x0: { // sb
                        uint32_t value = R[rs2] & 0xFF;
                        memory_wr_b(mem, addr, (int)value);
                        log_mem_write(log_file, addr, value);
                        break;
                    }
                    case 0x1: { // sh
                        uint32_t value = R[rs2] & 0xFFFF;
                        memory_wr_h(mem, addr, (int)value);
                        log_mem_write(log_file, addr, value);
                        break;
                    }
                    case 0x2: { // sw
                        uint32_t value = R[rs2];
                        memory_wr_w(mem, addr, (int)value);
                        log_mem_write(log_file, addr, value);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }

            // Branches 
            case 0x63: {
                int32_t imm = imm_B(instruction);
                uint32_t target = (uint32_t)((int32_t)current_pc + imm);
                int take = 0;

                // NT predictor: always predict NOT TAKEN
                stats.nt_predictions++;

                // BTFNT predictor: Backward Taken, Forward Not Taken
                stats.btfnt_predictions++;
                int btfnt_pred_taken = (target < current_pc);

                // Compute actual branch condition
                switch (funct3) {
                    case 0x0: { // beq
                        take = ((int32_t)R[rs1] == (int32_t)R[rs2]);
                        break;
                    }
                    case 0x1: { // bne
                        take = ((int32_t)R[rs1] != (int32_t)R[rs2]);
                        break;
                    }
                    case 0x4: { // blt
                        take = ((int32_t)R[rs1] < (int32_t)R[rs2]);
                        break;
                    }
                    case 0x5: { // bge
                        take = ((int32_t)R[rs1] >= (int32_t)R[rs2]);
                        break;
                    }
                    case 0x6: { // bltu
                        take = (R[rs1] < R[rs2]);
                        break;
                    }
                    case 0x7: { // bgeu
                        take = (R[rs1] >= R[rs2]);
                        break;
                    }
                    default:
                        break;
                }

                if (take) {
                    next_pc = target;
                    branch_taken = 1;
                }

                if (branch_taken) {
                    stats.nt_mispredictions++;
                }

                if (btfnt_pred_taken != branch_taken) {
                    stats.btfnt_mispredictions++;
                }

                break;
            }



            // jal
            case 0x6f: {
                int32_t imm = imm_J(instruction);
                uint32_t target = (uint32_t)((int32_t)current_pc + imm);
                R[rd] = current_pc + 4;
                log_reg_write(log_file, rd, R[rd]);
                next_pc = target;
                break;
            }

            // jalr
            case 0x67: {
                int32_t imm = imm_I(instruction);
                uint32_t t = (uint32_t)(((int32_t)R[rs1] + imm) & ~1u);
                R[rd] = current_pc + 4;
                log_reg_write(log_file, rd, R[rd]);
                next_pc = t;
                break;
            }

            // lui
            case 0x37: {
                int32_t imm = imm_U(instruction);
                R[rd] = (uint32_t)imm;
                log_reg_write(log_file, rd, R[rd]);
                break;
            }

             // auipc
            case 0x17: {
                int32_t imm = imm_U(instruction);
                R[rd] = (uint32_t)((int32_t)current_pc + imm);
                log_reg_write(log_file, rd, R[rd]);
                break;
            }

            // System: ecall
            case 0x73: {
                if (instruction == 0x00000073){
                    uint32_t a7 = R[17];
                    if (a7 == 1) {  // getchar -> A0
                        int c = getchar();
                        if (c == EOF){
                            c = -1;
                        }
                        R[10] = (uint32_t)c;
                    } else if (a7 == 2){ // putchar(A0)
                        putchar(R[10] & 0xff);
                        fflush(stdout);
                    } else if (a7 == 3 || a7 == 93){
                        stop = 1;
                    } else {
                        // Unknown ecall
                        fprintf(stderr, "Unhandled ecall %u at 0x%08x\n", a7, current_pc);
                        stop = 1;
                    }
                } else {
                    // Other system instructions not implemented
                    fprintf(stderr, "Uhandled system instruction 0x%08x at PC=0x%08x\n", instruction, current_pc);
                }
                break;
            }

            default:
                fprintf(stderr, "Unknown opcode: 0x%08x at 0x%08x\n", instruction, current_pc);
                stop = 1;
                break;
        }

        // x0 must remain zero
        R[0] = 0;

        // Logging per-instruction
        if(log_file){
            // Print instruction count, address, raw instruction, disassembler
            log_fmt(log_file, "%6ld => %08x : %08x    %s", instr_count, current_pc, instruction, disassem_buf);
            if (branch_taken) {
                // Log branch is taken
                log_fmt(log_file, " {T}");
            }
            log_fmt(log_file, "\n");
        }

        // Advance PC to next instruction
        PC = next_pc;

        // Save instr_count in stats periodically
        stats.insns = instr_count;
    }

    stats.insns = instr_count;
    return stats;
}

