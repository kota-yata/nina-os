from scapy.all import Ether, sendp

packet = Ether(dst="00:00:40:01:f9:06") / "Hello"
sendp(packet, iface="qemubr0")
