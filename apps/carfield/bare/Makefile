# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Cyril Koenig <cykoenig@iis.ee.ethz.ch>

# Path to the root of the repository
HERO_ROOT = $(realpath ../../..)

# Path to compiled buildroot
BR_OUTPUT_DIR ?= $(HERO_ROOT)/cva6-sdk/buildroot/output/
CROSS_COMPILE ?= $(BR_OUTPUT_DIR)/host/bin/riscv64-buildroot-linux-gnu-

PLATFORM ?= SAFETY_ISLAND
platform := $(shell echo $(PLATFORM) | tr '[:upper:]' '[:lower:]')

# Binaries
CC          := $(CROSS_COMPILE)gcc
AR          := $(CROSS_COMPILE)ar
OBJDUMP     := $(CROSS_COMPILE)objdump

# Final variables
CFLAGS := -Wall -O0 -g
CFLAGS := $(CFLAGS) -Iinclude -Isrc/common -Ivendor/o1heap/o1heap -I$(HERO_ROOT)/sw/libhero/include
CFLAGS := $(CFLAGS) -Wl,-R$(HERO_ROOT)/sw/libhero/lib
LDFLAGS := -L$(HERO_ROOT)/sw/libhero/lib -lhero_safety_island

SRCS   := main.c
OBJS   := $(SRCS:%.c=%.o)

all: main

%.o : %.c
	$(CC) $(CFLAGS) $^ -c -o $@
	$(CC) $(CFLAGS) $^ -MM -c > $*.d

%.dump : %
	$(OBJDUMP) -S $^ > $@

.PHONY: clean
clean:
	find . -name "*.o" -delete
	find . -name "*.d" -delete
	rm -f main
