from scapy.all import Ether, sendp

packet = Ether(dst="52:54:00:12:34:56") / "Hello"
sendp(packet, iface="qemubr0")
