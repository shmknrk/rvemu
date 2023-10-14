#include <cstdio>
#include <cstdlib>
#include "machine.h"

Machine::Machine(const char *memfile) {
    ram.readmem(memfile);
    for (int i=0; i<32; i++) {
        reg[i] = 0;
    }
    r.pc  = RESET_VECTOR;
    cycle = 0;

    char_size = 0;

#if defined(TRACE_RF)
    if ((fp = fopen(TRACE_RF_FILE, "w"))==NULL) {
        fprintf(stderr, "Error: trace rf file cannot be opened.\n");
        exit(0);
    }
#endif
}

Machine::~Machine() {
#if defined(TRACE_RF)
    fclose(fp);
#endif
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
    cycle++;
    pc   = r.pc;
    ir   = target_read_uint32(r.pc);

    is_compressed = ((ir & 0x3)!=0b11);
    cir           = target_read_uint16(r.pc);
    instr         = "";
    cinstr        = "";

    uint8_t opcode_1_0 =  ir        & 0x3 ; // ir[ 1: 0]
    uint8_t opcode_6_2 = (ir >> 2 ) & 0x1f; // ir[ 6: 2]
    uint8_t rd         = (ir >> 7 ) & 0x1f; // ir[11: 7]
    uint8_t funct3     = (ir >> 12) & 0x7 ; // ir[14:12]
    uint8_t rs1        = (ir >> 15) & 0x1f; // ir[19:15]
    uint8_t rs2        = (ir >> 20) & 0x1f; // ir[24:20]
    uint8_t funct7     = (ir >> 25) & 0x7f; // ir[31:25]

    uint8_t funct2;
    uint8_t funct5;
    uint8_t cond  ;
    uintx_t imm   ;
    uintx_t uimm  ;
    uintx_t addr  ;
    uintx_t data  ;

    switch (opcode_1_0) {
    case 0b00: // Quadrant 0
        rd     = 0x8 | ((ir >> 2 ) & 0x7); // ir[4:2] + 8
        rs1    = 0x8 | ((ir >> 7 ) & 0x7); // ir[9:7] + 8
        rs2    = 0x8 | ((ir >> 2 ) & 0x7); // ir[4:2] + 8
        funct3 =       ((ir >> 13) & 0x7); // ir[15:13]
        switch (funct3) {
        case 0b000: // c.addi4spn
            uimm = ((ir >> 1) & 0x3c0) | ((ir >> 7) & 0x30) | ((ir >> 2) & 0x8) | ((ir >> 4) & 0x4);
            if (uimm==0) {
                goto illegal_instr;
            }
            reg[rd] = reg[2] + uimm;
            ir      = (uimm << 20) | (0x2 << 15) | (rd << 7) | 0b0010011;
            instr   = "addi";
            cinstr  = "c.addi4spn";
            break;
        case 0b010: // c.lw
            uimm    = ((ir << 1) & 0x40) | ((ir >> 7) & 0x38) | ((ir >> 4) & 0x4);
            addr    = reg[rs1] + uimm;
            reg[rd] = (int32_t)target_read_uint32(addr);
            ir      = (uimm << 20) | (rs1 << 15) | (0b010 << 12) | (rd << 7) | 0b0000011;
            instr   = "lw";
            cinstr  = "c.lw";
            break;
        case 0b110: // c.sw
            uimm   = ((ir << 1) & 0x40) | ((ir >> 7) & 0x38) | ((ir >> 4) & 0x4);
            addr   = reg[rs1] + uimm;
            target_write_uint32(addr, reg[rs2]);
            ir     = ((uimm & 0xfe0) << 20) | (rs2 << 20) | (rs1 << 15) | (0b010 << 12) | ((uimm & 0x1f) << 7) | 0b0100011;
            instr  = "sw";
            cinstr = "c.sw";
            break;
        default:
            goto illegal_instr;
            break;
        }
        r.pc = pc+2;
        break; // Quadrant 0
    case 0b01: // Quadrant 1
        funct3 = (ir >> 13) & 0x7; // ir[15:13]
        switch (funct3) {
        case 0b000: // c.nop/c.addi
            if (rd==0) {
                ir      = 0b0010011;
                instr   = "addi";
                cinstr  = "c.nop";
            } else {
                imm     = ((ir >> 7) & 0x20) | ((ir >> 2) & 0x1f);
                imm     = ((int32_t)imm << 26) >> 26; // sext
                imm     = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
                reg[rd] = reg[rd] + imm;
                ir      = (imm << 20) | (rd << 15) | (rd << 7) | 0b0010011;
                instr   = "addi";
                cinstr  = "c.addi";
            }
            r.pc = pc+2;
            break;
        case 0b001: // c.jal
            imm    = ((ir >> 1) & 0xb40) | ((ir << 2) & 0x400) | ((ir << 1) & 0x80) | ((ir << 3) & 0x20) | ((ir >> 7) & 0x10) | ((ir >> 2) & 0xe);
            imm    = ((int32_t)imm << 20) >> 20; // sext
            imm    = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
            reg[1] = pc+2;
            r.pc   = pc + imm;
            ir     = (imm & 0x800ff000) | ((imm & 0x7fe) << 20) | ((imm & 0x800) << 9) | (0x1 << 7) | 0b1101111;
            instr  = "jal";
            cinstr = "c.jal";
            break;
        case 0b010: // c.li
            imm    = ((ir >> 7) & 0x20) | ((ir >> 2) & 0x1f);
            imm    = ((int32_t)imm << 26) >> 26; // sext
            imm    = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
            if (rd!=0) {
                reg[rd] = imm;
            }
            r.pc   = pc+2;
            ir     = (imm << 20) | (rd << 7) | 0b0010011;
            instr  = "addi";
            cinstr = "c.li";
            break;
        case 0b011: // c.addi16sp/c.lui
            if (rd==2) { // c.addi16sp
                imm    = ((ir >> 3) & 0x200) | ((ir << 4) & 0x180) | ((ir << 1) & 0x40) | ((ir << 3) & 0x20) | ((ir >> 2) & 0x10);
                imm    = ((int32_t)imm << 22) >> 22; // sext
                imm    = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
                if (imm==0) {
                    goto illegal_instr;
                }
                reg[2] = reg[2] + imm;
                ir     = (imm << 20) | (0x2 << 15) | (0x2 << 7) | 0b0010011;
                instr  = "addi";
                cinstr = "c.addi16sp";
            } else { // c.lui
                imm    = ((ir << 5) & 0x20000) | ((ir << 10) & 0x1f000);
                imm    = ((int32_t)imm << 14) >> 14; // sext
                imm    = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
                if (imm==0) {
                    goto illegal_instr;
                }
                if (rd!=0) {
                    reg[rd] = imm;
                }
                ir      = imm | (rd << 7) | 0b0110111;
                instr   = "lui";
                cinstr  = "c.lui";
            }
            r.pc = pc+2;
            break;
        case 0b100: // c.misc-alu
            funct2 =       ((ir >> 10) & 0x3); // ir[11:10]
            rd     = 0x8 | ((ir >> 7 ) & 0x7); // ir[9:7] + 8
            rs1    = 0x8 | ((ir >> 7 ) & 0x7); // ir[9:7] + 8
            rs2    = 0x8 | ((ir >> 2 ) & 0x7); // ir[4:2] + 8
            switch (funct2) {
            case 0b00: // c.srli
                uimm    = ((ir >> 7) & 0x20) | ((ir >> 2) & 0x1f);
                reg[rd] = reg[rd] >> uimm;
                ir      = (uimm << 20) | (rs1 << 15) | (0b101 << 12) | (rd << 7) | 0b0010011;
                instr   = "srli";
                cinstr  = "c.srli";
                break;
            case 0b01: // c.srai
                uimm    = ((ir >> 7) & 0x20) | ((ir >> 2) & 0x1f);
                reg[rd] = (intx_t)reg[rd] >> uimm;
                ir      = (0b0100000 << 25) | (uimm << 20) | (rs1 << 15) | (0b101 << 12) | (rd << 7) | 0b0010011;
                instr   = "srai";
                cinstr  = "c.srai";
                break;
            case 0b10: // c.andi
                imm     = ((ir >> 7) & 0x20) | ((ir >> 2) & 0x1f);
                imm     = ((int32_t)imm << 26) >> 26; // sext
                imm     = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
                reg[rd] = reg[rd] & imm;
                ir      = (imm << 20) | (rs1 << 15) | (0b111 << 12) | (rd << 7) | 0b0010011;
                instr   = "andi";
                cinstr  = "c.andi";
                break;
            case 0b11: // c.sub/c.or/c.and
                funct2 = ((ir >> 5) & 0x3); // ir[6:5]
                switch (funct2) {
                case 0b00: // c.sub
                    reg[rd] = reg[rd] - reg[rs2];
                    ir      = (0b0100000 << 25) | (rs2 << 20) | (rs1 << 15) | (0b000 << 12) | (rd << 7) | 0b0110011;
                    instr   = "sub";
                    cinstr  = "c.sub";
                    break;
                case 0b01: // c.xor
                    reg[rd] = reg[rd] ^ reg[rs2];
                    ir      = (rs2 << 20) | (rs1 << 15) | (0b100 << 12) | (rd << 7) | 0b0110011;
                    instr   = "xor";
                    cinstr  = "c.xor";
                    break;
                case 0b10: // c.or
                    reg[rd] = reg[rd] | reg[rs2];
                    ir      = (rs2 << 20) | (rs1 << 15) | (0b110 << 12) | (rd << 7) | 0b0110011;
                    instr   = "or";
                    cinstr  = "c.or";
                    break;
                case 0b11: // c.and
                    reg[rd] = reg[rd] & reg[rs2];
                    ir      = (rs2 << 20) | (rs1 << 15) | (0b111 << 12) | (rd << 7) | 0b0110011;
                    instr   = "and";
                    cinstr  = "c.and";
                    break;
                default:
                    goto illegal_instr;
                    break;
                }
                break;
            default:
                goto illegal_instr;
                break;
            }
            r.pc = pc+2;
            break;
        case 0b101: // c.j
            imm    = ((ir >> 1) & 0xb40) | ((ir << 2) & 0x400) | ((ir << 1) & 0x80) | ((ir << 3) & 0x20) | ((ir >> 7) & 0x10) | ((ir >> 2) & 0xe);
            imm    = ((int32_t)imm << 20) >> 20; // sext
            imm    = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
            r.pc   = pc + imm;
            ir     = (imm & 0x800ff000) | ((imm & 0x7fe) << 20) | ((imm & 0x800) << 9) | 0b1101111;
            instr  = "jal";
            cinstr = "c.j";
            break;
        case 0b110: // c.beqz
            rs1    = 0x8 | ((ir >> 7) & 0x7); // ir[9:7] + 8
            imm    = ((ir >> 4) & 0x100) | ((ir << 1) & 0xc0) | ((ir << 3) & 0x20) | ((ir >> 7) & 0x18) | ((ir >> 2) & 0x6);
            imm    = ((int32_t)imm << 23) >> 23; // sext
            imm    = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
            if (reg[rs1]==0) {
                r.pc = pc + imm;
            } else {
                r.pc = pc+2;
            }
            ir     = ((imm & 0xfe0) << 20) | (rs1 << 15) | ((imm & 0x1e) << 7) | ((imm & 0x100) >> 1) | 0b1100011;
            instr  = "beq";
            cinstr = "c.beqz";
            break;
        case 0b111: // c.bnez
            rs1    = 0x8 | ((ir >> 7) & 0x7); // ir[9:7] + 8
            imm    = ((ir >> 4) & 0x100) | ((ir << 1) & 0xc0) | ((ir << 3) & 0x20) | ((ir >> 7) & 0x18) | ((ir >> 2) & 0x6);
            imm    = ((int32_t)imm << 23) >> 23; // sext
            imm    = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
            if (reg[rs1]!=0) {
                r.pc = pc + imm;
            } else {
                r.pc = pc+2;
            }
            ir     = ((imm & 0xfe0) << 20) | (rs1 << 15) | (0b001 << 12) | ((imm & 0x1e) << 7) | ((imm & 0x100) >> 1) | 0b1100011;
            instr  = "bne";
            cinstr = "c.bnez";
            break;
        default:
            goto illegal_instr;
            break;
        }
        break; // Quadrant 1
    case 0b10: // Quadrant 2
        funct3 = (ir >> 13) & 0x7 ; // ir[15:13]
        rs1    = (ir >>  7) & 0x1f;
        rs2    = (ir >>  2) & 0x1f;
        switch (funct3) {
        case 0b000: // c.slli
            uimm   = ((ir >> 7) & 0x20) | ((ir >> 2) & 0x1f);
            if (rd!=0) {
                reg[rd] = reg[rd] << uimm;
            }
            r.pc   = pc+2;
            ir     = (uimm << 20) | (rd << 15) | (0b001 << 12) | (rd << 7) | 0b0010011;
            instr  = "slli";
            cinstr = "c.slli";
            break;
        case 0b010: // c.lwsp
            uimm   = ((ir << 4) & 0xc0) | ((ir >> 7) & 0x20) | ((ir >> 2) & 0x1c);
            addr   = reg[2] + uimm;
            if (rd!=0) {
                reg[rd] = target_read_uint32(addr);
            }
            r.pc   = pc+2;
            ir     = (uimm << 20) | (0x2 << 15) | (0b010 << 12) | (rd << 7) | 0b0000011;
            instr  = "lw";
            cinstr = "c.lwsp";
            break;
        case 0b100: // c.jr/c.mv/c.jalr/c.add
            if (((ir >> 12) & 0x1)==0) { // c.jr/c.mv
                if (rd==0) {
                    goto illegal_instr;
                }
                if (rs2==0) { // c.jr
                    r.pc    = reg[rs1];
                    ir      = (rs1 << 15) | 0b1100111;
                    instr   = "jalr";
                    cinstr  = "c.jr";
                } else { // c.mv
                    reg[rd] = reg[rs2];
                    r.pc    = pc+2;
                    ir      = (rs2 << 20) | (rd << 7) | (0b0110011);
                    instr   = "add";
                    cinstr  = "c.mv";
                }
            } else { // c.jalr/c.add
                if (rs1==0) { // c.ebreak
                    goto illegal_instr;
                }
                if (rs2==0) { // c.jalr
                    r.pc    = reg[rs1];
                    reg[1]  = pc+2;
                    ir      = (rs1 << 15) | (0x1 << 7) | 0b1100111;
                    instr   = "jalr";
                    cinstr  = "c.jalr";
                } else { // c.add
                    reg[rd] = reg[rd] + reg[rs2];
                    r.pc    = pc+2;
                    ir      = (rs2 << 20) | (rd << 15) | (rd << 7) | (0b0110011);
                    instr   = "add";
                    cinstr  = "c.add";
                }
            }
            break;
        case 0b110: // c.swsp
            uimm   = ((ir >> 1) & 0xc0) | ((ir >> 7) & 0x3c);
            addr   = reg[2] + uimm;
            target_write_uint32(addr, reg[rs2]);
            r.pc   = pc+2;
            ir     = ((uimm & 0xfe0) << 20) | (rs2 << 20) | (0x2 << 15) | (0b010 << 12) | ((uimm & 0x1f) << 7) | 0b0100011;
            instr  = "sw";
            cinstr = "c.swsp";
            break;
        default:
            goto illegal_instr;
            break;
        }
        break; // Quadrant 2
    case 0b11:
        switch (opcode_6_2) {
        case 0b00000: // load
            imm  = (int32_t)ir >> 20; // sext
            imm  = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
            addr = reg[rs1]+imm;
            switch (funct3) {
            case 0b000: // lb
                data  = (int8_t)target_read_uint8(addr);
                instr = "lb";
                break;
            case 0b001: // lh
                data  = (int16_t)target_read_uint16(addr);
                instr = "lh";
                break;
            case 0b010: // lw
                data  = (int32_t)target_read_uint32(addr);
                instr = "lw";
                break;
            case 0b011: // ld
                data  = (int64_t)target_read_uint64(addr);
                instr = "ld";
                break;
            case 0b100: // lbu
                data  = (uint8_t)target_read_uint8(addr);
                instr = "lbu";
                break;
            case 0b101: // lhu
                data  = (uint16_t)target_read_uint16(addr);
                instr = "lhu";
                break;
            case 0b110: // lwu
                data  = (uint32_t)target_read_uint32(addr);
                instr = "lwu";
                break;
            default:
                goto illegal_instr;
                break;
            }
            if (rd!=0) {
                reg[rd] = data;
            }
            r.pc = pc+4;
            break; // load
        case 0b01000: // store
            imm  = (((int32_t)ir >> 20) & 0xffffffe0) | ((ir >> 7) & 0x1f);
            imm  = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
            addr = reg[rs1]+imm;
            switch (funct3) {
            case 0b000: // sb
                target_write_uint8(addr, reg[rs2]);
                instr = "sb";
                break;
            case 0b001: // sh
                target_write_uint16(addr, reg[rs2]);
                instr = "sh";
                break;
            case 0b010: // sw
                target_write_uint32(addr, reg[rs2]);
                instr = "sw";
                break;
            case 0b011: // sd
                target_write_uint64(addr, reg[rs2]);
                instr = "sd";
                break;
            default:
                goto illegal_instr;
                break;
            }
            r.pc = pc+4;
            break; // store
        case 0b00100: // op-imm
            imm = (int32_t)ir >> 20;
            imm = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
            switch (funct3) {
            case 0b000: // addi
                data  = reg[rs1] + imm;
                instr = "addi";
                break;
            case 0b001: // slli
                if ((imm & ~(XLEN-1))!=0) {
                    goto illegal_instr;
                }
                data  = reg[rs1] << (imm & (XLEN-1));
                instr = "slli";
                break;
            case 0b010: // slti
                data  = (intx_t)reg[rs1] < (intx_t)imm;
                instr = "slti";
                break;
            case 0b011: // sltiu
                data  = reg[rs1] < (uintx_t)imm;
                instr = "sltiu";
                break;
            case 0b100: // xori
                data  = reg[rs1] ^ imm;
                instr = "xori";
                break;
            case 0b101: // srli/srai
                if ((imm & ~(XLEN-1 | 0x400))!=0) {
                    goto illegal_instr;
                }
                if (imm & 0x400) { // srai
                    data  = (intx_t)reg[rs1] >> (imm & (XLEN-1));
                    instr = "srai";
                } else { // srli
                    data  = (intx_t)((uintx_t)reg[rs1] >> (imm & (XLEN-1)));
                    instr = "srli";
                }
                break;
            case 0b110: // ori
                data  = reg[rs1] | imm;
                instr = "ori";
                break;
            case 0b111: // andi
                data  = reg[rs1] & imm;
                instr = "andi";
                break;
            default:
                goto illegal_instr;
                break;
            }
            if (rd!=0) {
                reg[rd] = data;
            }
            r.pc = pc+4;
            break; // op-imm
        case 0b00110: // op-imm-32
            imm = (int32_t)ir >> 20;
            switch (funct3) {
            case 0b000: // addiw
                imm   = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
                data  = (uint32_t)(reg[rs1] + imm);
                data  = ((intx_t)data << (XLEN-32)) >> (XLEN-32); // sext
                instr = "addiw";
                break;
            case 0b001: // slliw
                if ((imm & ~0x1f)!=0) {
                    goto illegal_instr;
                }
                data  = (uint32_t)(reg[rs1] << (imm & 0x1f));
                data  = ((intx_t)data << (XLEN-32)) >> (XLEN-32); // sext
                instr = "slliw";
                break;
            case 0b101: // srliw/sraiw
                if ((imm & ~0x41f)!=0) {
                    goto illegal_instr;
                }
                if (imm & 0x400) { // sraiw
                    data  = (int32_t)reg[rs1] >> (imm & 0x1f);
                    instr = "sraiw";
                } else { // srliw
                    data  = (uint32_t)reg[rs1] >> (imm & 0x1f);
                    instr = "srliw";
                }
                data = ((intx_t)data << (XLEN-32)) >> (XLEN-32); // sext
                break;
            default:
                goto illegal_instr;
                break;
            }
            if (rd!=0) {
                reg[rd] = data;
            }
            r.pc = pc+4;
            break;
        case 0b01100: // op
            if ((funct7 & ~0x21)!=0) {
                goto illegal_instr;
            }
            if (funct7 & 0x01) {
                switch (funct3) {
                case 0b000: // mul
                    data  = (uintx_t)((intx_t)reg[rs1] * (intx_t)reg[rs2]);
                    instr = "mul";
                    break;
                case 0b001: // mulh
                    data  = (uintx_t)(((int2x_t)(intx_t)reg[rs1] * (int2x_t)(intx_t)reg[rs2]) >> XLEN);
                    instr = "mulh";
                    break;
                case 0b010: // mulhsu
                    data  = (uintx_t)(((int2x_t)(intx_t)reg[rs1] * (int2x_t)(uintx_t)reg[rs2]) >> XLEN);
                    instr = "mulhsu";
                    break;
                case 0b011: // mulhu
                    data  = (uintx_t)(((int2x_t)(uintx_t)reg[rs1] * (int2x_t)(uintx_t)reg[rs2]) >> XLEN);
                    instr = "mulhu";
                    break;
                case 0b100: // div
                    if (reg[rs2]==0) {
                        data = -1;
                    } else if ((reg[rs1]==((uintx_t)1 << (XLEN-1))) && ((intx_t)reg[rs2]==-1)) {
                        data = reg[rs1];
                    } else {
                        data = (intx_t)reg[rs1] / (intx_t)reg[rs2];
                    }
                    instr = "div";
                    break;
                case 0b101: // divu
                    if (reg[rs2]==0) {
                        data = -1;
                    } else {
                        data = reg[rs1] / reg[rs2];
                    }
                    instr = "divu";
                    break;
                case 0b110: // rem
                    if (reg[rs2]==0) {
                        data = reg[rs1];
                    } else if ((reg[rs1]==((uintx_t)1 << (XLEN-1))) && ((intx_t)reg[rs2]==-1)) {
                        data = 0;
                    } else {
                        data = (intx_t)reg[rs1] % (intx_t)reg[rs2];
                    }
                    instr = "rem";
                    break;
                case 0b111: // remu
                    if (reg[rs2]==0) {
                        data = reg[rs1];
                    } else {
                        data = reg[rs1] % reg[rs2];
                    }
                    instr = "remu";
                    break;
                default:
                    goto illegal_instr;
                    break;
                }
            } else {
                switch (funct3) {
                case 0b000: // add/sub
                    if (funct7 & 0x20) { // sub
                        data  = reg[rs1] - reg[rs2];
                        instr = "sub";
                    } else { // add
                        data  = reg[rs1] + reg[rs2];
                        instr = "add";
                    }
                    break;
                case 0b001: // sll
                    data  = reg[rs1] << (reg[rs2] & (XLEN-1));
                    instr = "sll";
                    break;
                case 0b010: // slt
                    data  = (intx_t)reg[rs1] < (intx_t)reg[rs2];
                    instr = "slt";
                    break;
                case 0b011: // sltu
                    data  = reg[rs1] < reg[rs2];
                    instr = "sltu";
                    break;
                case 0b100: // xor
                    data  = reg[rs1] ^ reg[rs2];
                    instr = "xor";
                    break;
                case 0b101: // srl/sra
                    if (funct7 & 0x20) { // sra
                        data  = (intx_t)reg[rs1] >> (reg[rs2] & (XLEN-1));
                        instr = "sra";
                    } else { // srl
                        data  = reg[rs1] >> (reg[rs2] & (XLEN-1));
                        instr = "srl";
                    }
                    break;
                case 0b110: // or
                    data  = reg[rs1] | reg[rs2];
                    instr = "or";
                    break;
                case 0b111: // and
                    data  = reg[rs1] & reg[rs2];
                    instr = "and";
                    break;
                default:
                    goto illegal_instr;
                    break;
                }
            }
            if (rd!=0) {
                reg[rd] = data;
            }
            r.pc = pc+4;
            break; // op
        case 0b01110: // op-32
            if ((funct7 & ~0x21)!=0) {
                goto illegal_instr;
            }
            if (funct7 & 0x01) {
                switch (funct3) {
                case 0b000: // mulw
                    data  = (uint32_t)((intx_t)reg[rs1] * (intx_t)reg[rs2]);
                    data  = ((intx_t)data << (XLEN-32)) >> (XLEN-32); // sext
                    instr = "mulw";
                    break;
                case 0b100: // divw
                    if ((int32_t)reg[rs2]==0) {
                        data = -1;
                    } else if (((int32_t)reg[rs1]==((int32_t)1 << 31)) && ((int32_t)reg[rs2]==-1)) {
                        data = (int32_t)reg[rs1];
                    } else {
                        data = (int32_t)reg[rs1] / (int32_t)reg[rs2];
                        data = ((intx_t)data << (XLEN-32)) >> (XLEN-32);
                    }
                    instr = "divw";
                    break;
                case 0b101: // divuw
                    if ((uint32_t)reg[rs2]==0) {
                        data = -1;
                    } else {
                        data = (uint32_t)reg[rs1] / (uint32_t)reg[rs2];
                        data = ((intx_t)data << (XLEN-32)) >> (XLEN-32);
                    }
                    instr = "divuw";
                    break;
                case 0b110: // remw
                    if ((int32_t)reg[rs2]==0) {
                        data = reg[rs1];
                    } else if (((int32_t)reg[rs1]==((int32_t)1 << 31)) && ((int32_t)reg[rs2]==-1)) {
                        data = 0;
                    } else {
                        data = (int32_t)reg[rs1] % (int32_t)reg[rs2];
                        data = ((intx_t)data << (XLEN-32)) >> (XLEN-32);
                    }
                    instr = "remw";
                    break;
                case 0b111: // remuw
                    if ((uint32_t)reg[rs2]==0) {
                        data = reg[rs1];
                    } else {
                        data = (uint32_t)reg[rs1] % (uint32_t)reg[rs2];
                        data = ((intx_t)data << (XLEN-32)) >> (XLEN-32);
                    }
                    instr = "remuw";
                    break;
                default:
                    goto illegal_instr;
                    break;
                }
            } else {
                switch (funct3) {
                case 0b000: // addw/subw
                    if (funct7 & 0x20) { // subw
                        data  = (int32_t)(reg[rs1] - reg[rs2]);
                        instr = "sub";
                    } else { // addw
                        data  = (int32_t)(reg[rs1] + reg[rs2]);
                        instr = "add";
                    }
                    break;
                case 0b001: // sllw
                    data  = (uint32_t)(reg[rs1] << (reg[rs2] & 0x1f));
                    data  = ((intx_t)data << (XLEN-32)) >> (XLEN-32); // sext
                    instr = "sllw";
                    break;
                case 0b101: // srlw/sraw
                    if (funct7 & 0x20) { // sraw
                        data  = (int32_t)reg[rs1] >> (reg[rs2] & 0x1f);
                        instr = "sraw";
                    } else { // srlw
                        data  = (uint32_t)reg[rs1] >> (reg[rs2] & 0x1f);
                        instr = "srlw";
                    }
                    data = ((intx_t)data << (XLEN-32)) >> (XLEN-32); // sext
                    break;
                default:
                    goto illegal_instr;
                    break;
                }
            }
            if (rd!=0) {
                reg[rd] = data;
            }
            r.pc = pc+4;
            break; // op-32
        case 0b00101: // auipc
            imm  = (ir & 0xfffff000);
            imm  = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
            data = pc + imm;
            if (rd!=0) {
                reg[rd] = data;
            }
            r.pc  = pc+4;
            instr = "auipc";
            break; // auipc
        case 0b01101: // lui
            imm  = (ir & 0xfffff000);
            imm  = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
            data = imm;
            if (rd!=0) {
                reg[rd] = data;
            }
            r.pc  = pc+4;
            instr = "lui";
            break; // lui
        case 0b11000: // branch
            switch (funct3) {
            case 0b000: // beq
                cond  = (reg[rs1]==reg[rs2]);
                instr = "beq";
                break;
            case 0b001: // bne
                cond  = (reg[rs1]!=reg[rs2]);
                instr = "bne";
                break;
            case 0b100: // blt
                cond  = ((intx_t)reg[rs1]<(intx_t)reg[rs2]);
                instr = "blt";
                break;
            case 0b101: // bge
                cond  = ((intx_t)reg[rs1]>=(intx_t)reg[rs2]);
                instr = "bne";
                break;
            case 0b110: // bltu
                cond  = (reg[rs1]<reg[rs2]);
                instr = "bltu";
                break;
            case 0b111: // bgeu
                cond  = (reg[rs1]>=reg[rs2]);
                instr = "bgeu";
                break;
            default:
                goto illegal_instr;
                break;
            }
            if (cond) {
                imm  = (((int32_t)ir >> 19) & 0xfffff000) | ((ir << 4) & 0x800) | ((ir >> 20) & 0x7e0) | ((ir >> 7) & 0x1e);
                imm  = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
                r.pc = pc + imm;
            } else {
                r.pc = pc+4;
            }
            break; // branch
        case 0b11001: // jalr
            imm   = (int32_t)ir >> 20;
            imm   = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
            r.pc  = reg[rs1] + imm;
            if (rd!=0) {
                reg[rd] = pc+4;
            }
            instr = "jalr";
            break; // jalr
        case 0b11011: // jal
            if (rd!=0) {
                reg[rd] = pc+4;
            }
            imm   = (((int32_t)ir >> 11) & 0xfff00000) | (ir & 0x000ff000) | ((ir >> 9) & 0x800) | ((ir >> 20) & 0x7fe);
            imm   = ((intx_t)imm << (XLEN-32)) >> (XLEN-32); // sext
            r.pc  = pc + imm;
            instr = "jal";
            break; // jal
        case 0b00011: // misc-mem
            switch (funct3) {
            case 0b000: // fence
                break;
            case 0b001: // fence.i
                break;
            default:
                goto illegal_instr;
                break;
            }
            r.pc = pc+4;
            break;
        case 0b01011: // amo
                funct5 = ((ir >> 27) & 0x1f); // ir[31:27]
                addr   = reg[rs1];
                switch (funct3) {
                case 0b010: // amo.w
                    data = target_read_uint32(addr);
                    switch (funct5) {
                    case 0b00010: // lr.w
                        if (rs2!=0) {
                            goto illegal_instr;
                        }
                        load_res_addr = addr;
                        instr = "lr.w";
                        break;
                    case 0b00011: // sc.w
                        if (addr==load_res_addr) {
                            target_write_uint32(addr, reg[rs2]);
                            load_res_addr = (uintx_t)-1;
                            data = 0;
                        } else {
                            data = 1;
                        }
                        instr = "sc.w";
                        break;
                    case 0b00001: // amoswap.w
                        target_write_uint32(addr,        reg[rs2]);
                        instr = "amoswap.w";
                        break;
                    case 0b00000: // amoadd.w
                        target_write_uint32(addr, data + reg[rs2]);
                        instr = "amoadd.w";
                        break;
                    case 0b00100: // amoxor.w
                        target_write_uint32(addr, data ^ reg[rs2]);
                        instr = "amoxor.w";
                        break;
                    case 0b01100: // amoand.w
                        target_write_uint32(addr, data & reg[rs2]);
                        instr = "amoand.w";
                        break;
                    case 0b01000: // amoor.w
                        target_write_uint32(addr, data | reg[rs2]);
                        instr = "amoor.w";
                        break;
                    case 0b10000: // amomin.w
                        target_write_uint32(addr, ((intx_t)data < (intx_t)reg[rs2]) ? data : reg[rs2]);
                        instr = "amomin.w";
                        break;
                    case 0b10100: // amomax.w
                        target_write_uint32(addr, ((intx_t)data > (intx_t)reg[rs2]) ? data : reg[rs2]);
                        instr = "amomax.w";
                        break;
                    case 0b11000: // amominu.w
                        target_write_uint32(addr, ((uintx_t)data < (uintx_t)reg[rs2]) ? data : reg[rs2]);
                        instr = "amominu.w";
                        break;
                    case 0b11100: // amomaxu.w
                        target_write_uint32(addr, ((uintx_t)data > (uintx_t)reg[rs2]) ? data : reg[rs2]);
                        instr = "amomaxu.w";
                        break;
                    default:
                        goto illegal_instr;
                        break;
                    }
                    break;
                default:
                    goto illegal_instr;
                    break;
                }
                if (rd!=0) {
                    reg[rd] = (intx_t)data;
                }
                r.pc = pc+4;
            break;
        default:
            goto illegal_instr;
            break;
        }
        break;
    default:
        goto illegal_instr;
        break;
    }
    if (cycle>=TIMEOUT) halt = 1;
    return halt;

illegal_instr:
    fprintf(stderr, "Error: illegal instruction detected!!\n");
#if      XLEN == 32
    fprintf(stderr, "pc=[0x%08x] ir=[0x%08x]\n", pc, ir);
#else // XLEN == 64
    fprintf(stderr, "pc=[0x%016lx] ir=[0x%08x]\n", pc, ir);
#endif
    exit(0);
}

#if defined(TRACE_RF)
void Machine::dump_regs() {
#if      XLEN == 32
    fprintf(fp, "%08d %08x %08x", cycle, pc, ir);
#else // XLEN == 64
    fprintf(fp, "%08d %016lx %08x", cycle, pc, ir);
#endif
#if defined(DEBUG)
    fprintf(fp, " %17s", instr);
    if (is_compressed) {
        fprintf(fp, "     %04x %17s", cir, cinstr);
    }
#endif
    fprintf(fp, "\n");
    for (int i=0; i<4; i++) {
        for (int j=0; j<8; j++) {
#if      XLEN == 32
            fprintf(fp, "%08x", reg[i*8+j]);
#else // XLEN == 64
            fprintf(fp, "%016lx", reg[i*8+j]);
#endif
            fprintf(fp, ((j!=7) ? " " : "\n"));
        }
    }
}
#endif
