ROOT := $(shell realpath ../../../..)

HOST ?= sg2042
TARGET_HOST := riscv64-hero-linux-gnu
DEVS := hero1
TARGET_DEVS := $(foreach dev,$(DEVS),riscv32-hero-$(dev)-elf)

CSRCS = run.c matvec.c

CFLAGS   += -O3
CFLAGS   += -v -save-temps=obj

### HOST ###

LDFLAGS  += -L$(ROOT)/sw/libhero/lib -lhero_spatz_cluster -lm

### DEVICE 1 ###

CFLAGS   += --hero1-sysroot=$(HERO_INSTALL)/rv32imafdvzfh-ilp32d/riscv32-unknown-elf
CFLAGS   += -hero1-march=rv32imafdvzfh_xdma
CFLAGS   += -hero1-D__HERO_1 -hero1-D__HERO_DEV

LDFLAGS  += -hero1-I../common

CFLAGS   += -hero1-I$(CARFIELD_ROOT)/spatz/sw/snRuntime/include
CFLAGS   += -hero1-I$(CARFIELD_ROOT)/spatz/sw/snRuntime/vendor
CFLAGS   += -hero1-I$(CARFIELD_ROOT)/spatz/sw/snRuntime/vendor/riscv-opcodes

LDFLAGS   += -hero1-L$(HERO_INSTALL)/lib/clang/15.0.0/rv32imafdvzfh-ilp32d/lib/
LDFLAGS   += -hero1-lclang_rt.builtins-riscv32

LDFLAGS  += -hero1-T$(CARFIELD_ROOT)/spatz/hw/system/spatz_cluster/sw/build/snRuntime/common.ld
LDFLAGS  += -hero1-L$(CARFIELD_ROOT)/spatz/hw/system/spatz_cluster/sw/build/spatzBenchmarks
LDFLAGS  += -hero1-lomptarget

LDFLAGS  += -hero1-L$(CARFIELD_ROOT)/spatz/hw/system/spatz_cluster/sw/build/snRuntime
LDFLAGS  += -hero1-lsnRuntime-cluster

LDFLAGS  += --hero1-ld-path=$(HERO_INSTALL)/bin/ld.lld

-include ../common/default.mk

.PRECIOUS: helloworld-hero1.OMP.ll helloworld-host.OMP.ll
