# Copyright 2020 ETH Zurich and University of Bologna.
# Solderpad Hardware License, Version 0.51, see LICENSE for details.
# SPDX-License-Identifier: SHL-0.51

obj-m := snitch_pcie.o

CFLAGS += -DPCIE_OFFLOAD
ccflags-y += ${CFLAGS} -O2 -DDEBUG

HOST_KERNEL := $(shell uname -r)
KERNEL_DIR := /lib/modules/$(HOST_KERNEL)/build

.PHONY: all dis clean build

all: modules
build: modules

modules: $(patsubst %.o,%.c,$(obj-m)) $(patsubst %.o,%.h,$(obj-m))
	echo ${CFLAGS}
	$(MAKE) -S $(ARCH_ARG) $(CROSS_COMPILE_ARG) -C $(KERNEL_DIR) M=$(shell pwd) modules

dis: modules
	$(CROSS_COMPILE)objdump -DS snitch.ko > snitch.disasm

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