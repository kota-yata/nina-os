# #!/usr/bin/env bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# SRC_QEMU_PATH="${SCRIPT_DIR}/build/qemu-system-x86_64"
SRC_QEMU_PATH="/usr/bin/qemu-system-riscv64"
SRC_MONITOR_SOCKET="./qemu-monitor-socket"
SRC_BRIDGE="qemubr0"
SRC_BRIDGE_CONF="/etc/qemu/bridge.conf"
SRC_BRIDGE_HELPER="/usr/lib/qemu/qemu-bridge-helper"
SRC_VNC=61 #5961

VM_IP_ADDR="203.178.128.63"
VM_MAC_ADDR="52:54:00:12:34:56"
VM_MEMORY="16384"
VM_VCPU="4"
VM_IMG="./u24.img"
VM_USERDATA="/mnt/cosocaf-24f/user-data.img"

if ! getcap $(realpath $SRC_BRIDGE_HELPER) | grep cap_net_admin=ep; then
  echo "sudo setcap cap_net_admin+ep '$(realpath $SRC_BRIDGE_HELPER)'"
  exit 1
fi

if [ -e "$SRC_MONITOR_SOCKET" ]; then
  rm -f "$SRC_MONITOR_SOCKET"
fi

if ! grep -x "allow $SRC_BRIDGE" $SRC_BRIDGE_CONF; then
  mkdir -p $(dirname $SRC_BRIDGE_CONF)
  echo "allow $SRC_BRIDGE" >> $SRC_BRIDGE_CONF
fi

wait_for_socket() {
  local socket=$1
  echo "Waiting for socket: $socket"
  while [ ! -e "$socket" ]; do
    sleep 0.1
  done
  echo "Socket $socket is ready!"
}

echo "Launching guest OS..."

$SRC_QEMU_PATH \
  -m $VM_MEMORY \
  -d unimp,guest_errors -trace virtio_* -D qemu-u24.log \
  -machine virt \
  -smp $VM_VCPU \
  -vnc 0.0.0.0:$SRC_VNC \
  -k ja \
  -name debug-threads=on \
  -bios /usr/lib/riscv64-linux-gnu/opensbi/generic/fw_jump.elf \
  -kernel /usr/lib/u-boot/qemu-riscv64_smode/uboot.elf \
  -monitor unix:$SRC_MONITOR_SOCKET,server,nowait \
  -netdev bridge,id=net0,br=$SRC_BRIDGE,helper=$SRC_BRIDGE_HELPER \
  -device virtio-net-device,netdev=net0,mac=$VM_MAC_ADDR \
  -object filter-dump,id=f1,netdev=net0,file=virtio-net-u24.pcap\

# -drive file=$VM_IMG,format=raw,if=virtio \
  # -drive file=$VM_USERDATA,format=raw \
#-global virtio-mmio.force-legacy=false

SRC_QEMU_PID=$!

echo "Waiting for source QEMU socket..."
wait_for_socket $SRC_MONITOR_SOCKET
socat STDIO UNIX-CONNECT:$SRC_MONITOR_SOCKET <&0

wait $SRC_QEMU_PID
