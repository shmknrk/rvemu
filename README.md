# RVEmu

- RISC-V Emulator written in C++
- RV32IMAC, RV64IM

## Installation

```bash
$ git clone https://github.com/shmknrk/rvemu.git
$ cd rvemu
$ git submodule update --init --recursive
$ echo 'export RISCV="/path/to/riscv"' >> ~/.bashrc
$ source ~/.bashrc
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

### embench
$ make aha-mont64
$ make crc32
...
$ make wikisort

### all embench-iot
$ make embench
```
