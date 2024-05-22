# Copyright 2020 ETH Zurich and University of Bologna.
# Solderpad Hardware License, Version 0.51, see LICENSE for details.
# SPDX-License-Identifier: SHL-0.51

HERO_ROOT = $(realpath ../../..)

# Platform BUILDROOT : cross compile driver for the FPGA
#          NATIVE    : compile driver for this machine
PLATFORM ?= BUILDROOT

KERNEL_DIR_NATIVE := /lib/modules/$(shell uname -r)/build
KERNEL_DIR_BUILDROOT := $(BR_LINUX_DIR)

BR_OUTPUT_DIR ?= $(HERO_ROOT)/cva6-sdk/buildroot/output/
CROSS_COMPILE_NATIVE ?=
CROSS_COMPILE_BUILDROOT ?= $(BR_OUTPUT_DIR)/host/bin/riscv64-buildroot-linux-gnu-

obj-m := occamy.o
occamy-objs := occamy_driver.o occamy_fops.o

all: modules
build: modules

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE_$(PLATFORM)) -C $(KERNEL_DIR_$(PLATFORM)) M=$(shell pwd) ARCH=riscv modules

dis: modules
	$(CROSS_COMPILE)objdump -DS occamy.ko > occamy.ko.dump

clean:
	rm -f Module.symvers
	rm -f modules.order
	rm -f *.mod
	rm -rf *.mod.c
	rm -f *.o
	rm -f *.ko
	rm -f *.dump
	rm -f *.cmd
	rm -f .*.cmd

.PHONY: all deploy dis clean build
