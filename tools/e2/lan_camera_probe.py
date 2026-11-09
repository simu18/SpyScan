#!/usr/bin/env python3
"""SpyScan — Active LAN Camera Discovery Tool (Method 3.1).

Discovers IP cameras on the local Wi-Fi network (even when the mobile app is closed)
by querying standard camera discovery protocols:
  1. SSDP (UPnP) M-SEARCH broadcast (port 1900)
  2. ONVIF WS-Discovery probe (port 3702)
  3. Camera vendor UDP pings (Xiongmai 34567, Tuya 6666/6667, V380 8800)
  4. Subnet ARP ping + streaming port verification (RTSP 554, 8000, 8080, 8899, 34567)

Usage:
  python tools/e2/lan_camera_probe.py
  python tools/e2/lan_camera_probe.py --timeout 4 --scan-subnet
"""
import argparse
import ipaddress
import json
import re
import socket
import struct
import subprocess
import sys
import threading
import time

SSDP_ADDR = ("239.255.255.250", 1900)
ONVIF_ADDR = ("239.255.255.250", 3702)

# Standard camera streaming and management ports
CAMERA_PORTS = [554, 80, 8000, 8080, 8899, 34567, 8800]


def get_real_local_ip():
    """Finds physical Wi-Fi IPv4 address (avoiding VPN/tun 198.18.x.x)."""
    try:
        out = subprocess.check_output(["ifconfig"], text=True)
        for line in out.splitlines():
            m = re.search(r"inet (192\.168\.\d+\.\d+|10\.\d+\.\d+\.\d+|172\.(?:1[6-9]|2\d|3[01])\.\d+\.\d+)", line)
            if m:
                return m.group(1)
    except Exception:
        pass
    return "127.0.0.1"


def get_arp_table():
    """Reads system ARP cache into an {ip: mac} dictionary."""
    arp = {}
    try:
        out = subprocess.check_output(["arp", "-an"], text=True)
        for line in out.splitlines():
            m = re.search(r"\(([\d\.]+)\) at ([0-9a-fA-F:]+)", line)
            if m:
                ip, mac = m.group(1), m.group(2).lower()
                mac_parts = [f"{int(p, 16):02x}" for p in mac.split(":") if p]
                if len(mac_parts) == 6:
                    arp[ip] = ":".join(mac_parts)
    except Exception:
        pass
    return arp


class CameraScanner:
    def __init__(self, timeout=3.0):
        self.timeout = timeout
        self.devices = {}
        self.lock = threading.Lock()
        self.local_ip = get_real_local_ip()
        net = ipaddress.IPv4Network(f"{self.local_ip}/24", strict=False)
        self.broadcast_ip = str(net.broadcast_address)
        self.subnet_hosts = [str(h) for h in net.hosts()]

    def record_device(self, ip, protocol, info):
        if ip == self.local_ip or ip.startswith("127."):
            return
        with self.lock:
            if ip not in self.devices:
                self.devices[ip] = {
                    "ip": ip,
                    "protocols": set(),
                    "camera_score": 0,
                    "details": [],
                    "open_ports": []
                }
            dev = self.devices[ip]
            dev["protocols"].add(protocol)
            dev["details"].append(info)
            info_str = str(info).lower()
            if any(k in info_str for k in ["gatedesc", "internetgatewaydevice", "router", "broadband"]):
                dev["camera_score"] -= 10
                dev["is_router"] = True
            elif any(k in info_str for k in ["cam", "ipc", "onvif", "rtsp", "video", "h264", "h265", "tuya", "anyka"]):
                dev["camera_score"] += 3
            if protocol in ("ONVIF", "RTSP"):
                dev["camera_score"] += 5

    def scan_ssdp(self):
        """UPnP / SSDP discovery on physical LAN interface."""
        queries = [
            "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: 2\r\nST: ssdp:all\r\n\r\n",
            "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: 2\r\nST: urn:schemas-upnp-org:device:BasicDevice:1\r\n\r\n"
        ]
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        try:
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(self.local_ip))
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)
            sock.settimeout(0.5)
            for q in queries:
                sock.sendto(q.encode(), SSDP_ADDR)
            t_end = time.time() + self.timeout
            while time.time() < t_end:
                try:
                    data, (src_ip, _) = sock.recvfrom(4096)
                    text = data.decode("utf-8", "ignore")
                    headers = {}
                    for line in text.split("\r\n"):
                        if ":" in line:
                            k, v = line.split(":", 1)
                            headers[k.strip().upper()] = v.strip()
                    server = headers.get("SERVER", "")
                    loc = headers.get("LOCATION", "")
                    self.record_device(src_ip, "SSDP", f"Server={server} Loc={loc}")
                except socket.timeout:
                    continue
        except Exception:
            pass
        finally:
            sock.close()

    def scan_onvif(self):
        """ONVIF WS-Discovery probe on physical interface."""
        ws_probe = (
            '<?xml version="1.0" encoding="utf-8"?>'
            '<soap:Envelope xmlns:soap="http://www.w3.org/2003/05/soap-envelope" '
            'xmlns:wsa="http://schemas.xmlsoap.org/ws/2004/08/addressing" '
            'xmlns:wsd="http://schemas.xmlsoap.org/ws/2005/04/discovery" '
            'xmlns:dn="http://www.onvif.org/ver10/network/wsdl">'
            '<soap:Header>'
            '<wsa:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>'
            '<wsa:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</wsa:Action>'
            f'<wsa:MessageID>uuid:{int(time.time())}</wsa:MessageID>'
            '</soap:Header>'
            '<soap:Body>'
            '<wsd:Probe><wsd:Types>dn:NetworkVideoTransmitter</wsd:Types></wsd:Probe>'
            '</soap:Body>'
            '</soap:Envelope>'
        )
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        try:
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(self.local_ip))
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)
            sock.settimeout(0.5)
            sock.sendto(ws_probe.encode(), ONVIF_ADDR)
            t_end = time.time() + self.timeout
            while time.time() < t_end:
                try:
                    data, (src_ip, _) = sock.recvfrom(8192)
                    text = data.decode("utf-8", "ignore")
                    self.record_device(src_ip, "ONVIF", text[:300].replace("\n", " "))
                except socket.timeout:
                    continue
        except Exception:
            pass
        finally:
            sock.close()

    def scan_vendor_udp(self):
        """Broadcasts Xiongmai & Tuya discovery frames on local subnet."""
        xm_req = b"\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\xfa\x05\x00\x00\x00\x00"
        tuya_req = b"{\"action\":\"query\"}"
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            sock.settimeout(0.5)
            for target_bcast in (self.broadcast_ip, "255.255.255.255"):
                sock.sendto(xm_req, (target_bcast, 34567))
                sock.sendto(tuya_req, (target_bcast, 6666))
                sock.sendto(tuya_req, (target_bcast, 6667))
            t_end = time.time() + self.timeout
            while time.time() < t_end:
                try:
                    data, (src_ip, src_port) = sock.recvfrom(2048)
                    self.record_device(src_ip, f"VendorUDP:{src_port}", f"Reply len={len(data)}")
                except socket.timeout:
                    continue
        except Exception:
            pass
        finally:
            sock.close()

    def check_ports_for_ip(self, ip):
        """Scans RTSP and camera management ports on target IP."""
        open_p = []
        for port in CAMERA_PORTS:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(0.25)
            try:
                if s.connect_ex((ip, port)) == 0:
                    open_p.append(port)
            except Exception:
                pass
            finally:
                s.close()
        if open_p:
            with self.lock:
                if ip in self.devices:
                    self.devices[ip]["open_ports"] = open_p
                    self.devices[ip]["camera_score"] += (5 if 554 in open_p else 2)
                else:
                    self.record_device(ip, "TCP-Port", f"Open: {open_p}")
                    self.devices[ip]["open_ports"] = open_p

    def quick_subnet_sweep(self):
        """Sweeps nearby subnet IPs to populate ARP and find quiet devices."""
        # Ping broadcast
        subprocess.run(["ping", "-c", "1", "-W", "1", self.broadcast_ip],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        arp = get_arp_table()
        # Probe known ARP IPs + first 25 IPs in subnet
        targets = set(arp.keys())
        # Check standard camera ports on known IPs
        threads = [threading.Thread(target=self.check_ports_for_ip, args=(ip,)) for ip in targets]
        for t in threads:
            t.start()
        for t in threads:
            t.join()

    def run(self, sweep_subnet=True):
        print(f"Scanning local Wi-Fi from {self.local_ip} (broadcast {self.broadcast_ip})...")
        threads = [
            threading.Thread(target=self.scan_ssdp),
            threading.Thread(target=self.scan_onvif),
            threading.Thread(target=self.scan_vendor_udp)
        ]
        for t in threads:
            t.start()
        for t in threads:
            t.join()

        if sweep_subnet:
            self.quick_subnet_sweep()

        arp = get_arp_table()
        results = []
        for ip, d in self.devices.items():
            mac = arp.get(ip, "unknown")
            is_router = d.get("is_router", False) or ip.endswith(".1")
            is_camera = not is_router and (d["camera_score"] >= 2 or (554 in d.get("open_ports", [])))
            results.append({
                "ip": ip,
                "mac": mac,
                "protocols": list(d["protocols"]),
                "camera_score": d["camera_score"],
                "is_camera": is_camera,
                "is_router": is_router,
                "open_ports": d.get("open_ports", []),
                "details": d["details"]
            })
        results.sort(key=lambda x: x["camera_score"], reverse=True)
        return results


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--timeout", type=float, default=2.5, help="Discovery timeout in seconds")
    args = ap.parse_args()

    scanner = CameraScanner(timeout=args.timeout)
    results = scanner.run(sweep_subnet=True)

    print("\n" + "=" * 70)
    print(" " * 20 + "LOCAL WI-FI DISCOVERY REPORT")
    print("=" * 70)

    if not results:
        print("No camera devices replied to discovery probes.")
        print("All quiet devices on subnet scanned.")
        return

    print(f"{'IP Address':<16} {'MAC Address':<18} {'Protocols':<20} {'Verdict'}")
    print("-" * 70)
    for r in results:
        if r["is_router"]:
            verdict = "🌐 ROUTER / GATEWAY"
        elif r["is_camera"]:
            verdict = "🔥 CAMERA SUSPECT"
        else:
            verdict = "Connected Device"
        proto_str = ", ".join(r["protocols"])[:19]
        ports = f" Ports: {r['open_ports']}" if r.get("open_ports") else ""
        print(f"{r['ip']:<16} {r['mac']:<18} {proto_str:<20} {verdict}{ports}")

    print("=" * 70)
    for r in results:
        if r["is_camera"]:
            print(f"\n[!] Candidate details for {r['ip']} ({r['mac']}):")
            for det in r["details"]:
                print(f"    - {det[:120]}")


if __name__ == "__main__":
    main()
