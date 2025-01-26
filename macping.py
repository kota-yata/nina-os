from scapy.all import Ether, sendp

packet = Ether(dst="ff:ff:ff:ff:ff:ff") / "Hello"
sendp(packet, iface="qemubr0")
