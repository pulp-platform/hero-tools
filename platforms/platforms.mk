# Copyright 2024 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Cyril Koenig <cykoenig@iis.ee.ethz.ch>

#############
# Occamy HW #
#############

HERO_OCCAMY_ROOT ?= $(HERO_PLATFORMS_DIR)/occamy
HERO_OCCAMY_BIT := $(HERO_OCCAMY_ROOT)/target/fpga/occamy_vcu128/occamy_vcu128.runs/impl_1/occamy_vcu128_wrapper.bit

# Clone Occamy
$(HERO_OCCAMY_ROOT)/Bender.yml:
	git clone git@github.com:pulp-platform/occamy.git --branch=ck/fpga2 $(dir $@)
.PRECIOUS: $(HERO_OCCAMY_ROOT)/Bender.yml

# (Re)generate RTL
$(HERO_OCCAMY_ROOT)/target/sim/cfg/lru.hjson: $(HERO_OCCAMY_ROOT)/Bender.yml
	make -C $(HERO_OCCAMY_ROOT)/target/sim CFG_OVERRIDE=cfg/single-cluster.hjson rtl
.PRECIOUS: $(HERO_OCCAMY_ROOT)/target/sim/cfg/lru.hjson

# Build a bitstream
HERO_ARTIFACTS_ROOT_occamy-bit := $(HERO_ARTIFACTS_ROOT)/occamy_bit
HERO_ARTIFACTS_VARS_occamy-bit := EXT_JTAG=1 DEBUG=0
HERO_ARTIFACTS_FILES_occamy-bit = $(shell find $(HERO_OCCAMY_ROOT)/target/fpga -type f)
HERO_ARTIFACTS_DATA_occamy-bit := $(HERO_OCCAMY_ROOT)/target/fpga

$(HERO_OCCAMY_BIT): $(HERO_OCCAMY_ROOT)/target/sim/cfg/lru.hjson $(HERO_CVA6_SDK_DIR)/install64/u-boot-spl.bin
	riscv -riscv64-gcc-12.2.0 make -C $(HERO_OCCAMY_ROOT)/target/fpga occamy_vcu128 $(HERO_ARTIFACTS_VARS_occamy_bit)
.PRECIOUS: $(HERO_OCCAMY_BIT)

# Occamy requires a special cva6-sdk output
$(HERO_CVA6_SDK_DIR)/install64/u-boot-spl.bin:
	@echo -e $(TERM_RED)"Missing $@ have you build your cva6-sdk for Occamy?"$(TERM_NC);
	exit 1

# Fetch bitstream artifact (attention the sensitivity list needs $(HERO_OCCAMY_ROOT) to be built
hero-occamy-bit-all-artifacts: $(HERO_OCCAMY_ROOT)/Bender.yml hero-load-artifacts-occamy-bit $(HERO_OCCAMY_BIT) hero-save-artifacts-occamy-bit

# Build bitstream from scratch
hero-occamy-bit-all: $(HERO_OCCAMY_BIT)

hero-occamy-bit-clean:
	make -C $(HERO_OCCAMY_ROOT)/target/fpga/bootrom clean
	make -C $(HERO_OCCAMY_ROOT)/target/fpga/vivado_ips clean
	make -C $(HERO_OCCAMY_ROOT)/target/fpga clean

#############
# Occamy SW #
#############

# Directory of the omptarget device runtime
HERO_OCCAMY_DEVICE_RUNTIME_DIR := $(HERO_OCCAMY_ROOT)/target/sim/sw/device/apps/libomptarget_device

# Generate SW headers for this HW config
$(HERO_OCCAMY_ROOT)/sw/shared/platform/generated: $(HERO_OCCAMY_ROOT)/target/sim/cfg/lru.hjson
	make -C $(HERO_OCCAMY_ROOT)/target/sim all-headers
.PRECIOUS: $(HERO_OCCAMY_ROOT)/sw/shared/platform/generated

# Compile the snitch runtime
$(HERO_OCCAMY_ROOT)/target/sim/sw/device/runtime/build/libsnRuntime.a: $(HERO_OCCAMY_ROOT)/sw/shared/platform/generated FORCE
	make -C $(HERO_OCCAMY_ROOT)/target/sim/sw/device/runtime all
.PRECIOUS: $(HERO_OCCAMY_ROOT)/target/sim/sw/device/runtime/build/libsnRuntime.a

# Compile the libmath for snitch
$(HERO_OCCAMY_ROOT)/target/sim/sw/device/math/build/libmath.a: $(HERO_OCCAMY_ROOT)/sw/shared/platform/generated FORCE
	make -C $(HERO_OCCAMY_ROOT)/target/sim/sw/device/math all
.PRECIOUS: $(HERO_OCCAMY_ROOT)/target/sim/sw/device/math/build/libmath.a

# Compile the openmptarget device runtime
$(HERO_OCCAMY_DEVICE_RUNTIME_DIR)/build/libomptarget_device.a: $(HERO_OCCAMY_ROOT)/target/sim/sw/device/runtime/build/libsnRuntime.a \
                                                               $(HERO_OCCAMY_ROOT)/target/sim/sw/device/math/build/libmath.a \
															   FORCE
	make -C $(HERO_OCCAMY_DEVICE_RUNTIME_DIR) all
.PRECIOUS: $(HERO_OCCAMY_DEVICE_RUNTIME_DIR)/build/libomptarget_device.a

# Phonies
hero-occamy-sw-all: $(HERO_OCCAMY_DEVICE_RUNTIME_DIR)/build/libomptarget_device.a

hero-occamy-sw-clean:
	make -C $(HERO_OCCAMY_ROOT)/target/sim clean-headers clean-sw

###############
# Carfield HW #
###############

HERO_CARFIELD_ROOT ?= $(HERO_PLATFORMS_DIR)/carfield
HERO_SPATZ_ROOT ?= $(HERO_CARFIELD_ROOT)/spatz
HERO_SAFETY_ROOT ?= $(HERO_CARFIELD_ROOT)/safety_island
HERO_SAFETY_BIT := $(HERO_CARFIELD_ROOT)/target/xilinx/out/carfield_bd_vcu128_carfield_l2dual_safe_periph.bit
HERO_SPATZ_BIT := $(HERO_CARFIELD_ROOT)/target/xilinx/out/carfield_bd_vcu128_carfield_l2dual_spatz_periph.bit

HERO_ARTIFACTS_ROOT_safety-bit := $(HERO_ARTIFACTS_ROOT)/safety_bit
HERO_ARTIFACTS_VARS_safety-bit := GEN_NO_HYPERBUS=1 CARFIELD_CONFIG=carfield_l2dual_safe_periph GEN_EXT_JTAG=1 XILINX_FLAVOR=bd VIVADO_MODE=batch XILINX_BOARD=vcu128
HERO_ARTIFACTS_FILES_safety-bit = $(shell find $(HERO_CARFIELD_ROOT)/target/xilinx -type f)
HERO_ARTIFACTS_DATA_safety-bit := $(HERO_CARFIELD_ROOT)/target/xilinx/out

HERO_ARTIFACTS_ROOT_spatz-bit := $(HERO_ARTIFACTS_ROOT)/spatz_bit
HERO_ARTIFACTS_VARS_spatz-bit := GEN_NO_HYPERBUS=1 CARFIELD_CONFIG=carfield_l2dual_spatz_periph GEN_EXT_JTAG=1 XILINX_FLAVOR=bd VIVADO_MODE=batch XILINX_BOARD=vcu128
HERO_ARTIFACTS_FILES_spatz-bit = $(shell find $(HERO_CARFIELD_ROOT)/target/xilinx -type f)
HERO_ARTIFACTS_DATA_spatz-bit := $(HERO_CARFIELD_ROOT)/target/xilinx/out

# Clone carfield
$(HERO_CARFIELD_ROOT)/Bender.yml:
	git clone git@github.com:CyrilKoe/carfield.git --branch=ck/hero_paper $(dir $@)

# Fetch Carfield's islands
$(HERO_SPATZ_ROOT)/Bender.yml $(HERO_SAFETY_ROOT)/Bender.yml: $(HERO_CARFIELD_ROOT)/Bender.yml
	riscv -riscv64-gcc-12.2.0 make -C $(HERO_PLATFORMS_DIR)/carfield car-init-all

### SAFETY HW ###

$(HERO_SAFETY_BIT): $(HERO_SPATZ_ROOT)/Bender.yml $(HERO_SAFETY_ROOT)/Bender.yml
	riscv -riscv64-gcc-12.2.0 make -C $(HERO_CARFIELD_ROOT) car-xil-all $(HERO_ARTIFACTS_VARS_safety-bit)

hero-safety-bit-all: $(HERO_SAFETY_BIT)

hero-safety-bit-all-artifacts: $(HERO_SAFETY_ROOT)/Bender.yml hero-load-artifacts-safety-bit $(HERO_SAFETY_BIT) hero-save-artifacts-safety-bit

hero-safety-bit-clean:
	make -C $(HERO_CARFIELD_ROOT) car-xil-clean

hero-safety-boot: $(HERO_CVA6_SDK_DIR)/install64/vmlinux
	@if [ ! -f "$(HERO_SAFETY_BIT)" ]; then \
		echo "Missing bitstream, run hero-safety-bit-all[-artifacts] to build it"; \
		exit 1; \
	fi
	ln -sf $(HERO_CVA6_SDK_DIR)/install64 $(HERO_CARFIELD_ROOT)/sw/boot
	scp $(HERO_CVA6_SDK_DIR)/install64/uImage vcu128-01@bordcomputer:/srv/tftp/vcu128-01/carfield/${USER}-uImage
	sed -i -e 's/\[ 00 00 00 00 00 00 \]/$(XILINX_MAC_ADDR)/' $(HERO_CARFIELD_ROOT)/sw/boot/mac_address.dtsi
	printf "remote-boot = \"129.132.24.199:vcu128-01/carfield/${USER}-uImage\";" > $(HERO_CARFIELD_ROOT)/sw/boot/remote_boot.dtsi
	riscv -riscv64-gcc-12.2.0 make -C $(HERO_CARFIELD_ROOT) car-xil-flash $(HERO_ARTIFACTS_VARS_safety-bit) XILINX_BOOT_ETH=1
	make -C $(HERO_CARFIELD_ROOT) car-xil-program $(HERO_ARTIFACTS_VARS_safety-bit)

### SPATZ HW ###

$(HERO_SPATZ_BIT): $(HERO_SPATZ_ROOT)/Bender.yml $(HERO_SAFETY_ROOT)/Bender.yml
	riscv -riscv64-gcc-12.2.0 make -C $(HERO_CARFIELD_ROOT) car-xil-all $(HERO_ARTIFACTS_VARS_spatz-bit)

hero-spatz-bit-all: $(HERO_SPATZ_BIT)

hero-spatz-bit-all-artifacts: $(HERO_SPATZ_ROOT)/Bender.yml hero-load-artifacts-spatz-bit $(HERO_SPATZ_BIT) hero-save-artifacts-spatz-bit

hero-spatz-bit-clean:
	make -C $(HERO_CARFIELD_ROOT) car-xil-clean

hero-spatz-boot: $(HERO_CVA6_SDK_DIR)/install64/vmlinux
	@if [ ! -f "$(HERO_SPATZ_BIT)" ]; then \
		echo "Missing bitstream, run hero-spatz-bit-all[-artifacts] to build it"; \
		exit 1; \
	fi
	ln -sf $(HERO_CVA6_SDK_DIR)/install64 $(HERO_CARFIELD_ROOT)/sw/boot
	scp $(HERO_CVA6_SDK_DIR)/install64/uImage vcu128-01@bordcomputer:/srv/tftp/vcu128-01/carfield/${USER}-uImage
	sed -i -e 's/\[ .* \]/$(XILINX_MAC_ADDR)/' $(HERO_CARFIELD_ROOT)/sw/boot/mac_address.dtsi
	printf "remote-boot = \"129.132.24.199:vcu128-01/carfield/${USER}-uImage\";" > $(HERO_CARFIELD_ROOT)/sw/boot/remote_boot.dtsi
	riscv -riscv64-gcc-12.2.0 make -C $(HERO_CARFIELD_ROOT) car-xil-flash $(HERO_ARTIFACTS_VARS_spatz-bit) XILINX_BOOT_ETH=1
	make -C $(HERO_CARFIELD_ROOT) car-xil-program $(HERO_ARTIFACTS_VARS_spatz-bit)

####################
# Safety Island SW #
####################

$(HERO_SAFETY_ROOT)/sw/tests/runtime_omp/build/libomptarget_runtime.a: $(HERO_SAFETY_ROOT)/Bender.yml FORCE
	bash -c "source $(HERO_SAFETY_ROOT)/env/env_carfield.sh ; riscv -pulp-gcc-2.6.0 make -C $(HERO_SAFETY_ROOT)/sw/tests/runtime_omp archive_app"

hero-safety-sw-all: $(HERO_SAFETY_ROOT)/sw/tests/runtime_omp/build/libomptarget_runtime.a

hero-safety-sw-clean:
	make -C $(HERO_SAFETY_ROOT)/sw/tests/runtime_omp clean


############
# Spatz SW #
############

$(HERO_SPATZ_ROOT)/hw/system/spatz_cluster/sw/build/spatzBenchmarks/libomptarget.a: FORCE
	LLVM_INSTALL_DIR=$(HERO_INSTALL) GCC_INSTALL_DIR=/usr/pack/riscv-1.0-kgf/pulp-gcc-2.6.0 \
	SPATZ_CLUSTER_CFG=$(HERO_SPATZ_ROOT)/hw/system/spatz_cluster/cfg/carfield.hjson \
	make -C $(HERO_SPATZ_ROOT)/hw/system/spatz_cluster sw

hero-spatz-sw-all: $(HERO_SPATZ_ROOT)/hw/system/spatz_cluster/sw/build/spatzBenchmarks/libomptarget.a

hero-spatz-sw-clean:
	make -C $(HERO_SPATZ_ROOT)/hw/system/spatz_cluster clean.sw

.PHONY: hero-occamy-bit-all hero-occamy-bit-all-artifacts hero-occamy-bit-clean hero-occamy-sw-all hero-occamy-sw-clean
.PHONY: hero-safety-bit-all hero-safety-bit-all-artifacts hero-safety-bit-clean hero-safety-sw-all hero-safety-sw-clean hero-safety-boot
