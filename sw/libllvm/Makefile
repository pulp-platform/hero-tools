# Copyright 2022 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Cyril Koenig <cykoenig@iis.ee.ethz.ch>
#
# This file builds LLVM runtime librairies on RV64 as they are required
# to build libomp.so on RV64

HERO_ROOT = $(realpath ../..)

BR_OUTPUT_DIR ?= $(realpath ../../cva6-sdk/buildroot/output)
RISCV          = $(BR_OUTPUT_DIR)/host

# Force using the RV 64 toolchain
CC            := $(RISCV)/bin/riscv64-buildroot-linux-gnu-gcc
CXX           := $(RISCV)/bin/riscv64-buildroot-linux-gnu-g++

LLVM_DIR := $(realpath ../../install/lib/cmake/llvm)
LIBOMPTARGET_LLVM_INCLUDE_DIRS := $(realpath ../../install/include/llvm)
HERO_DEV_INC_DIR := $(realpath ../libsnitch/include)

# Note LLVM_TABLEGEN may be replaced by LLVM_NATIVE_TOOL_DIR in newer LLVM

LLVM_COMPILE_FLAGS= \
	-DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_CROSSCOMPILING=True \
	-DCMAKE_SYSTEM_NAME=Linux \
	-DLLVM_TABLEGEN=$(HERO_ROOT)/install/bin/llvm-tblgen \
	-DCLANG_TABLEGEN=$(HERO_ROOT)/install/bin/clang-tblgen \
	-DLLVM_DEFAULT_TARGET_TRIPLE=riscv64-unknown-linux-gnu \
	-DCMAKE_C_FLAGS='-march=rv64imafdc' \
	-DCMAKE_CXX_FLAGS='-march=rv64imafdc' \
	-DLLVM_TARGETS_TO_BUILD=RISCV \
	-DLLVM_TARGET_ARCH=riscv64 \
	-DLLVM_ENABLE_ASSERTIONS=OFF \
	-LLVM_INCLUDE_TESTS=OFF

all: compile

configure:
	CC=$(CC) CXX=$(CXX) cmake $(LLVM_COMPILE_FLAGS) -G Ninja ../../toolchain/llvm-project/llvm

build.ninja: configure

compile: build.ninja
	ninja -j `nproc`

# Clean everything but Makefile and gitignore
clean:
	find . -not -name Makefile -not -name .gitignore -delete

.PHONY: all configure compile clean
