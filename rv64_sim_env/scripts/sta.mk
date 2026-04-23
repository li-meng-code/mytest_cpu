STA_RTL_FILES ?= $(abspath $(shell find build/sta -name "*.sv"))
STA_SDC_FILE ?= $(abspath scripts/npc.sdc)
STA_DESIGN = ysyx_25090239_npc 
STA_RESULT_DIR ?= $(abspath build/sta-result)
STA_FREQ_MHZ ?= 1000
STA_CLK_PORT_NAME ?= clock
YOSYS_STA_ROOT=/home/xinglin/ysyx/yosys-sta

sta: sta-verilog
	mkdir -p $(STA_RESULT_DIR)
	PATH=/home/xinglin/ysyx/oss-cad-suite-linux-x64-20250806/oss-cad-suite/bin:${PATH} \
	make -C $(YOSYS_STA_ROOT) sta \
		DESIGN=$(STA_DESIGN) SDC_FILE="$(STA_SDC_FILE)" \
		CLK_FREQ_MHZ=$(STA_FREQ_MHZ) CLK_PORT_NAME=$(STA_CLK_PORT_NAME) O=$(STA_RESULT_DIR) \
		RTL_FILES="$(STA_RTL_FILES)"

