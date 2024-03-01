#!/bin/bash -xe

# Copyright 2022 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

THIS_DIR=$(dirname "$(readlink -f "$0")")

if [ -z "$CC" ]; then
  export CC=`which gcc`
fi
if [ -z "$CXX" ]; then
  export CXX=`which g++`
fi
if [ -z "$CMAKE" ]; then
  export CMAKE=`which cmake`
fi
if [ -z "$MARCH" ]; then
  export MARCH=rv32ima
fi
if [ -z "$MABI" ]; then
  export MABI=ilp32
fi

echo "Requesting C compiler $CC"
echo "Requesting CXX compiler $CXX"
echo "Requesting cmake $CMAKE"

##############################
# newlib
##############################

[ ! -d newlib-rv32-${MARCH}-${MABI} ] && git clone --depth 1 -b newlib-3.3.0 https://sourceware.org/git/newlib-cygwin.git newlib-rv32-${MARCH}-${MABI}

# Newlib for rv32
cd newlib-rv32-${MARCH}-${MABI}

./configure                                           \
    --target=riscv32-unknown-elf                      \
    -nfp                                              \
    --prefix=${HERO_INSTALL}/${MARCH}-${MABI}/        \
    AR_FOR_TARGET=${HERO_INSTALL}/bin/llvm-ar         \
    AS_FOR_TARGET=${HERO_INSTALL}/bin/llvm-as         \
    LD_FOR_TARGET=${HERO_INSTALL}/bin/llvm-ld         \
    RANLIB_FOR_TARGET=${HERO_INSTALL}/bin/llvm-ranlib \
    CC_FOR_TARGET="${HERO_INSTALL}/bin/clang -march=${MARCH} -mabi=${MABI}" \
    CFLAGS_FOR_TARGET="-mno-relax -DMALLOC_PROVIDED=1"

# Build newlib
make
make install

cd ..

##############################
# compiler-rt
##############################
mkdir -p compiler-rt32-${MARCH}-${MABI}
cd compiler-rt32-${MARCH}-${MABI}
# NOTE: CMAKE_SYSTEM_NAME is set to linux to allow the configure step to
#       correctly validate that clang works for cross compiling

ls ../llvm_build/bin/
${CMAKE} -G"Unix Makefiles"                                                  \
    -DCMAKE_SYSTEM_NAME=Linux                                                \
    -DCMAKE_INSTALL_PREFIX=$(${HERO_INSTALL}/bin/clang -print-resource-dir)/${MARCH}-${MABI}/  \
    -DCMAKE_C_COMPILER=${HERO_INSTALL}/bin/clang                             \
    -DCMAKE_CXX_COMPILER=${HERO_INSTALL}/bin/clang                           \
    -DCMAKE_AR=${HERO_INSTALL}/bin/llvm-ar                                   \
    -DCMAKE_NM=${HERO_INSTALL}/bin/llvm-nm                                   \
    -DCMAKE_RANLIB=${HERO_INSTALL}/bin/llvm-ranlib                           \
    -DCMAKE_C_COMPILER_TARGET="riscv32-unknown-elf"                          \
    -DCMAKE_CXX_COMPILER_TARGET="riscv32-unknown-elf"                        \
    -DCMAKE_ASM_COMPILER_TARGET="riscv32-unknown-elf"                        \
    -DCMAKE_C_FLAGS="-march=${MARCH} -mabi=${MABI} -mno-relax"               \
    -DCMAKE_CXX_FLAGS="-march=${MARCH} -mabi=${MABI} -mno-relax"             \
    -DCMAKE_ASM_FLAGS="-march=${MARCH} -mabi=${MABI} -mno-relax"             \
    -DCMAKE_EXE_LINKER_FLAGS="-nostartfiles -nostdlib -fuse-ld=lld"          \
    -DCMAKE_SYSROOT="${HERO_INSTALL}/${MARCH}-${MABI}/riscv32-unknown-elf"   \
    -DCOMPILER_RT_BAREMETAL_BUILD=ON                                         \
    -DCOMPILER_RT_BUILD_BUILTINS=ON                                          \
    -DCOMPILER_RT_BUILD_MEMPROF=OFF                                          \
    -DCOMPILER_RT_BUILD_LIBFUZZER=OFF                                        \
    -DCOMPILER_RT_BUILD_PROFILE=OFF                                          \
    -DCOMPILER_RT_BUILD_SANITIZERS=OFF                                       \
    -DCOMPILER_RT_BUILD_XRAY=OFF                                             \
    -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON                                     \
    -DCOMPILER_RT_BUILTINS_HIDE_SYMBOLS=ON                                   \
    -DCOMPILER_RT_USE_BUILTINS_LIBRARY=ON                                    \
    -DCOMPILER_RT_OS_DIR=""                                                  \
    -DLLVM_CONFIG_PATH=../llvm_build/bin/llvm-config                         \
    ../../../toolchain/llvm-project/compiler-rt
${CMAKE} --build . --target install
cd ..

##############################
# Symlinks
##############################

# Add symlinks to LLVM tools
cd ${HERO_INSTALL}/bin
for TRIPLE in riscv32-unknown-elf; do
  for TOOL in clang clang++ cc c++; do
    ln -fsv clang ${TRIPLE}-${TOOL}
  done
  ln -fsv llvm-ar ${TRIPLE}-ar
  ln -fsv llvm-objdump ${TRIPLE}-objdump
  ln -fsv llvm-readelf ${TRIPLE}-readelf
done
