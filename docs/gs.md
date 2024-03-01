# Getting Started

In this tutorial we take 

## Dependencies

To *build* HeroTools, you will need:

- Cmake `>= 3.18.1`
- Host GCC `>= 9.2.0`

## Building the Linux image

You will first need to build the RISCV64 Linux GCC compiler and a Linux image with CVA6-SDK. This GCC compiler
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

The command below will compile the libhero (briding between the hardware drivers and the OpenMP runtime), the libllvm (required for the OpenMP runtime), and the the OpenMP target host runtime itself.
All these libraries are compiled for the host (RV64) using the GCC / LLVM compiler previously built.

```bash
make hero-sw-all
```

## Start your platform on an FPGA

Now, let's fetch a bitstream. We will use the Safety-Island emulated on a VCU128 for this tutorial.

First fetch the Carfield repository and your bitstream:

```bash
make hero-safety-bit-all-artifacts
```

Then let's add all the host software to the Linux Image.

```bash
make hero-sw-deploy
make hero-cva6-sdk-clean
make hero-cva6-sdk-all
```
