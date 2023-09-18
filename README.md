# RVEmu

- RISC-V Emulator written in C++
- RV32I

## Installation

```bash
$ git clone https://github.com/shmknrk/rvemu.git
$ cd rvemu
$ git submodule update --init --recursive
$ echo 'export PATH="/path/to/riscv-gnu-toolchain:$PATH"' >> ~/.bashrc
$ source ~/.bashrc
$ make -C prog/riscv-tests
$ make -C prog/coremark
```

## Usage

```bash
cd rvemu

### riscv-tests/isa
$ make add
$ make sub
...
$ make auipc

### all riscv-tests/isa
$ make isa

### coremark
$ make coremark
```
