# HeroSDK

HeroSDK is an open-research software development kit for heterogeneous RISC-V platforms. This project enables deploying accelerated Linux user space applications using OpenMP target for programmable multi-core clusters. We currently target [Carfield](https://github.com/pulp-platform/carfield/) and [Occamy](https://github.com/pulp-platform/occamy/).

__HeroSDK is a continuity of Hero and HeroV2 but both projects are not yet compatible. Use the [original Hero repository](https://github.com/pulp-platform/hero) for everything referred to [HeroV2](https://arxiv.org/abs/2201.03861).__

HeroSDK is developed as part of the [PULP project](https://pulp-platform.org/), a joint effort between ETH Zurich and the University of Bologna.

## This repository

The HeroSDK contains compilers (Linux GCC & Bare metal LLVM), Linux kernel modules, host and device runtimes. See the figure felow for more informations.

_(Figure disclosed once the paper is on Arxiv)_

## This repository

First, fetch the required repositories:

```bash
git submodule update --init --recursive
```

This repository contains the following directories:

| Directory    | Contains                                                                                         |
| ------------ | ------------------------------------------------------------------------------------------------ |
| `apps`       | Some example applications using OpenMP                                                           |
| `artifacts`  | Automatically managed artifact cache                                                             |
| `cva6-sdk`   | The cva6-sdk git submodule containing Buildroot                                                  |
| `docs`       | Files used to build the documentation                                                            |
| `install`*   | LLVM and compiled binaries and libraries. Also contains device's compiled newlibs.               |
| `output`*    | Intermediary compilation folder for LLVM and newlibs.                                            |
| `platforms`* | The hardware platforms available (cloned on demand)                                              |
| `scripts`    | Helper scripts                                                                                   |
| `sw`         | Hero runtime library, LLVM libraries, the OpenMP target runtime library, and the device drivers  |
| `toolchain`  | An LLVM fork containing the Hero OpenMP target runtime library implementation                    |
_* generated files_
## Getting started

To build the software stack, compile a FPGA bitstream, get a Linux image, and more, go to [Getting Started](gs.md).

## Dependencies

This project contains the following dependancies:
- [CVA6-sdk](https://github.com/pulp-platform/cva6-sdk/) to build Linux images using [Buildroot](https://buildroot.org/).
- [llvm-project](https://github.com/llvm/llvm-project) to build the heterogeneous compiler and OpenMP target runtime.
- [o1heap](https://github.com/pavel-kirienko/o1heap) to manage dynamic memory allocation for device.

## License

Unless specified otherwise in the respective file headers, all code checked into this repository is made available under a permissive license. All software sources are licensed under Apache 2.0.

## References

_(Added once the paper is on Arxiv)_
