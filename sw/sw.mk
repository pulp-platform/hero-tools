# Copyright 2024 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Cyril Koenig <cykoenig@iis.ee.ethz.ch>

# Hero targets
HERO_RUNTIME_DEVICES := safety_island spatz_cluster occamy

###########
# Host SW #
###########

# Compile LLVM Support for rv64 (needed for libomp)
$(HERO_SW_DIR)/libllvm/lib/libLLVMSupport.a:
	make -C $(HERO_SW_DIR)/libllvm

# Compile libhero-[device] layer
$(HERO_SW_DIR)/libhero/lib/libhero_%.so:
	make PLATFORM=$* -C $(HERO_SW_DIR)/libhero

# Compile libomp/libomptarget for rv64 and a specific libomptarget-hero-[device] runtime 
$(HERO_SW_DIR)/libomp/lib/libomptarget.rtl.herodev_%.so: $(HERO_SW_DIR)/libllvm/lib/libLLVMSupport.a $(HERO_SW_DIR)/libhero/lib/libhero_%.so
	make PLATFORM=$* -C $(HERO_SW_DIR)/libomp

#############
# Device SW #
#############

# No device SW is currently available here, they can 
# be found directly in the HW platforms repositories (see platforms.mk) 

###########
# PHONIES #
###########

hero-sw-clean-devices:
	make -C $(HERO_SW_DIR)/libomp clean
	make -C $(HERO_SW_DIR)/libhero clean
	make -C $(HERO_SW_DIR)/hero-pcie-dts-driver clean
hero-sw-clean-all: hero-sw-clean-devices
	make -C $(HERO_SW_DIR)/libllvm clean

hero-sw-all: $(foreach dev,$(HERO_RUNTIME_DEVICES),$(HERO_SW_DIR)/libhero/lib/libhero_$(dev).so $(HERO_SW_DIR)/libomp/lib/libomptarget.rtl.herodev_$(dev).so)

hero-sw-deploy: $(foreach dev,$(HERO_RUNTIME_DEVICES),$(HERO_SW_DIR)/libhero/lib/libhero_$(dev).so $(HERO_SW_DIR)/libomp/lib/libomptarget.rtl.herodev_$(dev).so)
	mkdir -p $(HERO_CVA6_SDK_DIR)/rootfs/usr/lib
	cp $(HERO_SW_DIR)/libomp/lib/*.so $(HERO_CVA6_SDK_DIR)/rootfs/usr/lib
	cp $(HERO_SW_DIR)/libhero/lib/*.so $(HERO_CVA6_SDK_DIR)/rootfs/usr/lib

.PHONY: hero-sw-clean-devices hero-sw-clean-all hero-sw-all
