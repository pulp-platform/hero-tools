# Getting Started

The central components of HeroSDK are the CVA6 Linux image and the LLVM toolchain, we will start by compiling them,.

## Prior notes

```bash
# Before starting make sure you set up your environment on every terminal you use!
source scripts/setenv.sh
# Set up your FPGA specific variables too
source scripts/[your_fpga].sh
```

```bash
# Before starting make sure you are on the correct branch (the CVA6-SDK is platform-specific at the moment)
git checkout [platform]/main
git submodule update --init --recursive
# If you have old builds from other branches in the CVA6-sdk, it may worth deep cleaning it
cd cva6-sdk
git clean -fdx
git submodule foreach "git clean -fdx"
```

## Building the Linux image

You will first need to build the RISCV64 Linux GCC compiler and a Linux image with the CVA6-SDK. This GCC compiler
serves to compile the kernel and drivers, is will also provides a RISCV64 standard library to the LLVM toolchain later.
This Linux image will run on the platform's host (CVA6).

You can fetch the GCC toolchain with

```bash
# Fetch from previous artifacts
make hero-tc-gcc-artifacts
# Force build
make hero-tc-gcc
```

You can already build a Linux image for your platform:

```bash
make hero-cva6-sdk-all
```

This will create multiple files in `cva6-sdk/install64` including:

- fw_payload.bin: OpenSBI with U-boot as a payload
- uImage: A Linux image ready to be read by u-boot (from a flash/file server)
- vmlinux: An intermediate result of the uImage, this can be used to obtain debug symbols

## LLVM

In order to build heterogeneous applications, you need to build the heterogeneous LLVM containing the OpenMP Hero runtime plugin.

```bash
# Fetch from previous artifacts
make hero-tc-llvm-artifacts
# Force build
make hero-tc-llvm
```

## RISC-V 64 software runtimes

The command below will compile the libhero (bridging between the hardware drivers and the OpenMP runtime), the libllvm (required for the OpenMP target runtime), and the the OpenMP target host runtime itself.
All these libraries are compiled for the host (RV64) using the GCC / LLVM compiler previously built.

```bash
make hero-sw-all
```

## Start your platform on an FPGA

Go to [Targets](platforms/index.md) and pick the architecture you want to use.
