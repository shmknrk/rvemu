#if !defined(RAM_H_)
#define RAM_H_

#include "rvemu.h"

struct RAM {
    uint8_t ram[MEMSIZE];

    // Read
    uint8_t  read_uint8 (uintx_t addr);
    uint16_t read_uint16(uintx_t addr);
    uint32_t read_uint32(uintx_t addr);
    uint64_t read_uint64(uintx_t addr);

    // Write
    void write_uint8 (uintx_t addr, uint8_t  data);
    void write_uint16(uintx_t addr, uint16_t data);
    void write_uint32(uintx_t addr, uint32_t data);
    void write_uint64(uintx_t addr, uint64_t data);

    // Memory init
    void readmem(const char *filename);
};

#endif // RAM_H_
