#if !defined(RVEMU_H_)
#define RVEMU_H_

//------------------------------------------------------------------------------
#define ILEN 32

#if !defined(XLEN)
#define XLEN 32
#endif

#if !defined(USE_COMPRESSED)
#define USE_COMPRESSED
#endif

//------------------------------------------------------------------------------
#if !defined(TRACE_RF)
//#define TRACE_RF
//#define TRACE_RF_FILE "trace_rf.txt"
#endif

#if !defined(DEBUG)
//#define DEBUG
#endif

#if !defined(TIMEOUT)
#if defined(TRACE_RF)
#define TIMEOUT 100000000
#else
#define TIMEOUT 10000000000
#endif // TRACE_RF
#endif // TIMEOUT

//------------------------------------------------------------------------------
#define MEMSIZE (128*1024) // 128 KiB

#define RESET_VECTOR 0x00000000
#define MTIME_ADDR   0x20000000
#define TOHOST_ADDR  0x40008000

//==============================================================================
#include <cstdint>

#if   XLEN == 32
typedef uint32_t uintx_t;
typedef int32_t  intx_t ;
#elif XLEN == 64
typedef uint64_t uintx_t;
typedef int64_t  intx_t ;
#else
#error unsupported XLEN
#endif

#endif // RVEMU_H_
