# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Cyril Koenig <cykoenig@iis.ee.ethz.ch>

# Path to the root of the repository
HERO_ROOT = $(realpath ../..)

# Path to compiled buildroot
BR_OUTPUT_DIR ?= $(realpath ../../cva6-sdk/buildroot/output/)
CROSS_COMPILE ?= $(BR_OUTPUT_DIR)/host/bin/riscv64-buildroot-linux-gnu-

PLATFORM ?=

# Binaries
CC          := $(CROSS_COMPILE)gcc
AR          := $(CROSS_COMPILE)ar
OBJDUMP     := $(CROSS_COMPILE)objdump

# Platform specific variables
CFLAGS_safety_island := -I src/carfield -I $(HERO_ROOT)/cva6-sdk/sw/drivers/carfield
SRCS_safety_island := src/carfield/safety_island.c

CFLAGS_spatz_cluster := -I src/carfield -I $(HERO_ROOT)/cva6-sdk/sw/drivers/carfield
SRCS_spatz_cluster := src/carfield/spatz_cluster.c

CFLAGS_occamy := -I src/occamy
SRCS_occamy := src/occamy/snitch_cluster.c

# Final variables
CFLAGS := $(CFLAGS_$(PLATFORM))
CFLAGS := $(CFLAGS) -Wall -O0 -g -fPIC -DPLATFORM=$(PLATFORM) -DLINUX_APP
CFLAGS := $(CFLAGS) -Iinclude -Isrc/common -Ivendor/o1heap/o1heap

SRCS   := $(SRCS_$(PLATFORM)) src/common/hero_api.c vendor/o1heap/o1heap/o1heap.c
OBJS   := $(SRCS:%.c=%.o)

LIBDIR := lib

all: lib/libhero_$(PLATFORM).so lib/libhero_$(PLATFORM).a lib/libhero_$(PLATFORM).dump

%.o : %.c
	$(CC) $(CFLAGS) $^ -c -o $@
	$(CC) $(CFLAGS) $^ -MM -c > $*.d

%.dump : %.a
	$(OBJDUMP) -S $^ > $@

check_platform:
	@if [ -z "$(SRCS_$(PLATFORM))" ]; then echo "Incorrect PLATFORM selected"; exit 1; fi

lib/libhero_$(PLATFORM).so: $(OBJS) | check_platform $(LIBDIR)
	$(CC) $(CFLAGS) -shared -o $@ $^

lib/libhero_$(PLATFORM).a: $(OBJS) | check_platform $(LIBDIR)
	$(AR) rvs -o $@ $^

$(LIBDIR):
	mkdir -p $@

.PHONY: clean deploy check_platform

clean:
	rm -rf lib
	find . -name "*.o" -delete
	find . -name "*.d" -delete

to_upper = $(shell echo '$*' | tr '[:upper:]' '[:lower:]')
