ifeq ($(CONFIG_ITRACE),)
 SRCS-BLACKLIST-y += $(abspath csrc/utils/disasm.c)
else
# use abspath for verilator's behavior
LIBCAPSTONE = $(abspath tools/capstone/repo/libcapstone.so.5)
CFLAGS += -I$(abspath tools/capstone/repo/include)
$(abspath csrc/utils/disasm.c): $(LIBCAPSTONE)
$(LIBCAPSTONE):
	$(MAKE) -C tools/capstone
endif
