#if !defined(RVEMU_H_)
#define RVEMU_H_

#include <cstdint>

#define TIMEOUT 1000000

#define TRACE_RF
#define TRACE_RF_FILE "trace_rf.txt"

#define RESET_VECTOR 0x00000000
#define MTIME_ADDR   0x20000000
#define TOHOST_ADDR  0x40008000

#define ILEN 32
#define XLEN 32

#if   XLEN == 32
typedef uint32_t uintx_t;
typedef int32_t  intx_t ;
#elif XLEN == 64
typedef uint64_t uintx_t;
typedef int64_t  intx_t ;
#else
#error unsupported XLEN
#endif

#define MEMSIZE (128*1024) // 128 KiB

#endif // RVEMU_H_
