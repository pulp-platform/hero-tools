# Copyright 2020 ETH Zurich and University of Bologna.
# Solderpad Hardware License, Version 0.51, see LICENSE for details.
# SPDX-License-Identifier: SHL-0.51

HERO_ROOT = $(realpath ../../..)

PLATFORM ?= BUILDROOT

BR_OUTPUT_DIR ?= $(HERO_ROOT)/cva6-sdk/buildroot/output/

KERNEL_DIR_NATIVE := /lib/modules/$(shell uname -r)/build
KERNEL_DIR_BUILDROOT := $(BR_OUTPUT_DIR)/build/linux-6.1.22

CROSS_COMPILE_NATIVE ?=
CROSS_COMPILE_BUILDROOT ?= $(BR_OUTPUT_DIR)/host/bin/riscv64-buildroot-linux-gnu-

ARCH := riscv

# Linux compilation arguments
obj-m := carfield.o
carfield-objs := carfield_driver.o carfield_fops.o
ccflags-y += -I$(src)/../common

.PHONY: all dis clean build

all: modules
build: modules

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE_$(PLATFORM)) -C $(KERNEL_DIR_$(PLATFORM)) M=$(shell pwd) ARCH=riscv modules

dis: modules
	$(CROSS_COMPILE_$(PLATFORM))objdump -DS carfield.ko > carfield.disasm

clean:
	rm -f modules.order
	rm -f *.mod
	rm -f *.o
	rm -f *.ko
	rm -rf *.mod.c
	rm -f Module.symvers
	rm -f .*.o.cmd
	rm -f .*.ko.cmd
	rm -f *.disasm
	rm -f *.o_shipped
	rm -f *.cmd
	rm -f .*.cmd
