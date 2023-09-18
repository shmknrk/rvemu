#===============================================================================
# Config
#-------------------------------------------------------------------------------
ITERATIONS          := 1
TARGET              := coremark

RISCV_XLEN          := 32
RISCV_ARCH          := rv32i
RISCV_ABI           := ilp32

#===============================================================================
# Makefile
#-------------------------------------------------------------------------------
source-to-object     = \
    $(subst .c,.o,$(filter %c,$1)) \
    $(subst .s,.o,$(filter %s,$1))

#===============================================================================
# Sources
#-------------------------------------------------------------------------------
SRC_DIR             := coremark
PORT_DIR            := riscv
INC_DIR             := $(SRC_DIR) $(PORT_DIR)

SRCS                += $(wildcard $(SRC_DIR)/*.c)
SRCS                += $(wildcard $(PORT_DIR)/*.c)
SRCS                += $(wildcard $(PORT_DIR)/*.s)
OBJS                := $(call source-to-object,$(SRCS))

#===============================================================================
# RISC-V
#-------------------------------------------------------------------------------
RISCV_PREFIX        := riscv$(RISCV_XLEN)-unknown-elf-
RISCV_GCC           := $(RISCV_PREFIX)gcc
RISCV_OBJDUMP       := $(RISCV_PREFIX)objdump -d
RISCV_OBJCOPY       := $(RISCV_PREFIX)objcopy

RISCV_CFLAGS        += -O2
RISCV_CFLAGS        += -march=$(RISCV_ARCH)
RISCV_CFLAGS        += -mabi=$(RISCV_ABI)
RISCV_CFLAGS        += -nostartfiles
RISCV_CFLAGS        += -DITERATIONS=$(ITERATIONS)
RISCV_CFLAGS        += $(addprefix -I,$(INC_DIR))

RISCV_LFLAGS        += -T$(PORT_DIR)/link.ld

#===============================================================================
# Build rules
#-------------------------------------------------------------------------------
.PHONY: default clean
default: $(addprefix $(TARGET), .elf .bin .32.hex .dump)

%.dump: %.elf
	$(RISCV_OBJDUMP) $< > $@

%.32.hex: %.bin
	hexdump -v -e '1/4 "%08x\n"' $< > $@

%.bin: %.elf
	$(RISCV_OBJCOPY) -O binary $< $@.tmp
	dd if=$@.tmp of=$@ bs=32KiB conv=sync
	rm -f $@.tmp

%.elf: $(OBJS)
	$(RISCV_GCC) $(RISCV_CFLAGS) $(RISCV_LFLAGS) $^ -o $@

%.o: %.c
	$(RISCV_GCC) -c $(RISCV_CFLAGS) -DFLAGS_STR=\""$(RISCV_CFLAGS)"\" $< -o $@

%.o: %.s
	$(RISCV_GCC) -c $(RISCV_CFLAGS) $< -o $@

clean:
	rm -f *.elf *.bin *.hex *.dump
