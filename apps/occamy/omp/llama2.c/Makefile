ROOT := $(shell realpath ../../../..)

TARGET_HOST := riscv64-hero-linux-gnu
DEVS := hero1
TARGET_DEVS := $(foreach dev,$(DEVS),riscv32-hero-$(dev)-elf)

CSRCS = run.c matvec.c

CFLAGS   += -O2
CFLAGS   += -v -debug -save-temps=obj

### HOST ###

LDFLAGS  += -L$(ROOT)/sw/libhero/lib -lhero_occamy

### DEVICE 1 ###

CFLAGS   += --hero1-sysroot=$(HERO_INSTALL)/rv32imafd-ilp32d/riscv32-unknown-elf
CFLAGS   += -hero1-march=rv32imafd_zfh1p0_xfrep0p1_xssr0p1_xdma0p1_xfalthalf0p1_xfquarter0p1_xfaltquarter0p1_xfvecsingle0p1_xfvechalf0p1_xfvecalthalf0p1_xfvecquarter0p1_xfvecaltquarter0p1_xfauxhalf0p1_xfauxalthalf0p1_xfauxquarter0p1_xfauxaltquarter0p1_xfauxvecsingle0p1_xfauxvechalf0p1_xfauxvecalthalf0p1_xfauxvecquarter0p1_xfauxvecaltquarter0p1_xfexpauxvechalf0p1_xfexpauxvecalthalf0p1_xfexpauxvecquarter0p1_xfexpauxvecaltquarter0p1
CFLAGS   += -hero1-D__HERO_1 -hero1-D__HERO_DEV

LDFLAGS  += -hero1-I../common

CFLAGS   += -hero1-I$(OCCAMY_ROOT)/target/sim/sw/device/runtime/src
CFLAGS   += -hero1-I$(OCCAMY_ROOT)/target/sim/sw/shared/platform/generated/
CFLAGS   += -hero1-I$(OCCAMY_ROOT)/target/sim/sw/shared/platform/
CFLAGS   += -hero1-I$(OCCAMY_ROOT)/target/sim/sw/shared/runtime/
CFLAGS   += -hero1-I$(OCCAMY_ROOT)/deps/snitch_cluster/sw/snRuntime/api
CFLAGS   += -hero1-I$(OCCAMY_ROOT)/deps/snitch_cluster/sw/snRuntime/src
CFLAGS   += -hero1-I$(OCCAMY_ROOT)/deps/snitch_cluster/sw/snRuntime/api/omp
CFLAGS   += -hero1-I$(OCCAMY_ROOT)/deps/snitch_cluster/sw/snRuntime/src/omp
CFLAGS   += -hero1-I$(OCCAMY_ROOT)/deps/snitch_cluster/sw/deps/riscv-opcodes

LDFLAGS   += -hero1-L$(HERO_INSTALL)/lib/clang/15.0.0/rv32imafd-ilp32d/lib/
LDFLAGS   += -hero1-lclang_rt.builtins-riscv32

LDFLAGS  += -hero1-T$(OCCAMY_ROOT)/target/sim/sw/device/apps/libomptarget_device/link.ld
LDFLAGS  += -hero1-L$(OCCAMY_ROOT)/target/sim/sw/device/apps/libomptarget_device/build
LDFLAGS  += -hero1-lomptarget_device

LDFLAGS  += --hero1-ld-path=$(HERO_INSTALL)/bin/ld.lld

LDFLAGS  += -lm

-include ../common/default.mk

.PRECIOUS: lama-hero1.OMP.ll lama-host.OMP.ll
