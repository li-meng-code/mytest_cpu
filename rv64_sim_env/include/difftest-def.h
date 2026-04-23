#ifndef __DIFFTEST_DEF_H__
#define __DIFFTEST_DEF_H__

#include <common.h>

/* ISA width detect */
#if defined(CONFIG_XLEN) && (CONFIG_XLEN == 64)
  #define CONFIG_ISA64 1
#else
  #define CONFIG_ISA64 0
#endif

/* difftest direction */
enum {
  DIFFTEST_TO_DUT = 0,
  DIFFTEST_TO_REF = 1,
};

#endif