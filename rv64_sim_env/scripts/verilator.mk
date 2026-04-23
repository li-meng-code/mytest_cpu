TOP_NAME ?= Top
XLEN ?= 64

BUILD_ROOT ?= build
BUILD_DIR ?= $(BUILD_ROOT)/rv$(XLEN)
VSRC_DIR ?= vsrc
CSRC_DIR ?= $(abspath csrc)
VERILATED_DIR ?= $(BUILD_DIR)/obj_dir
WAVE_FILE ?= $(BUILD_DIR)/wave.vcd
LOG_FILE ?= $(BUILD_DIR)/log.txt

## makefile includes
-include $(abspath include/config/auto.conf)
-include $(abspath include/config/auto.conf.cmd)
-include $(abspath scripts/difftest.mk)

# nvboard
ifneq ($(USE_NVBOARD),)
LDFLAGS += $(NPC_NV_ARCHIVE) $(NPC_NV_LDFLAGS)
INCPATH += $(NPC_NV_INCPATH)
CSRC += $(NPC_NV_CSRC)
PREDEFS += __NVBOARD__
endif

remove_quote = $(patsubst "%",%,$(1))

## isa configuration
XLEN ?= 64

ifeq ($(XLEN),64)
GUEST_ISA := riscv64
else
GUEST_ISA := riscv32
endif

## source file collection
FILELIST_MK = $(shell find $(abspath $(CSRC_DIR)) -name "filelist.mk")
-include $(FILELIST_MK)
CSRC += $(abspath $(CSRCS-y))
CSRC += $(abspath $(shell find $(CDIRS-y) -name "*.c" -o -name "*.cpp"))
CSRC := $(filter-out $(SRCS-BLACKLIST-y),$(CSRC))

VFILELIST ?= filelist.f
ifeq ($(wildcard $(VFILELIST)),)
VSRC ?= $(abspath $(shell find $(VSRC_DIR) -name "*.v" -o -name "*.sv"))
else
VSRC := -f $(abspath $(VFILELIST))
endif

VINCPATH +=

## verilator flags

LIBS += -L/home/user04/.local/lib -lcapstone -Wl,-rpath,/home/user04/.local/lib
CONFIG_WAVE_TRACE ?= y
ifeq ($(CONFIG_WAVE_TRACE),y)
VFLAGS += --trace-vcd
PREDEFS += CONFIG_WAVE_TRACE
endif

VFLAGS += --cc -Wall -MMD
VFLAGS += -Wno-UNUSEDSIGNAL -Wno-UNDRIVEN
VFLAGS += --Wno-DECLFILENAME --Wno-VARHIDDEN
VFLAGS += -Wno-IMPORTSTAR
VFLAGS += -Wno-EOFNEWLINE
VFLAGS += -Wno-UNUSEDPARAM
VFLAGS += -Wno-fatal
VFLAGS += $(addprefix -I,$(VINCPATH))
VFLAGS += --autoflush
VFLAGS += -DCONFIG_XLEN=$(XLEN)

ifeq ($(TOP_NAME),ysyxSoCFull)
IGNORED_WARNING = ASSIGNDLY PINCONNECTEMPTY UNUSEDPARAM DEFPARAM SYNCASYNCNET
IGNORED_WARNING := $(addprefix -Wno-,$(IGNORED_WARNING))
VFLAGS += --timescale "1ns/1ns" --no-timing $(IGNORED_WARNING)
endif

## includes
INCPATH += $(abspath include) $(abspath csrc/isa/$(GUEST_ISA)/include)
INCFLAG = $(addprefix -I,$(INCPATH))

## predefs
PREDEFS += __GUEST_ISA__=$(GUEST_ISA) TOP_MODULE=$(TOP_NAME) CONFIG_XLEN=$(XLEN)
ifeq ($(TOP_NAME),ysyxSoCFull)
PREDEFS += IS_YSYX_SOC
endif

CFLAGS += $(addprefix -D,$(PREDEFS))

## libraries
LDFLAGS += $(LIBS)

## compile flags
CFLAGS += -fPIE
CFLAGS += -fmacro-prefix-map=$(NPC_HOME)/csrc=csrc
CFLAGS += -O2 -Wall $(INCFLAG)

## rules
EXECUTABLE = $(BUILD_DIR)/V$(TOP_NAME)

## debug settings
ifeq ($(filter verilator-gdb gdb,$(MAKECMDGOALS)),)
# do nothing
else
VERILATED_DIR = $(BUILD_DIR)/obj_debug_dir
CFLAGS := $(filter-out -O2,$(CFLAGS))
CFLAGS += -O3
EXECUTABLE := $(addsuffix .debug,$(EXECUTABLE))
endif

ARGS ?= --wave=$(WAVE_FILE) --log=$(LOG_FILE)
override ARGS += $(ARGS_DIFF)

TEST ?= rv64ui
IMG ?=
RISCV_TESTS_HOME ?= /home/user04/workspace/our_CPU/share_work/riscv-tests/isa

ifeq ($(IMG),)
IMG_HOME :=
else
IMG_HOME := $(RISCV_TESTS_HOME)/$(TEST)_output/bin/$(IMG)
endif

VERILATOR = verilator

$(EXECUTABLE): verilated

verilated:
	@mkdir -p $(VERILATED_DIR)
	@echo "+CFG XLEN=$(XLEN) GUEST_ISA=$(GUEST_ISA)"
	@echo "+VL $(VSRC) $(CSRC) -> $(EXECUTABLE)"
	@$(VERILATOR) $(VFLAGS) \
		$(addprefix -CFLAGS ,$(CFLAGS)) \
		$(addprefix -LDFLAGS ,$(LDFLAGS)) \
		$(VSRC) --exe $(CSRC) \
		-Mdir $(VERILATED_DIR) --top-module $(TOP_NAME) \
		-o $(abspath $(EXECUTABLE)) --build

veri-clean:
	rm -rf $(VERILATED_DIR) $(BUILD_DIR)

NPC_EXEC := $(EXECUTABLE) $(ARGS) $(IMG_HOME)

verilator-sim-env: $(EXECUTABLE) $(DIFF_REF_SO)

veri-sim: verilator-sim-env
	@echo "running $(NPC_EXEC)"
	@$(NPC_EXEC)

verilator-gdb: verilator-sim-env
	@echo "debugging $(NPC_EXEC)"
	@gdb -s $(EXECUTABLE) --args $(NPC_EXEC)

wave:
	@if test -f $(WAVE_FILE); then verdi -sv -f $(abspath $(VFILELIST)) -ssf $(WAVE_FILE) & else echo "No '$(WAVE_FILE)' found"; fi

log:
	@if test -f $(LOG_FILE); then cat $(LOG_FILE); else echo "No '$(LOG_FILE)' found"; fi

.PHONY: verilated veri-clean veri-sim verilator-gdb wave log