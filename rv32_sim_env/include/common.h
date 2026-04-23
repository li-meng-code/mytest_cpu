#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

typedef uint32_t word_t;
typedef uint32_t paddr_t;
typedef paddr_t vaddr_t;

#include <generated/autoconf.h>
#include <utils.h>
#include <debug.h>
#include <macro.h>

#define FMT_PADDR "0x%08x"
#define FMT_WORD  "0x%08x"

#endif


