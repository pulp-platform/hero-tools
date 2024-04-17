# Occamy

## Platform

...

## Get started

### Prior notes

```bash
# Before starting make sure you set up your environment on every terminal you use!
source scripts/setenv.sh
# Set up your FPGA specific variables too
source scripts/[your_fpga].sh
```

```bash
# Before starting make sure you are on the correct branch (the CVA6-SDK is platform-specific at the moment)
git checkout occamy/main
git submodule update --init --recursive
# If you have old builds from other branches in the CVA6-sdk, it may worth deep cleaning your CVA6-SDK
cd cva6-sdk
git clean -fdx
git submodule foreach "git clean -fdx"
```

### U-boot SPL

Now let's look at Occamy emulated on a VCU128.

For Occamy (and not for Cheshire based system), we use U-boot SPL within the bootrom to boot CVA6.
So you will need to build U-boot (and the CVA6-sdk) before being able to compile the bitstream:

```bash
# Build the Linux images (and the GCC Linux toolchain if needed)
make hero-cva6-sdk-all
# Note if you already have the toolchain saved in your artifacts folder you can get it before with
make hero-tc-gcc-artifacts
```

### Occamy bitstream

Now you can clone the Occamy repository and fetch or build your bitstream:

```bash
# Fetch from previous artifacts
make hero-occamy-bit-artifacts
# Force build
make hero-occamy-bit
```

### Boot on CVA6

### Use on Milk-V
