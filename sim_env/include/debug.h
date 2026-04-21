#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <utils.h>
#include <stdio.h>
#include <assert.h>

#define Log(format, ...) \
    _Log(ANSI_FMT("[%s:%d %s] " format, ANSI_FG_YELLOW) "\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif
void npc_cleanup();
#ifdef __cplusplus
}
#endif

#define Assert(cond, fmt, ...) \
  do { \
      if (!(cond)) { \
        fflush(stdout); \
        fprintf(stderr, ANSI_FMT("[%s:%d %s] " fmt, ANSI_FG_RED) "\n", \
        __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        npc_cleanup(); \
        assert(0); \
    } \
  } while(0)

#define panic(format, ...) Assert(0, format, ## __VA_ARGS__)
#define TODO() panic("please implement me")

#endif
