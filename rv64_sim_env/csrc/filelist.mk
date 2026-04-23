CSRCS-y += csrc/npc-main.c
CSRCS-y += csrc/hostcall.c

CDIRS-y += csrc/cpu csrc/sim-ctrl csrc/monitor csrc/utils csrc/memory csrc/device csrc/perf

LIBS += -lreadline -ldl -pie
