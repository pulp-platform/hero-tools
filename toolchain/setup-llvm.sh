# Copyright 2024 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

#!/bin/bash -xe

# This script is usually executed in output/tc-llvm
# THIS_DIR holds the script's path (toolchain)
THIS_DIR=$(dirname "$(readlink -f "$0")")

# Check installation path
if [ -z "$HERO_INSTALL" ]; then
    echo "Fatal error: set HERO_INSTALL to install location of the toolchain"
    exit 1
fi
mkdir -p $HERO_INSTALL

BUILD_TYPE="Release"
if [ ! -z "$1" ]; then
    BUILD_TYPE=$1
fi

# Use all procs
if [ "x${PARALLEL_JOBS}" == "x" ]; then
  PARALLEL_JOBS=$(nproc)
fi

# If CC and CXX are unset, set them to default values.
if [ -z "$CC" ]; then
  if [-f /etc/iis.version]; then
    export CC=/usr/pack/gcc-9.2.0-af/linux-x64/bin/gcc
  else
    export CC=`which gcc`
  fi
fi
if [ -z "$CXX" ]; then
  if [-f /etc/iis.version]; then
    export CXX=/usr/pack/gcc-9.2.0-af/linux-x64/bin/g++
  else
    export CXX=`which g++`
  fi
fi
if [ -z "$CMAKE" ]; then
  export CMAKE=`which cmake`
fi

echo "Requesting C compiler $CC"
echo "Requesting CXX compiler $CXX"
echo "Requesting cmake $CMAKE"

# clean environment when running together with an env source script
unset HERO_PULP_INC_DIR
unset HERO_LIBPULP_DIR
# remove HERO_INSTALL subdirectories from path to prevent incorrect sysroot to be located
PATH=`sed "s~${HERO_INSTALL}[^:]*:~~g" <<< $PATH`

# setup llvm build
mkdir -p llvm_build
cd llvm_build

# run llvm build and install
echo "Building LLVM project"
# Notes:
# - Use the cmake from the host tools to ensure a recent version.
# - Do not build PULP libomptarget offloading plugin as part of the LLVM build on the *development*
#   machine.  That plugin will be compiled for each Host architecture through a Buildroot package.
${CMAKE} \
    -DCMAKE_BUILD_TYPE="Debug" -DLLVM_ENABLE_ASSERTIONS=OFF \
    -DBUILD_SHARED_LIBS=True \
    -DCMAKE_INSTALL_PREFIX=${HERO_INSTALL} \
    -DLLVM_ENABLE_PROJECTS="clang;openmp;lld" \
    -DLLVM_TARGETS_TO_BUILD="RISCV" \
    -DLLVM_DEFAULT_TARGET_TRIPLE="riscv32-unknown-elf" \
    -DLLVM_ENABLE_LLD=False -DLLVM_APPEND_VC_REV=ON \
    -DLLVM_OPTIMIZED_TABLEGEN=True \
    -DCMAKE_C_COMPILER=$CC \
    -DCMAKE_CXX_COMPILER=$CXX \
    -G Ninja $THIS_DIR/llvm-project/llvm
ninja -j ${PARALLEL_JOBS}
ninja install
cd ..

mkdir -p llvm_passes
cd llvm_passes
echo "Building Custom LLVM passes"
${CMAKE} -G Ninja -DCMAKE_INSTALL_PREFIX=$HERO_INSTALL \
      -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DLLVM_DIR:STRING=$HERO_INSTALL/lib/cmake/llvm \
      -DCMAKE_C_COMPILER=$CC \
      -DCMAKE_CXX_COMPILER=$CXX \
      $THIS_DIR/llvm-support/
${CMAKE} --build . --target install
cd ..

# Install wrapper script (this script invokes custom passes)
# FIXME: this wrapper script should be transparantly included in the HC compiler
echo "Installing hc-omp-pass wrapper script"
cp $THIS_DIR/llvm-support/hc-omp-pass $HERO_INSTALL/bin
