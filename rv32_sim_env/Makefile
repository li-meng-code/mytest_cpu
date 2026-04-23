VERILOG_DIR ?= vsrc/generated
STA_DIR ?= build/sta

PRJ =	npc

all: verilated

clean: verilator-clean
	rm -rf $(VERILOG_DIR)

sim: verilator-sim
# 	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!

run: sim

gdb: verilator-gdb

.PHONY: test verilog help reformat checkformat clean sim run

include scripts/verilator.mk
include scripts/config.mk

include scripts/sta.mk

-include ../Makefile
