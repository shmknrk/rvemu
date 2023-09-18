#include <cstdio>
#include <cstdlib>
#include "machine.h"

Machine::Machine(const char *memfile) {
    ram.readmem(memfile);
    for (int i=0; i<32; i++) {
        reg[i] = 0;
    }
    npc   = RESET_VECTOR;
    cycle = 0;

    // Debug
    if ((fp = fopen(TRACE_RF_FILE, "w"))==NULL) {
        fprintf(stderr, "Error: trace register file cannot be opened.\n");
        exit(0);
    }
}

Machine::~Machine() {
    fclose(fp);
}

#define TARGET_READ_UINT(size) \
uint ## size ## _t Machine::target_read_uint ## size(uintx_t addr) { \
    uint ## size ## _t data; \
    switch (addr) { \
    case MTIME_ADDR: \
        data = 0; \
        break; \
    default: \
        data = ram.read_uint ## size(addr); \
        break; \
    } \
    return data; \
}
TARGET_READ_UINT(8)
TARGET_READ_UINT(16)
TARGET_READ_UINT(32)
TARGET_READ_UINT(64)

#define TARGET_WRITE_UINT(size) \
void Machine::target_write_uint ## size(uintx_t addr, uint ## size ## _t data) { \
    ram.write_uint ## size(addr, data); \
}
TARGET_WRITE_UINT(8)
TARGET_WRITE_UINT(16)
TARGET_WRITE_UINT(64)

void Machine::target_write_uint32(uintx_t addr, uint32_t data) {
    switch (addr) {
    case TOHOST_ADDR:
        if (data & 0x00010000) buf[char_size++] = (data & 0xff);
        if (data & 0x00020000) {
            for (int i=0; i<char_size; i++) {
                printf("%c", buf[i]);
            }
            halt = 1;
        }
        break;
    default:
        ram.write_uint32(addr, data);
        break;
    }
}

int Machine::eval() {
    halt = 0;
    pc   = npc;
    ir   = ram.read_uint32(pc);

    uint8_t opcode = ( ir        & 0x7f); // ir[ 6: 0]
    uint8_t rd     = ((ir >> 7 ) & 0x1f); // ir[11: 7]
    uint8_t funct3 = ((ir >> 12) & 0x7 ); // ir[14:12]
    uint8_t rs1    = ((ir >> 15) & 0x1f); // ir[19:15]
    uint8_t rs2    = ((ir >> 20) & 0x1f); // ir[24:20]
    uint8_t funct7 = ((ir >> 25) & 0x7f); // ir[31:25]

    uintx_t t   ;
    uint8_t cond;
    uintx_t imm ;
    uintx_t addr;
    uintx_t data;

    switch (opcode) {
    case 0b0000011: // load
        imm  = (int32_t)ir >> 20;
        addr = reg[rs1]+imm;
        switch (funct3) {
        case 0b000: // lb
            data = (int8_t)target_read_uint8(addr);
            break;
        case 0b001: // lh
            data = (int16_t)target_read_uint16(addr);
            break;
        case 0b010: // lw
            data = (int32_t)target_read_uint32(addr);
            break;
        case 0b100: // lbu
            data = (uint8_t)target_read_uint8(addr);
            break;
        case 0b101: // lhu
            data = (uint16_t)target_read_uint16(addr);
            break;
        default:
            goto illegal_instr;
            break;
        }
        if (rd!=0) {
            reg[rd] = data;
        }
        npc = pc+4;
        break; // load
    case 0b0100011: // store
        imm  = (((int32_t)ir >> 20) & 0xffffffe0) | ((ir >> 7) & 0x1f);
        addr = reg[rs1]+imm;
        switch (funct3) {
        case 0b000: // sb
            target_write_uint8(addr, reg[rs2]);
            break;
        case 0b001: // sh
            target_write_uint16(addr, reg[rs2]);
            break;
        case 0b010: // sw
            target_write_uint32(addr, reg[rs2]);
            break;
        default:
            goto illegal_instr;
            break;
        }
        npc = pc+4;
        break; // store
    case 0b0010011: // op-imm
        imm = (int32_t)ir >> 20;
        switch (funct3) {
        case 0b000: // addi
            data = reg[rs1] + imm;
            break;
        case 0b001: // slli
            if ((imm & ~(XLEN-1))!=0) {
                goto illegal_instr;
            }
            data = reg[rs1] << (imm & (XLEN - 1));
            break;
        case 0b010: // slti
            data = (intx_t)reg[rs1] < (intx_t)imm;
            break;
        case 0b011: // sltiu
            data = reg[rs1] < (uintx_t)imm;
            break;
        case 0b100: // xori
            data = reg[rs1] ^ imm;
            break;
        case 0b101: // srli/srai
            if ((imm & ~(XLEN-1 | 0x400))!=0) {
                goto illegal_instr;
            }
            if (imm & 0x400) { // srai
                data = (intx_t)reg[rs1] >> (imm & (XLEN - 1));
            } else { // srli
                data = (intx_t)((uintx_t)reg[rs1] >> (imm & (XLEN - 1)));
            }
            break;
        case 0b110: // ori
            data = reg[rs1] | imm;
            break;
        case 0b111: // andi
            data = reg[rs1] & imm;
            break;
        default:
            goto illegal_instr;
            break;
        }
        if (rd!=0) {
            reg[rd] = data;
        }
        npc = pc+4;
        break; // op-imm
    case 0b0110011: // op
        if ((funct7 & ~0x20)!=0) {
            goto illegal_instr;
        }
        switch (funct3) {
        case 0b000: // add/sub
            if (funct7 & 0x20) { // sub
                data = reg[rs1] - reg[rs2];
            } else { // add
                data = reg[rs1] + reg[rs2];
            }
            break;
        case 0b001: // sll
            data = reg[rs1] << (reg[rs2] & (XLEN - 1));
            break;
        case 0b010: // slt
            data = (intx_t)reg[rs1] < (intx_t)reg[rs2];
            break;
        case 0b011: // sltu
            data = reg[rs1] < reg[rs2];
            break;
        case 0b100: // xor
            data = reg[rs1] ^ reg[rs2];
            break;
        case 0b101: // srl/sra
            if (funct7 & 0x20) { // sra
                data = (intx_t)reg[rs1] >> (reg[rs2] & (XLEN - 1));
            } else { // srl
                data = (intx_t)((uintx_t)reg[rs1] >> (reg[rs2] & (XLEN - 1)));
            }
            break;
        case 0b110: // or
            data = reg[rs1] | reg[rs2];
            break;
        case 0b111: // and
            data = reg[rs1] & reg[rs2];
            break;
        default:
            goto illegal_instr;
            break;
        }
        if (rd!=0) {
            reg[rd] = data;
        }
        npc = pc+4;
        break; // op
    case 0b0010111: // auipc
        imm  = (ir & 0xfffff000);
        data = pc + imm;
        if (rd!=0) {
            reg[rd] = data;
        }
        npc = pc+4;
        break; // auipc
    case 0b0110111: // lui
        imm  = (ir & 0xfffff000);
        data = imm;
        if (rd!=0) {
            reg[rd] = data;
        }
        npc = pc+4;
        break; // lui
    case 0b1100011: // branch
        switch (funct3) {
        case 0b000: // beq
            cond = (reg[rs1]==reg[rs2]);
            break;
        case 0b001: // bne
            cond = (reg[rs1]!=reg[rs2]);
            break;
        case 0b100: // blt
            cond = ((intx_t)reg[rs1]<(intx_t)reg[rs2]);
            break;
        case 0b101: // bge
            cond = ((intx_t)reg[rs1]>=(intx_t)reg[rs2]);
            break;
        case 0b110: // bltu
            cond = (reg[rs1]<reg[rs2]);
            break;
        case 0b111: // bgeu
            cond = (reg[rs1]>=reg[rs2]);
            break;
        default:
            goto illegal_instr;
            break;
        }
        if (cond) {
            imm = (((int32_t)ir >> 19) & 0xfffff000) | ((ir << 4) & 0x800) | ((ir >> 20) & 0x7e0) | ((ir >> 7) & 0x1e);
            npc = pc + imm;
        } else {
            npc = pc+4;
        }
        break; // branch
    case 0b1100111: // jalr
        t   = pc+4;
        imm = (int32_t)ir >> 20;
        npc = reg[rs1] + imm;
        if (rd!=0) {
            reg[rd] = t;
        }
        break; // jalr
    case 0b1101111: // jal
        if (rd!=0) {
            reg[rd] = pc+4;
        }
        imm = (((int32_t)ir >> 11) & 0xfff00000) | (ir & 0x000ff000) | ((ir >> 9) & 0x800) | ((ir >> 20) & 0x7fe);
        npc = pc + imm;
        break; // jal
    default:
        goto illegal_instr;
        break;
    }
    cycle++;
    if (cycle>=TIMEOUT) halt = 1;
    return halt;

illegal_instr:
    fprintf(stderr, "Error: illegal instruction detected!!\n");
    fprintf(stderr, "pc=[0x%08x] ir=[0x%08x]\n", pc, ir);
    exit(0);
}

void Machine::dump_regs() {
    fprintf(fp, "%08x %08x %08x\n", cycle, pc, ir);
    for (int i=0; i<4; i++) {
        for (int j=0; j<8; j++) {
            fprintf(fp, "%08x", reg[i*8+j]);
            fprintf(fp, ((j!=7) ? " " : "\n"));
        }
    }
}
