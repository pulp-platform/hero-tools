## Hero tools

### Compile the GCC toolchain and the Kernel

```bash
git submodule update --init --recursive
make tc-gcc
```

### Compile the cross LLVM toolchain (x86 to RISCV)

```bash
# Building LLVM requires "recent" GCC
export CC=/usr/pack/gcc-9.2.0-af/linux-x64/bin/gcc && export CXX=/usr/pack/gcc-9.2.0-af/linux-x64/bin/g++
export HERO_INSTALL=`pwd`/install && export PATH=$HERO_INSTALL/share:$HERO_INSTALL/bin:$PATH
make tc-llvm
# Compile also newlib rv32
make tc-snitch
```

### Compile the supports

```bash
make sw
```