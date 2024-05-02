ROOT := $(shell realpath ../../../..)

TARGET_HOST := riscv64-hero-linux-gnu
DEVS := hero1# hero2
TARGET_DEVS := $(foreach dev,$(DEVS),riscv32-hero-$(dev)-elf)

CSRCS = helloworld.c

CFLAGS   += -O0
CFLAGS   += -v -debug -save-temps=obj

### HOST ###

LDFLAGS  += -L$(ROOT)/sw/libhero/lib -lhero_safety_island

### DEVICE 1 ###

CFLAGS   += --hero1-sysroot=$(HERO_INSTALL)/rv32ima-ilp32/riscv32-unknown-elf
CFLAGS   += -hero1-march=rv32ima
CFLAGS   += -hero1-D__HERO_1 -hero1-D__HERO_DEV

LDFLAGS   += -hero1-L$(HERO_INSTALL)/lib/clang/15.0.0/rv32ima-ilp32/lib/
LDFLAGS   += -hero1-lclang_rt.builtins-riscv32

LDFLAGS  += -hero1-T$(CARFIELD_ROOT)/safety_island/sw/tests/runtime_omp/link.ld
LDFLAGS  += -hero1-L$(CARFIELD_ROOT)/safety_island/sw/tests/runtime_omp/build/
LDFLAGS  += -hero1-L../common
LDFLAGS  += -hero1-lomptarget_runtime
LDFLAGS  += --hero1-ld-path=$(HERO_INSTALL)/bin/ld.lld

### DEVICE 2 ###

# CFLAGS   += --hero2-sysroot=$(HERO_INSTALL)/riscv32-unknown-elf
# CFLAGS   += -hero2-march=rv32ima
# LDFLAGS  += -hero2-T$(CARFIELD_ROOT)/safety_island/sw/pulp-runtime/kernel/chips/carfield/link.ld
# LDFLAGS  += -hero2-L$(CARFIELD_ROOT)/safety_island/sw/tests/runtime_omp/build/
# LDFLAGS  += -hero2-lomptarget_runtime
# LDFLAGS  += --hero2-ld-path=$(HERO_INSTALL)/bin/ld.lld

-include ../common/default.mk

.PRECIOUS: helloworld-hero1.OMP.ll helloworld-host.OMP.ll
