#===============================================================================
# Config
#-------------------------------------------------------------------------------
XLEN                := 32
#XLEN                := 64
#USE_MULDIV          := 1
#USE_ATOMIC          := 1
#USE_COMPRESSED      := 1

ITERATIONS          := 1
TARGET              := coremark

#===============================================================================
# Makefile
#-------------------------------------------------------------------------------
source-to-object     = \
    $(subst .c,.o, $(filter %c,$1)) \
    $(subst .s,.o, $(filter %s,$1))

#===============================================================================
# Sources
#-------------------------------------------------------------------------------
SRC_DIR             := coremark
PORT_DIR            := riscv
INC_DIR             += $(SRC_DIR) $(PORT_DIR)

SRCS                += $(wildcard $(SRC_DIR)/*.c)
SRCS                += $(wildcard $(PORT_DIR)/*.c)
SRCS                += $(wildcard $(PORT_DIR)/*.s)
OBJS                := $(call source-to-object,$(SRCS))

#===============================================================================
# RISC-V
#-------------------------------------------------------------------------------
### Architecture
RISCV_ARCH          := rv$(XLEN)i
ifdef USE_MULDIV
RISCV_ARCH          := $(addsuffix m,$(RISCV_ARCH))
endif
ifdef USE_ATOMIC
RISCV_ARCH          := $(addsuffix a,$(RISCV_ARCH))
endif
ifdef USE_COMPRESSED
RISCV_ARCH          := $(addsuffix c,$(RISCV_ARCH))
endif

### ABI
ifeq      ($(XLEN),32)
RISCV_ABI           := ilp32
else ifeq ($(XLEN),64)
RISCV_ABI           := lp64
else
$(error Invalid XLEN!!)
endif

-include config.mk
ifeq ($(RISCV),)
$(error Error: Please set the $$RISCV environment variable.)
endif
RISCV_PREFIX        := $(RISCV)/bin/riscv$(XLEN)-unknown-elf-
RISCV_GCC           := $(RISCV_PREFIX)gcc
RISCV_OBJDUMP       := $(RISCV_PREFIX)objdump -d
RISCV_OBJCOPY       := $(RISCV_PREFIX)objcopy

RISCV_CFLAGS        += -O2
RISCV_CFLAGS        += -march=$(RISCV_ARCH)
RISCV_CFLAGS        += -mabi=$(RISCV_ABI)
RISCV_CFLAGS        += -nostartfiles
RISCV_CFLAGS        += -DXLEN=$(XLEN)
RISCV_CFLAGS        += -DITERATIONS=$(ITERATIONS)
RISCV_CFLAGS        += $(addprefix -I,$(INC_DIR))

RISCV_LFLAGS        += -T$(PORT_DIR)/link.ld

#===============================================================================
# Build rules
#-------------------------------------------------------------------------------
.PHONY: default clean distclean
default: $(RISCV_ARCH) $(addprefix $(RISCV_ARCH)/$(TARGET), .elf .bin .32.hex .dump)

$(RISCV_ARCH):
	mkdir -p $@

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

.c.o:
	$(RISCV_GCC) -c $(RISCV_CFLAGS) -DFLAGS_STR=\""$(RISCV_CFLAGS)"\" $< -o $@

.s.o:
	$(RISCV_GCC) -c $(RISCV_CFLAGS) $< -o $@

clean:
	rm -f *.elf *.bin *.hex *.dump

distclean: clean
	rm -rf rv32*/ rv64*/
