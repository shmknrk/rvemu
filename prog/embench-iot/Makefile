#===============================================================================
# Config
#-------------------------------------------------------------------------------
XLEN                := 32
#XLEN                := 64
#USE_MULDIV          := 1
#USE_ATOMIC          := 1
#USE_COMPRESSED      := 1

EMBENCH_DIR         := embench-iot
BUILD_DIR           := $(CURDIR)/bd
LOG_DIR             := $(CURDIR)/logs
CONFIG_DIR          := $(EMBENCH_DIR)/config
ARCH                := riscv
CHIP                := generic
BOARD               := rvcore

#===============================================================================
# Sources
#-------------------------------------------------------------------------------
embench             := \
	aha-mont64 crc32 cubic edn huffbench matmult-int md5sum minver nbody \
	nettle-aes nettle-sha256 nsichneu picojpeg primecount qrduino \
	sglib-combined slre st statemate tarfind ud wikisort

embench_base_dir    := $(addprefix $(BUILD_DIR)/src/, $(embench))
embench_base        := $(join $(addsuffix /,$(embench_base_dir)), $(embench))
embench_dump        := $(addsuffix .dump,$(embench_base))
ifeq ($(XLEN),64)
embench_64_hex      := $(addsuffix .64.hex,$(embench_base))
endif
embench_32_hex      := $(addsuffix .32.hex,$(embench_base))
embench_bin         := $(addsuffix .bin,$(embench_base))

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

RISCV_LFLAGS        += -T$(CURDIR)/embench-iot/config/$(ARCH)/boards/$(BOARD)/link.ld

#===============================================================================
# Build rules
#-------------------------------------------------------------------------------
.PHONY: default all build clean distclean install
default: clean all install

all: build $(embench_bin) $(embench_32_hex) $(embench_64_hex) $(embench_dump)

build: $(CONFIG_DIR)/$(ARCH)
	$(EMBENCH_DIR)/build_all.py \
	--arch $(ARCH) \
	--chip $(CHIP) \
	--board $(BOARD) \
	--cc $(RISCV_GCC) \
	--cflags="$(RISCV_CFLAGS) -c" \
	--ldflags="$(RISCV_CFLAGS) $(RISCV_LFLAGS)" \
	--user-libs="-lm" \
	--builddir $(BUILD_DIR) \
	--logdir $(LOG_DIR)
	@rm -rf $(CONFIG_DIR)/$(ARCH)

$(CONFIG_DIR)/$(ARCH):
	cp -r $(ARCH) $(EMBENCH_DIR)/config

%.dump: %
	$(RISCV_OBJDUMP) $< > $@

%.32.hex: %.bin
	hexdump -v -e '1/4 "%08x\n"' $< > $@

%.64.hex: %.bin
	hexdump -v -e '1/8 "%016x\n"' $< > $@

%.bin: %
	$(RISCV_OBJCOPY) -O binary $< $@.tmp
	dd if=$@.tmp of=$@ conv=sync bs=128KiB
	rm -f $@.tmp

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(LOG_DIR)
	rm -rf $(CONFIG_DIR)/$(ARCH)
	rm -rf $(shell find . -name __pycache__)

distclean: clean
	rm -rf rv32*/ rv64*/

INSTALL_DIR         := $(RISCV_ARCH)
install: $(embench_dump) $(embench_32_hex) $(embench_64_hex) $(embench_bin)
	install -d $(INSTALL_DIR)
	install -p -m 644 $(embench_dump) $(embench_32_hex) $(embench_64_hex) $(embench_bin) $(INSTALL_DIR)
