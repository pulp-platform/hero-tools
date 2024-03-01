ROOT := $(shell realpath ../../../..)

TARGET_HOST := riscv64-hero-linux-gnu
DEVS := hero1
TARGET_DEVS := $(foreach dev,$(DEVS),riscv32-hero-$(dev)-elf)

CSRCS = helloworld.c

CFLAGS   += -O0
CFLAGS   += -v -debug -save-temps=obj

### HOST ###

LDFLAGS  += -L$(ROOT)/sw/libhero/lib -lhero_safety_island

### DEVICE 1 ###

CFLAGS   += --hero1-sysroot=$(HERO_INSTALL)/rv32imafd-ilp32d/riscv32-unknown-elf
CFLAGS   += -hero1-march=rv32imafd
CFLAGS   += -hero1-D__HERO_1 -hero1-D__HERO_DEV

LDFLAGS   += -hero1-L$(HERO_INSTALL)/lib/clang/15.0.0/rv32imafd-ilp32d/lib/
LDFLAGS   += -hero1-lclang_rt.builtins-riscv32

LDFLAGS  += -hero1-T$(OCCAMY_ROOT)/target/sim/sw/device/apps/libomptarget_device/link.ld
LDFLAGS  += -hero1-L$(OCCAMY_ROOT)/target/sim/sw/device/apps/libomptarget_device/build
LDFLAGS  += -hero1-L../common
LDFLAGS  += -hero1-lomptarget_device
LDFLAGS  += --hero1-ld-path=$(HERO_INSTALL)/bin/ld.lld

-include ../common/default.mk

.PRECIOUS: helloworld-hero1.OMP.ll helloworld-host.OMP.ll
