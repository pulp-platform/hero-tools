# Copyright 2024 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Cyril Koenig <cykoenig@iis.ee.ethz.ch>

NUM_DEVICES := 0

# Usage add_device [device]
# Example: $(eval $(call add_device,"occamy"))
define add_device =
# Increment NUM_DEVICES
$(eval NUM_DEVICES=$(shell echo $$(($(NUM_DEVICES)+1))))
# Define __HERO_DEV __HERO_[DEIVCE_NUM] and __HERO_[DEVICE_NAME]
CFLAGS   += -hero$(NUM_DEVICES)-D__HERO_$(NUM_DEVICES) -hero$(NUM_DEVICES)-D__HERO_DEV
CFLAGS   += -hero$(NUM_DEVICES)-D__HERO_$(shell echo $(1) | tr  '[:lower:]' '[:upper:]')
# Common device linker args
LDFLAGS  += -hero$(NUM_DEVICES)-I../common
LDFLAGS  += --hero$(NUM_DEVICES)-ld-path=$(HERO_INSTALL)/bin/ld.lld
# Host linker args
LDFLAGS  +=  -L$(HERO_ROOT)/sw/libhero/lib
# Common includes
CFLAGS   += -hero$(NUM_DEVICES)-I$(HERO_ROOT)/apps/omp

ifeq ($(1),occamy)
# ABI march
CFLAGS   += --hero$(NUM_DEVICES)-sysroot=$(HERO_INSTALL)/rv32imafd-ilp32d/riscv32-unknown-elf
CFLAGS   += -hero$(NUM_DEVICES)-march=rv32imafd_zfh1p0_xfrep0p1_xssr0p1_xdma0p1_xfalthalf0p1_xfquarter0p1_xfaltquarter0p1_xfvecsingle0p1_xfvechalf0p1_xfvecalthalf0p1_xfvecquarter0p1_xfvecaltquarter0p1_xfauxhalf0p1_xfauxalthalf0p1_xfauxquarter0p1_xfauxaltquarter0p1_xfauxvecsingle0p1_xfauxvechalf0p1_xfauxvecalthalf0p1_xfauxvecquarter0p1_xfauxvecaltquarter0p1_xfexpauxvechalf0p1_xfexpauxvecalthalf0p1_xfexpauxvecquarter0p1_xfexpauxvecaltquarter0p1
LDFLAGS  += -hero$(NUM_DEVICES)-L$(HERO_INSTALL)/lib/clang/15.0.0/rv32imafd-ilp32d/lib/
LDFLAGS  += -hero$(NUM_DEVICES)-lclang_rt.builtins-riscv32
# Runtime
CFLAGS   += -hero$(NUM_DEVICES)-I$(OCCAMY_ROOT)/target/sim/sw/device/runtime/src
CFLAGS   += -hero$(NUM_DEVICES)-I$(OCCAMY_ROOT)/target/sim/sw/shared/platform/generated/
CFLAGS   += -hero$(NUM_DEVICES)-I$(OCCAMY_ROOT)/target/sim/sw/shared/platform/
CFLAGS   += -hero$(NUM_DEVICES)-I$(OCCAMY_ROOT)/target/sim/sw/shared/runtime/
CFLAGS   += -hero$(NUM_DEVICES)-I$(OCCAMY_ROOT)/deps/snitch_cluster/sw/snRuntime/api
CFLAGS   += -hero$(NUM_DEVICES)-I$(OCCAMY_ROOT)/deps/snitch_cluster/sw/snRuntime/src
CFLAGS   += -hero$(NUM_DEVICES)-I$(OCCAMY_ROOT)/deps/snitch_cluster/sw/snRuntime/api/omp
CFLAGS   += -hero$(NUM_DEVICES)-I$(OCCAMY_ROOT)/deps/snitch_cluster/sw/snRuntime/src/omp
CFLAGS   += -hero$(NUM_DEVICES)-I$(OCCAMY_ROOT)/deps/snitch_cluster/sw/deps/riscv-opcodes
LDFLAGS  += -hero$(NUM_DEVICES)-T$(OCCAMY_ROOT)/target/sim/sw/device/apps/libomptarget_device/link.ld
LDFLAGS  += -hero$(NUM_DEVICES)-L$(OCCAMY_ROOT)/target/sim/sw/device/apps/libomptarget_device/build
LDFLAGS  += -hero$(NUM_DEVICES)-lomptarget_device
# Host flags
LDFLAGS  += -lhero_occamy
endif

ifeq ($(1),spatz_cluster)
# ABI MARCH
CFLAGS   += --hero$(NUM_DEVICES)-sysroot=$(HERO_INSTALL)/rv32imafdvzfh-ilp32d/riscv32-unknown-elf
CFLAGS   += -hero$(NUM_DEVICES)-march=rv32imafdvzfh_xdma
LDFLAGS  += -hero$(NUM_DEVICES)-L$(HERO_INSTALL)/lib/clang/15.0.0/rv32imafdvzfh-ilp32d/lib/
LDFLAGS  += -hero$(NUM_DEVICES)-lclang_rt.builtins-riscv32
# Runtime
CFLAGS   += -hero$(NUM_DEVICES)-I$(CARFIELD_ROOT)/spatz/sw/snRuntime/include
CFLAGS   += -hero$(NUM_DEVICES)-I$(CARFIELD_ROOT)/spatz/sw/snRuntime/vendor
CFLAGS   += -hero$(NUM_DEVICES)-I$(CARFIELD_ROOT)/spatz/sw/snRuntime/vendor/riscv-opcodes
LDFLAGS  += -hero$(NUM_DEVICES)-T$(CARFIELD_ROOT)/spatz/hw/system/spatz_cluster/sw/build/snRuntime/common.ld
LDFLAGS  += -hero$(NUM_DEVICES)-L$(CARFIELD_ROOT)/spatz/hw/system/spatz_cluster/sw/build/snRuntime
LDFLAGS  += -hero$(NUM_DEVICES)-L$(CARFIELD_ROOT)/spatz/hw/system/spatz_cluster/sw/build/spatzBenchmarks
LDFLAGS  += -hero$(NUM_DEVICES)-lsnRuntime-cluster
LDFLAGS  += -hero$(NUM_DEVICES)-lomptarget
# Host flags
LDFLAGS  += -lhero_spatz_cluster
endif

ifeq ($(1),safety_island)
# ABI MARCH
CFLAGS   += --hero1-sysroot=$(HERO_INSTALL)/rv32ima-ilp32/riscv32-unknown-elf
CFLAGS   += -hero1-march=rv32ima
LDFLAGS  += -hero1-L$(HERO_INSTALL)/lib/clang/15.0.0/rv32ima-ilp32/lib/
LDFLAGS  += -hero1-lclang_rt.builtins-riscv32
# Runtime
LDFLAGS  += -hero1-T$(CARFIELD_ROOT)/safety_island/sw/tests/runtime_omp/link.ld
LDFLAGS  += -hero1-L$(CARFIELD_ROOT)/safety_island/sw/tests/runtime_omp/build/
LDFLAGS  += -hero$(NUM_DEVICES)-lomptarget_runtime
# Host flags
LDFLAGS  += -lhero_safety_island
endif

endef
