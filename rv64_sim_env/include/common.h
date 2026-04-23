#ifndef __COMMON_H__
#define __COMMON_H__

#include <generated/autoconf.h>

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifndef CONFIG_XLEN
  #if defined(CONFIG_ISA)
    #if defined(CONFIG_ISA_riscv)
      /* Fallback: derive XLEN from CONFIG_ISA string when CONFIG_XLEN is absent */
      #define CONFIG_XLEN 64
    #else
      #define CONFIG_XLEN 32
    #endif
  #else
    #define CONFIG_XLEN 32
  #endif
#endif

#if CONFIG_XLEN == 32

typedef uint32_t word_t;
typedef uint32_t paddr_t;
typedef paddr_t  vaddr_t;

#define FMT_PADDR "0x%08" PRIx32
#define FMT_WORD  "0x%08" PRIx32

#elif CONFIG_XLEN == 64

typedef uint64_t word_t;
typedef uint64_t paddr_t;
typedef paddr_t  vaddr_t;

#define FMT_PADDR "0x%016" PRIx64
#define FMT_WORD  "0x%016" PRIx64

#else
#error "Unsupported CONFIG_XLEN, only 32 or 64 is allowed"
#endif

#include <utils.h>
#include <debug.h>
#include <macro.h>

#endif
