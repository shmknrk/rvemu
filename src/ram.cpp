#include <cstdio>
#include <cstdlib>
#include "ram.h"

#define READ_UINT(size) \
uint ## size ## _t RAM::read_uint ## size(uintx_t addr) { \
    if (addr>(MEMSIZE-size/8)) { \
        fprintf(stderr, "Error: ram read address (0x%08lx) is out of range. (read_uint" #size ")\n", (uint64_t)addr); \
        exit(0); \
    } \
    uint ## size ## _t *data = (uint ## size ## _t *)&ram[addr]; \
    return *data; \
}
READ_UINT(8)
READ_UINT(16)
READ_UINT(32)
READ_UINT(64)

#define WRITE_UINT(size) \
void RAM::write_uint ## size(uintx_t addr, uint ## size ## _t data) { \
    if (addr>(MEMSIZE-size/8)) { \
        fprintf(stderr, "Error: ram write address (0x%08lx) is out of range. (write_uint" #size ")\n", (uint64_t)addr); \
        exit(0); \
    } \
    uint ## size ##_t *p = (uint ## size ## _t *)&ram[addr]; \
    *p = data; \
}
WRITE_UINT(8)
WRITE_UINT(16)
WRITE_UINT(32)
WRITE_UINT(64)

void RAM::readmem(const char *filename) {
    FILE *fp;

    // open
    if ((fp = fopen(filename, "rb"))==NULL) {
        fprintf(stderr, "Error: memfile (%s) cannot be found.\n", filename);
        exit(0);
    }

    // read binfile and write ram
    uintx_t  addr = 0;
    uint32_t data;
    while (fread(&data, sizeof(uint32_t), 1, fp)!=0) {
        write_uint32(addr, data);
        addr=addr+4;
    }

    // close
    fclose(fp);
}
