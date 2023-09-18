#if !defined(MACHINE_H_)
#define MACHINE_H_

#include <cstdio>
#include "rvemu.h"
#include "ram.h"

struct Machine {
    RAM      ram    ;

    uintx_t  pc     ;
    uintx_t  npc    ;
    uint32_t ir     ;
    uintx_t  reg[32];

    uint8_t  halt   ;
    uint32_t cycle  ;

    // tohost
    char buf[2048];
    uint32_t char_size;

    Machine(const char* memfile);
    ~Machine();

    uint8_t  target_read_uint8 (uintx_t addr);
    uint16_t target_read_uint16(uintx_t addr);
    uint32_t target_read_uint32(uintx_t addr);
    uint64_t target_read_uint64(uintx_t addr);

    void target_write_uint8 (uintx_t addr, uint8_t  data);
    void target_write_uint16(uintx_t addr, uint16_t data);
    void target_write_uint32(uintx_t addr, uint32_t data);
    void target_write_uint64(uintx_t addr, uint64_t data);

    int eval();

    // Debug
    FILE *fp;
    void dump_regs();
};

#endif // MACHINE_H_
