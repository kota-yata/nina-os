#!/bin/bash
set -xue
QEMU=qemu-system-riscv32
CC=clang

CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32 -ffreestanding -nostdlib"

OBJCOPY=/usr/bin/llvm-objcopy

# building shell
$CC $CFLAGS -Wl,-Tuser.ld -Wl,-Map=shell.map -o shell.elf shell.c user.c common.c
# elf to bin with meory layout
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary shell.elf shell.bin
$OBJCOPY -Ibinary -Oelf32-littleriscv shell.bin shell.bin.o

# building kernel
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
    kernel.c common.c virtio.c shell.bin.o

$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
  -d unimp,guest_errors,int,cpu_reset -D qemu.log \
  -netdev tap,id=net0,ifname=tap0\
  -device virtio-net-device,netdev=net0,bus=virtio-mmio-bus.0,mac=52:54:00:12:34:56 \
  -kernel kernel.elf

# -drive id=drive0,file=lorem.txt,format=raw,if=none \
# -device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0 \
