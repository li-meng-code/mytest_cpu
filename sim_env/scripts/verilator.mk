TOP_NAME ?= Top
BUILD_DIR ?= build
VSRC_DIR ?= vsrc
CSRC_DIR ?= $(abspath csrc)
VERILATED_DIR ?= $(BUILD_DIR)/obj_dir
WAVE_FILE = wave.vcd
LOG_FILE = log.txt

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
GUEST_ISA = $(call remove_quote,$(CONFIG_ISA))

## source file collection
FILELIST_MK = $(shell find $(abspath $(CSRC_DIR)) -name "filelist.mk")
-include $(FILELIST_MK)
CSRC += $(abspath $(CSRCS-y))
CSRC += $(abspath $(shell find $(CDIRS-y) -name "*.c" -o -name "*.cpp"))
CSRC := $(filter-out $(SRCS-BLACKLIST-y),$(CSRC))

#VSRC ?= $(abspath $(shell find $(VSRC_DIR) -name "*.v" -o -name "*.sv"))


VFILELIST ?= filelist.f
ifeq ($(wildcard $(VFILELIST)),)
VSRC ?= $(abspath $(shell find $(VSRC_DIR) -name "*.v" -o -name "*.sv"))
else
VSRC := -f $(abspath $(VFILELIST))
endif
VINCPATH +=


## compliance flags
# verilator flags
# wave trace
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
ifeq ($(TOP_NAME),ysyxSoCFull)
IGNORED_WARNING = ASSIGNDLY PINCONNECTEMPTY UNUSEDPARAM DEFPARAM SYNCASYNCNET
IGNORED_WARNING := $(addprefix -Wno-,$(IGNORED_WARNING))
VFLAGS += --timescale "1ns/1ns" --no-timing $(IGNORED_WARNING) # ysyxSoC
endif

# includes
INCPATH += $(abspath include) $(abspath csrc/isa/$(GUEST_ISA)/include)
INCFLAG = $(addprefix -I,$(INCPATH))

# predefs
PREDEFS += __GUEST_ISA__=$(GUEST_ISA) TOP_MODULE=$(TOP_NAME)
ifeq ($(TOP_NAME),ysyxSoCFull)
PREDEFS += IS_YSYX_SOC
endif
CFLAGS += $(addprefix -D,$(PREDEFS))

# libraries
LDFLAGS += $(LIBS)

# sanity check off
# CFLAGS += -fsanitize=address
# LDFLAGS += -fsanitize=address

# pie add
CFLAGS += -fPIE

# turn __FILE__ to a relative path
CFLAGS += -fmacro-prefix-map=$(NPC_HOME)/csrc=csrc

# collecting cflags
CFLAGS += -O2 -Wall $(INCFLAG)

## rules
EXECUTABLE = $(BUILD_DIR)/V$(TOP_NAME)

# debug settings
ifeq ($(filter verilator-gdb gdb, $(MAKECMDGOALS)),)
# do nothing
else
VERILATED_DIR = $(BUILD_DIR)/obj_debug_dir
CFLAGS += -O3
CXXFLAGS= -O3 -march=native -flto -DNDEBUG
CFLAGS := $(filter-out -O2,$(CFLAGS))
EXECUTABLE := $(addsuffix .debug,$(EXECUTABLE))
endif

ARGS ?= --wave=$(WAVE_FILE) --log=$(LOG_FILE)
override ARGS += $(ARGS_DIFF)

IMG ?=

IMG_HOME=/home/user04/workspace/our_CPU/share_work/riscv-tests/isa/rv32ui_output/bin/$(IMG)

VERILATOR = verilator
# Let verilator deal with dependencies itself
$(EXECUTABLE): verilated
verilated:
	@mkdir -p $(VERILATED_DIR)
	@echo "+VL $(VSRC) $(CSRC) -> $(EXECUTABLE)"
	@$(VERILATOR) $(VFLAGS) \
		$(addprefix -CFLAGS ,$(CFLAGS)) \
		$(addprefix -CXXFLAGS ,$(CXXFLAGS)) \
		$(addprefix -LDFLAGS ,$(LDFLAGS)) \
		$(VSRC) --exe $(CSRC) \
		-Mdir $(VERILATED_DIR) --top-module $(TOP_NAME) \
		-o $(abspath $(EXECUTABLE)) --build

veri-clean:
	rm -rf $(VERILATED_DIR) $(BUILD_DIR) $(WAVE_FILE) 

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