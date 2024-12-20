# Copyright 2024 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Cyril Koenig <cykoenig@iis.ee.ethz.ch>

export HERO_ROOT=$(pwd)
export CVA6_SDK=$HERO_ROOT/cva6-sdk
export HERO_INSTALL=$HERO_ROOT/install
export RISCV=$CVA6_SDK/buildroot/output/host/
export RV64_SYSROOT=$RISCV/riscv64-buildroot-linux-gnu/sysroot
export CROSS_COMPILE=$RISCV/bin/riscv64-buildroot-linux-gnu-

# Parse the device Linux path from buildroot
if [ -d $CVA6_SDK/buildroot/output/build ]; then
export BR_LINUX_DIR=$CVA6_SDK/buildroot/output/$(make -C cva6-sdk/buildroot linux-show-info \
                    | python3 -c "import sys, json; print(json.loads(sys.stdin.readlines()[1])['linux']['build_dir'])")
fi

# For Occamy bootrom
export UBOOT_SPL_BIN=$CVA6_SDK/install64/u-boot-spl.bin
export UBOOT_ITB=$CVA6_SDK/install64/u-boot.itb

# Add linux-gcc and clang to path
export PATH=$RISCV/bin:$PATH
export PATH=$HERO_INSTALL/bin:$PATH

export HERO_ARTIFACTS_ROOT=/usr/scratch2/wuerzburg/cykoenig/memora/hero

export CARFIELD_ROOT=$HERO_ROOT/platforms/carfield
export OCCAMY_ROOT=$HERO_ROOT/platforms/occamy
