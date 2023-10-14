#===============================================================================
# Config
#-------------------------------------------------------------------------------
#XLEN                := 32
XLEN                := 64
USE_MULDIV          := 1
#USE_ATOMIC          := 1
#USE_COMPRESSED      := 1

#TRACE_RF            := 1
#TRACE_RF_FILE       := trace_rf.txt
#DEBUG               := 1
#-----------------------------------------------------------------------------------#
# Format of TRACE_RF_FILE                                                           #
#-----------------------------------------------------------------------------------#
# count pc ir                             | count pc ir                             #
# zero  ra   sp   gp   tp   t0   t1   t2  |  x0   x1   x2   x3   x4   x5   x6   x7  #
#  s0   s1   a0   a1   a2   a3   a4   a5  |  x8   x9   x10  x11  x12  x13  x14  x15 #
#  a6   a7   s2   s3   s4   s5   s6   s7  |  x16  x17  x18  x19  x20  x21  x22  x23 #
#  s8   s9   s10  s11  t3   t4   t5   t6  |  x24  x25  x26  x27  x28  x29  x30  x31 #
#-----------------------------------------------------------------------------------#

#-------------------------------------------------------------------------------
ARCH                := rv$(XLEN)i
ifdef USE_MULDIV
ARCH                := $(addsuffix m, $(ARCH))
endif
ifdef USE_ATOMIC
ARCH                := $(addsuffix a, $(ARCH))
endif
ifdef USE_COMPRESSED
ARCH                := $(addsuffix c, $(ARCH))
endif

TARGET              := rvemu

#===============================================================================
# Sources
#-------------------------------------------------------------------------------
source-to-object     = $(subst .cpp,.o, $(filter %.cpp, $1))

SRC_DIR             := src
SRCS                += $(wildcard $(SRC_DIR)/*.cpp)
OBJS                := $(call source-to-object, $(SRCS))
INC_DIR             += $(SRC_DIR)

PROG_DIR            := prog
ISA_DIR             := $(PROG_DIR)/riscv-tests
COREMARK_DIR        := $(PROG_DIR)/coremark
EMBENCH_DIR         := $(PROG_DIR)/embench-iot

#===============================================================================
# g++
#-------------------------------------------------------------------------------
CPPFLAGS            += $(addprefix -I,$(INC_DIR))

CXXFLAGS            += -O2
CXXFLAGS            += -MD

ifdef XLEN
CXXFLAGS            += -DXLEN=$(XLEN)
endif

ifdef TRACE_RF
TRACE_RF_FILE       ?= trace_rf.txt
CXXFLAGS            += -DTRACE_RF
CXXFLAGS            += -DTRACE_RF_FILE=\"$(TRACE_RF_FILE)\"
endif

ifdef DEBUG
CXXFLAGS            += -DDEBUG
endif

#===============================================================================
# Build rules
#-------------------------------------------------------------------------------
.PHONY: default program
default: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ -o $@

.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

#-------------------------------------------------------------------------------
.PHONY: clean program_clean distclean
clean:
	rm -f $(SRC_DIR)/*.o
	rm -f $(SRC_DIR)/*.d
	rm -f a.out
	rm -f $(TARGET)
	rm -f *.txt

program_clean:
	@make distclean -C prog/riscv-tests --no-print-directory
	@make distclean -C prog/coremark --no-print-directory
	@make distclean -C prog/embench-iot --no-print-directory

distclean: clean program_clean

-include $(SRC_DIR)/*.d

#===============================================================================
# riscv-tests/isa
#-------------------------------------------------------------------------------
rv32ui_tests        := \
	simple \
	add addi sub and andi or ori xor xori \
	sll slli srl srli sra srai slt slti sltiu sltu \
	beq bge bgeu blt bltu bne jal jalr \
	sb sh sw lb lbu lh lhu lw \
	auipc lui \
	fence_i ma_data

rv32um_tests        := \
	mul mulh mulhsu mulhu \
	div divu \
	rem remu

rv32ua_tests        := \
	amoadd_w amoand_w amomax_w amomaxu_w amomin_w amominu_w amoor_w amoxor_w amoswap_w \
	lrsc

rv32uc_tests        := \
	rvc

rv64ui_tests        := \
	simple \
	add addi addiw addw sub subw and andi or ori xor xori \
	sll slli slliw sllw srl srli srliw srlw sra srai sraiw sraw slt slti sltiu sltu \
	beq bge bgeu blt bltu bne jal jalr \
	lb lbu lh lhu lw lwu ld sb sh sw sd \
	lui auipc \
	fence_i ma_data

rv64um_tests        := \
	mul mulh mulhsu mulhu mulw \
	div divu divuw divw \
	rem remu remuw remw

.PHONY: isa
ifeq      ($(XLEN),32)
isa: rv32ui_test
isa: rv32um_test
isa: rv32ua_test
isa: rv32uc_test
else ifeq ($(XLEN),64)
isa: rv64ui_test
isa: rv64um_test
else
$(error Error: Unsupported XLEN.)
endif

# $(eval $(call riscv-tests-template,TVM))
define riscv-tests-template

$1_p_tests := $$(addprefix $1-p-, $$($1_tests))

.PHONY: $1_test $$($1_tests) $$($1_p_tests) 
$1_test: $$($1_p_tests)
$$($1_p_tests): $$(TARGET) $$(ISA_DIR)/$1
	@echo -------------------------------------------------------------------------------
	@echo $$@
	@echo
	@./$$(TARGET) $$(ISA_DIR)/$1/$$@.bin
	@echo

$$(ISA_DIR)/$1:
	make XLEN=$(XLEN) $1 -C $$(ISA_DIR)

$$($1_tests): %: $1-p-%

endef

ifeq      ($(XLEN),32)
$(eval $(call riscv-tests-template,rv32ui))
$(eval $(call riscv-tests-template,rv32um))
$(eval $(call riscv-tests-template,rv32ua))
$(eval $(call riscv-tests-template,rv32uc))
else ifeq ($(XLEN),64)
$(eval $(call riscv-tests-template,rv64ui))
$(eval $(call riscv-tests-template,rv64um))
else
$(error Error: Unsupported XLEN.)
endif

#===============================================================================
# CoreMark
#-------------------------------------------------------------------------------
coremark: $(TARGET) $(COREMARK_DIR)/$(ARCH)
	@echo -------------------------------------------------------------------------------
	@echo $@
	@echo
	@./$(TARGET) $(COREMARK_DIR)/$(ARCH)/$@.bin
	@echo

$(COREMARK_DIR)/$(ARCH):
	make XLEN=$(XLEN) RISCV_ARCH=$(ARCH) -C $(COREMARK_DIR)

#===============================================================================
# Embench
#-------------------------------------------------------------------------------
embench             := \
	aha-mont64 crc32 cubic edn huffbench matmult-int md5sum minver nbody \
	nettle-aes nettle-sha256 nsichneu picojpeg primecount qrduino \
	sglib-combined slre st statemate tarfind ud wikisort

embench: $(embench)
$(embench): $(TARGET) $(EMBENCH_DIR)/$(ARCH)
	@echo -------------------------------------------------------------------------------
	@echo $@
	@echo
	@./$(TARGET) $(EMBENCH_DIR)/$(ARCH)/$@.bin
	@echo

$(EMBENCH_DIR)/$(ARCH):
	make XLEN=$(XLEN) RISCV_ARCH=$(ARCH) -C $(EMBENCH_DIR)
