# Copyright 2022 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Cyril Koenig <cykoenig@iis.ee.ethz.ch>
#
# This file builds all the OpenMP target libraries on RV64
# it requires ../libllvm to be compiled

HERO_ROOT = $(realpath ../..)

BR_OUTPUT_DIR ?= $(realpath ../../cva6-sdk/buildroot/output)
RISCV          = $(BR_OUTPUT_DIR)/host

# Note: Could this be compiled directly with the clang cross compiler?
CC            := $(RISCV)/bin/riscv64-buildroot-linux-gnu-gcc
CXX           := $(RISCV)/bin/riscv64-buildroot-linux-gnu-g++

LLVM_DIR := $(realpath ../libllvm)
LIBOMPTARGET_LLVM_INCLUDE_DIRS := $(realpath ../libllvm/include/llvm)
HERO_LIBHERO_DIR := $(realpath ../../sw/libhero)
PLATFORM ?=

LLVM_COMPILE_FLAGS= \
	-DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_CROSSCOMPILING=True \
	-DCMAKE_SYSTEM_NAME=Linux \
	-DCLANG_TABLEGEN=$(HERO_ROOT)/install/bin/clang-tblgen \
	-DCMAKE_C_FLAGS='-march=rv64imafdc' \
	-DCMAKE_CXX_FLAGS='-march=rv64imafdc' \
	-DLLVM_TARGETS_TO_BUILD=RISCV \
	-DLLVM_ENABLE_ASSERTIONS=OFF \
	-DLLVM_INCLUDE_TESTS=OFF \
	-DLIBOMPTARGET_DEP_LIBFFI_LIBRARIES='$(RISCV)/riscv64-buildroot-linux-gnu/sysroot/usr/lib/' \
	-DLIBOMPTARGET_DEP_LIBFFI_INCLUDE_DIRS='$(RISCV)/riscv64-buildroot-linux-gnu/sysroot/usr/include/' \
	-DLIBOMPTARGET_BUILD_CUDA_PLUGIN=0 \
	-DLIBOMPTARGET_HERO_DEV_TYPE=$(PLATFORM) \
	-DLIBOMPTARGET_LLVM_INCLUDE_DIRS=$(LIBOMPTARGET_LLVM_INCLUDE_DIRS) \
	-DOMPTARGET_DEBUG=1 \
	-DCMAKE_CXX_FLAGS='-g'

# Outputs
all: lib/libomp.so lib/libomptarget.so

lib/libomp.so: compile

build.ninja: configure

# LLVM Cmake flow
configure:
	LLVM_DIR=$(LLVM_DIR) CC=$(CC) CXX=$(CXX) HERO_LIBHERO_DIR=$(HERO_LIBHERO_DIR) HERO_DEV_INC_DIR=$(HERO_LIBHERO_DIR)/include cmake ${LLVM_COMPILE_FLAGS} -G Ninja $(HERO_ROOT)/toolchain/llvm-project/openmp

lib/libomptarget.so: compile
lib/libomp.so: compile
compile: build.ninja
	ninja -j 8 -v
	mkdir -p lib include
	find . -name "*.so" -not -path "./lib/*"     | xargs -I{} cp {} lib
	find . -name "*.a"  -not -path "./lib/*"     | xargs -I{} cp {} lib
	find . -name "*.h"  -not -path "./include/*" | xargs -I{} cp {} include

# Clean everything but Makefile and gitignore
clean:
	find . -not -name Makefile -not -name .gitignore -delete

.PHONY: all configure compile clean
