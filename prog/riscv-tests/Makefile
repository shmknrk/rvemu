#===============================================================================
# Makefile for riscv-tests/isa
#-------------------------------------------------------------------------------
XLEN                ?= 32

block_size          := 8KiB

src_dir             := riscv-tests/isa

ifeq ($(XLEN),64)
include $(src_dir)/rv64ui/Makefrag
include $(src_dir)/rv64uc/Makefrag
include $(src_dir)/rv64um/Makefrag
include $(src_dir)/rv64ua/Makefrag
include $(src_dir)/rv64uf/Makefrag
include $(src_dir)/rv64ud/Makefrag
include $(src_dir)/rv64uzfh/Makefrag
include $(src_dir)/rv64si/Makefrag
include $(src_dir)/rv64ssvnapot/Makefrag
include $(src_dir)/rv64mi/Makefrag
include $(src_dir)/rv64mzicbo/Makefrag
endif
include $(src_dir)/rv32ui/Makefrag
include $(src_dir)/rv32uc/Makefrag
include $(src_dir)/rv32um/Makefrag
include $(src_dir)/rv32ua/Makefrag
include $(src_dir)/rv32uf/Makefrag
include $(src_dir)/rv32ud/Makefrag
include $(src_dir)/rv32uzfh/Makefrag
include $(src_dir)/rv32si/Makefrag
include $(src_dir)/rv32mi/Makefrag

default: rv32ui

#-------------------------------------------------------------------------------
# Build rules
#-------------------------------------------------------------------------------
RISCV_PREFIX        ?= riscv$(XLEN)-unknown-elf-
RISCV_GCC           ?= $(RISCV_PREFIX)gcc
RISCV_GCC_OPTS      ?= -O2 -static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles
RISCV_OBJDUMP       ?= $(RISCV_PREFIX)objdump --disassemble-all --disassemble-zeroes --section=.text --section=.text.startup --section=.text.init --section=.data
RISCV_OBJCOPY       ?= $(RISCV_PREFIX)objcopy

vpath %.S $(src_dir)

#-------------------------------------------------------------------------------
# Build assembly tests
#-------------------------------------------------------------------------------
%.dump: %.elf
	$(RISCV_OBJDUMP) $< > $@

%.32.hex: %.bin
	hexdump -v -e '1/4 "%08x\n"' $< > $@

%.64.hex: %.bin
	hexdump -v -e '1/8 "%016x\n"' $< > $@

%.bin: %.elf
	$(RISCV_OBJCOPY) -O binary $< $@.tmp
	dd if=$@.tmp of=$@ bs=$(block_size) conv=sync
	rm -f $@.tmp

# $(eval $(call compile_template,TVM,-march=ARCH -mabi=ABI))
define compile_template
$1_tests += $$($1_p_tests)

$1_p_tests_elf = $$(addsuffix .elf,$$($1_p_tests))

$$($1_p_tests_elf): $1-p-%.elf: $1/%.S
	$$(RISCV_GCC) $2 $$(RISCV_GCC_OPTS) -I. -I$$(src_dir)/../env -I$$(src_dir)/macros/scalar -Tlink.ld $$< -o $$@

$1_tests_elf    = $$(addsuffix .elf, $$($1_tests))
$1_tests_bin    = $$(addsuffix .bin, $$($1_tests))
$1_tests_32_hex = $$(addsuffix .32.hex, $$($1_tests))
$1_tests_64_hex = $$(addsuffix .64.hex, $$(filter rv64%,$$($1_tests)))
$1_tests_dump   = $$(addsuffix .dump, $$($1_tests))

.PHONY: $1
$1: $$($1_tests_elf) $$($1_tests_bin) $$($1_tests_32_hex) $$($1_tests_64_hex) $$($1_tests_dump)

COMPILER_SUPPORTS_$1 := $$(shell $$(RISCV_GCC) $2 -c -x c /dev/null -o /dev/null 2> /dev/null; echo $$$$?)

ifeq ($$(COMPILER_SUPPORTS_$1),0)
tests += $$($1_tests)
endif
endef # compile_template

$(eval $(call compile_template,rv32ui,-march=rv32g -mabi=ilp32))
$(eval $(call compile_template,rv32uc,-march=rv32g -mabi=ilp32))
$(eval $(call compile_template,rv32um,-march=rv32g -mabi=ilp32))
$(eval $(call compile_template,rv32ua,-march=rv32g -mabi=ilp32))
$(eval $(call compile_template,rv32uf,-march=rv32g -mabi=ilp32))
$(eval $(call compile_template,rv32ud,-march=rv32g -mabi=ilp32))
$(eval $(call compile_template,rv32uzfh,-march=rv32g_zfh -mabi=ilp32))
$(eval $(call compile_template,rv32si,-march=rv32g -mabi=ilp32))
$(eval $(call compile_template,rv32mi,-march=rv32g -mabi=ilp32))
ifeq ($(XLEN),64)
$(eval $(call compile_template,rv64ui,-march=rv64g -mabi=lp64))
$(eval $(call compile_template,rv64uc,-march=rv64g -mabi=lp64))
$(eval $(call compile_template,rv64um,-march=rv64g -mabi=lp64))
$(eval $(call compile_template,rv64ua,-march=rv64g -mabi=lp64))
$(eval $(call compile_template,rv64uf,-march=rv64g -mabi=lp64))
$(eval $(call compile_template,rv64ud,-march=rv64g -mabi=lp64))
$(eval $(call compile_template,rv64uzfh,-march=rv64g_zfh -mabi=lp64))
$(eval $(call compile_template,rv64mzicbo,-march=rv64g_zicboz -mabi=lp64))
$(eval $(call compile_template,rv64si,-march=rv64g -mabi=lp64))
$(eval $(call compile_template,rv64ssvnapot,-march=rv64g -mabi=lp64))
$(eval $(call compile_template,rv64mi,-march=rv64g -mabi=lp64))
endif

tests_elf     = $(addsuffix .elf, $(tests))
tests_bin     = $(addsuffix .bin, $(tests))
tests_32_hex  = $(addsuffix .32.hex, $(tests))
tests_64_hex  = $(addsuffix .64.hex, $(filter rv64%,$(tests)))
tests_dump    = $(addsuffix .dump, $(tests))

#-------------------------------------------------------------------------------
.PHONY: all
all: $(tests_elf) $(tests_bin) $(tests_32_hex) $(tests_64_hex) $(tests_dump)

#-------------------------------------------------------------------------------
.PHONY: clean
clean:
	rm -f *.elf *.bin *.hex *.dump
