# Hero-Tools

This repository contains the Hero(-Tools) software stack that support heterogeneous computing for RISC-V based platforms. Today, Hero uses the OpenMP API to offload from a RV64 host to a RV32 device.

The Hero-Tools contain two compilers (Linux GCC & Bare metal LLVM), Linux drivers, host and device runtimes to fully use our multiple heterogeneous platforms. At the moment the supported devices are: the [Occamy](https://github.com/pulp-platform/occamy/) Snitch Cluster, the [Carfield](https://github.com/pulp-platform/carfield/) Safety-Island.

First, fetch the required repositories :

```bash
git submodule update --init --recursive
```

## This repository

Tihs repository contains the following directories:

| Directory   | Contains                                                                                         |
| ----------- | ------------------------------------------------------------------------------------------------ |
| `apps`      | Some example applications using OpenMP                                                           |
| `artifacts` | The scripts and outputs of the artifact management system                                        |
| `cva6-sdk`  | CVA6 SDK submodule containing Linux and the platform drivers                                     |
| `platform`  | The hardware platforms available (cloned on demand)                                              |
| `scripts`   | Helper scripts                                                                                   |
| `sw`        | Hero runtime library, built (rv64) LLVM libraries and built (rv64) OpenMP target runtime library |
| `toolchain` | LLVM fork containing the OpenMP taget runtime library implementation                             |


## Getting started

To build the software stack, compile an FPGA bitstream, get a Linux image, and more, go to [Getting Started](gs.md).
