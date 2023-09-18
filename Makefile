#===============================================================================
# Sources
#-------------------------------------------------------------------------------
source-to-object     = $(subst .cpp,.o,$(filter %.cpp, $1))

SRC_DIR             := src
SRCS                += $(wildcard $(SRC_DIR)/*.cpp)
OBJS                := $(call source-to-object, $(SRCS))
INC_DIR             += $(SRC_DIR)

PROG_DIR            := prog
ISA_DIR             := $(PROG_DIR)/riscv-tests
COREMARK_DIR        := $(PROG_DIR)/coremark

#===============================================================================
# g++
#-------------------------------------------------------------------------------
CPPFLAGS            += $(addprefix -I,$(INC_DIR))

CXXFLAGS            += -O2
CXXFLAGS            += -MD

#===============================================================================
# Build rules
#-------------------------------------------------------------------------------
.PHONY: default clean
default: rvemu

rvemu: $(OBJS)
	$(CXX) $^ -o $@

clean:
	rm -f $(SRC_DIR)/*.o
	rm -f $(SRC_DIR)/*.d
	rm -f a.out
	rm -f rvemu
	rm -f *.txt

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

-include $(SRC_DIR)/*.d

#===============================================================================
# riscv-tests/isa
#-------------------------------------------------------------------------------
rv32ui_tests := \
	simple \
	add addi sub and andi or ori xor xori \
	sll slli srl srli sra srai slt slti sltiu sltu \
	beq bge bgeu blt bltu bne jal jalr \
	sb sh sw lb lbu lh lhu lw \
	auipc lui

rv32ui_p_tests := $(addprefix rv32ui-p-, $(rv32ui_tests))

.PHONY: isa
isa: rv32ui_test

.PHONY:rv32ui_test $(rv32ui_tests) $(rv32ui_p_tests) 
rv32ui_test: $(rv32ui_p_tests)
$(rv32ui_p_tests): rvemu
	@echo -------------------------------------------------------------------------------
	@echo $@
	@./rvemu $(ISA_DIR)/$@.bin
	@echo

$(rv32ui_tests): %: rv32ui-p-%

#===============================================================================
# CoreMark
#-------------------------------------------------------------------------------
coremark: rvemu
	@echo -------------------------------------------------------------------------------
	@echo $@
	@./rvemu $(COREMARK_DIR)/$@.bin
	@echo
