# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Cyril Koenig <cykoenig@iis.ee.ethz.ch>

ROOT := $(patsubst %/,%, $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

# Hero target
HERO_TARGET_HOST ?= root@172.31.182.94

# Outputs of cva6-sdk/buildroot
BR_OUTPUT_DIR ?= $(realpath ../../cva6-sdk/buildroot/output/)
CROSS_COMPILE ?= $(BR_OUTPUT_DIR)/host/bin/riscv64-buildroot-linux-gnu-
KERNEL_DIR    ?= $(BR_OUTPUT_DIR)/build/linux-5.16.9

all:
	echo "Hello world"

# Cross compiler gcc toolchain
tc-gcc:
	make -C cva6-sdk all

# Cross compiler llvm toolchain
tc-llvm:
	mkdir -p $(CURDIR)/output/tc-llvm/
	cd $(CURDIR)/output/tc-llvm/ && $(ROOT)/toolchain/setup-llvm.sh Release

# Newlib and compiler-rt32
tc-snitch:
	mkdir -p $(CURDIR)/output/tc-llvm/
	cd $(CURDIR)/output/tc-llvm/ && $(ROOT)/toolchain/setup-llvm-snitch.sh $(ROOT)/toolchain/llvm-project

sw:
	make -C sw-devices/libllvm
	make -C sw-devices/libomp deploy
# Rebuild image with Libraries
	make -C cva6-sdk clean images

deploy-hero:
	make -C libsnitch HERO_TARGET_HOST=$(HERO_TARGET_HOST) deploy
	make -C libllvm
	make -C libomp HERO_TARGET_HOST=$(HERO_TARGET_HOST) deploy

.PHONY: sw-devices
