#include <cstdio>
#include <cstdlib>
#include "rvemu.h"
#include "machine.h"

int main(int argc, char **argv) {
    if (argc!=2) {
        fprintf(stderr, "Usage: ./rvemu <memfile>\n");
        exit(0);
    }
    const char *memfile = argv[1];

    Machine machine(memfile);
    int halt;

    while (1) {
        halt = machine.eval();
#if defined(TRACE_RF)
        machine.dump_regs();
#endif
        if (halt) {
            printf("\n"                         );
            printf("cycle: %d\n" , machine.cycle);
            break;
        }
    };

    return 0;
}
