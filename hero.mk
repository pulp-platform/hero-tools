# Copyright 2024 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Cyril Koenig <cykoenig@iis.ee.ethz.ch>

HERO_PLATFORMS_DIR := $(HERO_ROOT)/platforms
HERO_SW_DIR := $(HERO_ROOT)/sw
HERO_ARTIFACTS_DIR := $(HERO_ROOT)/artifacts

all:
	echo "Hello world, help message incoming"

########################
# Artifacts management #
########################

include $(HERO_ARTIFACTS_DIR)/artifacts.mk

############
# CVA6 SDK #
############

HERO_CVA6_SDK_DIR := $(HERO_ROOT)/cva6-sdk

# Outputs of cva6-sdk/buildroot
HERO_BR_OUTPUT_DIR := $(realpath $(HERO_CVA6_SDK_DIR)/buildroot/output/)
HERO_LINUX_CROSS_COMPILE := $(HERO_BR_OUTPUT_DIR)/host/bin/riscv64-buildroot-linux-gnu-
HERO_KERNEL_DIR    := $(HERO_BR_OUTPUT_DIR)/build/linux-5.16.9

HERO_ARTIFACTS_ROOT_tc-gcc := $(HERO_ARTIFACTS_ROOT)/tc_gcc
HERO_ARTIFACTS_DATA_tc-gcc := $(HERO_CVA6_SDK_DIR)/buildroot/output/host
HERO_ARTIFACTS_VARS_tc-gcc :=
HERO_ARTIFACTS_FILES_tc-gcc := $(HERO_CVA6_SDK_DIR)/configs/buildroot64_defconfig

# Cross compiler *Linux* gcc toolchain
$(HERO_LINUX_CROSS_COMPILE)-gcc:
	make -C $(HERO_CVA6_SDK_DIR) all

# Compile linux image and device driver
$(HERO_CVA6_SDK_DIR)/install64/vmlinux:
	make -C $(HERO_CVA6_SDK_DIR) images

hero-tc-gcc: $(HERO_LINUX_CROSS_COMPILE)-gcc
hero-tc-gcc-artifacts: hero-load-artifacts-tc-gcc $(HERO_LINUX_CROSS_COMPILE)-gcc hero-save-artifacts-tc-gcc

hero-cva6-sdk-clean:
	make -C $(HERO_CVA6_SDK_DIR) clean

hero-cva6-sdk-all: $(HERO_LINUX_CROSS_COMPILE)-gcc $(HERO_CVA6_SDK_DIR)/install64/vmlinux

########
# LLVM #
########

HERO_ARTIFACTS_ROOT_tc-llvm := $(HERO_ARTIFACTS_ROOT)/tc_llvm
HERO_ARTIFACTS_DATA_tc-llvm := $(HERO_INSTALL)
HERO_ARTIFACTS_VARS_tc-llvm := "$(shell git submodule status | grep llvm-project)"
HERO_ARTIFACTS_FILES_tc-llvm := $(HERO_ROOT)/toolchain/*.sh $(shell find $(HERO_ROOT)/toolchain/llvm-support -type f)

# Cross compiler LLVM host/device toolchain with newlib + rt32
$(HERO_INSTALL)/bin/clang:
	mkdir -p $(HERO_ROOT)/output/tc-llvm/
	cd $(HERO_ROOT)/output/tc-llvm/ && $(HERO_ROOT)/toolchain/setup-llvm.sh Release
.PRECIOUS: $(HERO_INSTALL)/bin/clang

# Build newlib for a MARCH ($1) and MABI $(2) couple
HERO_DEVS_NEWLIB :=
define hero_devs_build_newlib =
$(HERO_INSTALL)/$(1)-$(2): $(HERO_INSTALL)/bin/clang
	cd $(HERO_ROOT)/output/tc-llvm/ && MARCH=$(1) MABI=$(2) $(HERO_ROOT)/toolchain/setup-llvm-device.sh $(HERO_ROOT)/toolchain/llvm-project
HERO_DEVS_NEWLIB += $(HERO_INSTALL)/$(1)-$(2)
.PRECIOUS: $(HERO_INSTALL)/$(1)-$(2)
endef
# Todo check for c(ompressed) support
$(eval $(call hero_devs_build_newlib,rv32ima,ilp32))
$(eval $(call hero_devs_build_newlib,rv32imafd,ilp32d))
$(eval $(call hero_devs_build_newlib,rv32imafdvzfh,ilp32d))

hero-tc-llvm: $(HERO_INSTALL)/bin/clang
hero-tc-llvm-artifacts: hero-load-artifacts-tc-llvm $(HERO_INSTALL)/bin/clang $(HERO_DEVS_NEWLIB) hero-save-artifacts-tc-llvm
    # $(HERO_DEVS_NEWLIB) should be more recent than clang for GNU Make
	touch $(HERO_DEVS_NEWLIB)

###############
# SW runtimes #
###############

include $(HERO_SW_DIR)/sw.mk

#############
# Platforms #
#############

include $(HERO_PLATFORMS_DIR)/platforms.mk

##########
# Footer #
##########

.PHONY: all hero-tc-gcc hero-cva6-sdk-all hero-tc-llvm

ifndef HERO_INSTALL
$(error HERO_INSTALL is not set, please source scripts/setenv.sh from the the root)
endif

ifndef HERO_ROOT
$(error HERO_ROOT is not set, please execute me from the root of the repository)
endif
