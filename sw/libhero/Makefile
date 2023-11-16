# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Cyril Koenig <cykoenig@iis.ee.ethz.ch>

HERO_ROOT = $(realpath ../..)

MAKEFLAGS=--warn-undefined-variables

BR_OUTPUT_DIR ?= $(realpath ../../cva6-sdk/buildroot/output/)
CROSS_COMPILE ?= $(BR_OUTPUT_DIR)/host/bin/riscv64-buildroot-linux-gnu-

CFLAGS		:= -Wall -O0 -g -fPIC -DPLATFORM=$(PLATFORM) $(CFLAGS)
CFLAGS      := -DLINUX_APP $(CFLAGS)
CC          := $(CROSS_COMPILE)gcc
AR          := $(CROSS_COMPILE)ar
OBJDUMP     := $(CROSS_COMPILE)objdump

SRCS   := src/carfield/safety_island.c src/common/libhero_api.c vendor/o1heap/o1heap/o1heap.c
OBJS   := $(SRCS:%.c=%.o)

LIBDIR := lib
SO     := $(LIBDIR)/libhero.so
A      := $(LIBDIR)/libhero.a

# include paths
INC_DIRS  := include
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

all: $(SO) $(A)

$(SO): $(OBJS) | $(LIBDIR)
	$(CC) $(CFLAGS) -shared -o $@ $^

$(A): $(OBJS) | $(LIBDIR)
	$(AR) rvs -o $@ $^

-include $(OBJS:.o=.d)

%.o : %.c
	$(CC) $(CFLAGS) $(INC_FLAGS) $^ -c -o $@
	$(CC) $(CFLAGS) $(INC_FLAGS) $^ -MM -c > $*.d

%.dump : %.a
	$(OBJDUMP) -S $^ > $@

$(LIBDIR):
	mkdir -p $@

.PHONY: clean deploy

deploy:
	mkdir -p $(HERO_ROOT)/cva6-sdk/rootfs/usr/lib
	cp $(SO) $(HERO_ROOT)/cva6-sdk/rootfs/usr/lib

upload:
	scp $(SO) root@172.31.182.94:/usr/lib

clean:
	rm -rf $(DEPDIR)
	rm -rf $(LIBDIR)
	rm -f $(OBJS)
	find . -name "*.o" -delete
	find . -name "*.d" -delete