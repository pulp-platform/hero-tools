# Milk-V

## Platform

...

The SG2042 can be used as an host to your PCIe accelerator. In this configuration CVA6 stays dormant, and SG2042 interracts with the accelerator directly.

## Get started

In order to offload kernels to a PCIe platform using the HeroSDK, you need to mount this platform's device tree into your host system.

### Patching your kernel

Usually, PCIe devices do not appear in the device tree of your Linux machine. But [Linux 6.6-rc5](https://lore.kernel.org/lkml/CAHk-=wh7awamHE3ujoxZFcGMg3wvLTk8UZYSm3m7vUDhpmP2+A@mail.gmail.com/) integrates patches for this new feature. These patches have been cherry-picked on top of Linux 6.1.22 in this [Kernel fork](https://iis-git.ee.ethz.ch/hero/linux-hero-server).

```bash
# Clone the fork on the Milk-V
git clone git@iis-git.ee.ethz.ch:hero/linux-hero-server.git
# Build your kernel
make build -j32
```

__Note: the patch above does not apply for every PCIe device by default, it activates only for Occamy. If you have another PCIe device with a different PCIe device ID, please add it similarly to [this commit](https://iis-git.ee.ethz.ch/hero/linux-hero-server/-/commit/700368083b2e0d5438e3be553401db1d06fd7bee).__

Once this is done you can install it

```bash
# Mount the boot partition
sudo mount /dev/nvme0n1p2 /boot 
# Install your kernel
sudo make modules_install
sudo make install
```

Now you can add a new entry in `/boot/extlinux/extlinux.conf`

```
label fc38-custom
        menu label Fedora-riscv64-Sophgo (6.1.22+)
        linux /vmlinuz-6.1.22+
        initrd /initramfs-6.1.22+.img
        append console=ttyS0,115200 root=UUID=5c425ea1-80f9-4dc9-b62d-12eea45ed1d0 rootfstype=ext4 rootwait rw earlycon debug selinux=0 LANG=en_US.UTF-8
```

You can now reboot your system and select this new kernel. If the FPGA is programmed and connected via PCIe, you can oberve the new `dev` entry in the device tree:

```bash
tree /proc/device-tree/*pcie* | less
# [Look for dev@]
```

### Modify the Milk-V device tree

In order for HeroSDK to work on the Milk-V you will also need to use device tree overlays. This requires to modify the SG2042's device tree.

We will do this step in a (compatible) MicroSD card to avoid modifying the bootcode in flah with unofficial code.

Download a pre-compiled live boot image [here](https://milkv.io/docs/pioneer/getting-started/InstallOS).

Flash SD-card with it.

```bash
# Find your SD card block device sdX
lsblk
# Flash the extracted image to it (up to one hour)
sudo dd if=data of=/dev/sdX bs=512 status=progress 
```

Now you need to modify the device tree within this SD-card.

```bash
# Create a work directory
mkdir hero-milkv-sdcard/mnt/efi && cd hero-milkv-sdcard
# Mount the SD card's EFI partition
sudo mount /dev/sdX1 mnt/efi
# Decompile the device tree from there
sudo cp mnt/efi/riscv64/mango-milkv-pioneer.dtb mango-milkv-pioneer.dtb
dtc -I dtb -O dts mango-milkv-pioneer.dtb > mango-milkv-pioneer.dts
# Now recompile with the -@ flag to allow device tree overlay
dtc -@ mango-milkv-pioneer.dts -o mango-milkv-pioneer.new.dtb
# Now copy back to the sd card this modified device tree
sudo cp mango-milkv-pioneer.new.dtb mnt/efi/riscv64/mango-milkv-pioneer.dtb
# Carefully unmount and remove the SD card
sudo umunt mnt/efi
```

You can now insert the SD card in the Milk-V and boot! Make sure you select the modified boot image installed above (on the disk) in the bootloader.

Verify that it works:

```bash
# You should have a new device-tree entry
ls /proc/device-tree/__symbols__/
```

You are now ready for to apply device tree overlays!

### Insert the accelerator's device tree

The next steps may be platform specific, go back to [Platforms](../index.md) to continue.
