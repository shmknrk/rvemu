#===============================================================================
# Config
#-------------------------------------------------------------------------------
XLEN                := 32
#USE_COMPRESSED      := 1

#TRACE_RF            := 1
#TRACE_RF_FILE       := trace_rf.txt
#DEBUG               := 1

#-------------------------------------------------------------------------------
ARCH                := rv$(XLEN)i
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

ifdef USE_COMPRESSED
CXXFLAGS            += -DUSE_COMPRESSED=$(USE_COMPRESSED)
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
	auipc lui

rv32uc_tests        := \
	rvc

.PHONY: isa
isa: rv32ui_test
isa: rv32uc_test

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
	make $1 -C $$(ISA_DIR)

$$($1_tests): %: $1-p-%

endef

$(eval $(call riscv-tests-template,rv32ui))
$(eval $(call riscv-tests-template,rv32uc))

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
	make RISCV_ARCH=$(ARCH) -C $(COREMARK_DIR)

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
	make RISCV_ARCH=$(ARCH) -C $(EMBENCH_DIR)
