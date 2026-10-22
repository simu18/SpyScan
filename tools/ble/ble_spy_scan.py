#!/usr/bin/env python3
"""SpyScan — BLE Camera & Covert Tracker Sniffer (Method 3).

Detects hidden BLE-enabled cameras, Apple AirTags, Samsung SmartTags,
Tile trackers, and Tuya/SmartLife IoT surveillance beacons.

Features:
  - Real-time live table with threat classification
  - Audio/Visual proximity meter for pinpointing hidden trackers
  - Fingerprints AirTags, SmartTags, Tile, and Tuya camera beacons

Usage:
  python tools/ble/ble_spy_scan.py
  python tools/ble/ble_spy_scan.py --locate <device_address_or_partial>
  python tools/ble/ble_spy_scan.py --beep
"""
import argparse
import asyncio
import collections
import os
import sys
import time

try:
    from bleak import BleakScanner
    from bleak.backends.device import BLEDevice
    from bleak.backends.scanner import AdvertisementData
except ImportError:
    sys.exit("bleak is required: pip install bleak")

# Terminal color codes
RESET = "\033[0m"
BOLD = "\033[1m"
DIM = "\033[2m"
RED = "\033[91m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
BLUE = "\033[94m"
MAGENTA = "\033[95m"
CYAN = "\033[96m"
WHITE = "\033[97m"
BG_RED = "\033[41m"
BG_YELLOW = "\033[43m"
CLEAR_SCREEN = "\033[2J\033[H"
HIDE_CURSOR = "\033[?25l"
SHOW_CURSOR = "\033[?25h"

# Known Company Identifiers
COMPANY_APPLE = 0x004C
COMPANY_MICROSOFT = 0x0006
COMPANY_SAMSUNG = 0x0075
COMPANY_TUYA = 0x07D0
COMPANY_TILE = 0x011A

# Known 16-bit Service UUIDs for Trackers and Cams
UUID_AIRTAG_NEARBY = "0000fd6f-0000-1000-8000-00805f9b34fb"  # Exposure Notification / FindMy
UUID_TILE = "0000feed-0000-1000-8000-00805f9b34fb"
UUID_SAMSUNG_FIND = "0000fd5a-0000-1000-8000-00805f9b34fb"
UUID_GOOGLE_FASTPAIR = "0000fe2c-0000-1000-8000-00805f9b34fb"
UUID_TUYA_BEACON = "0000fd50-0000-1000-8000-00805f9b34fb"

CAMERA_NAME_KEYWORDS = [
    "cam", "camera", "ipc", "v380", "anyka", "a9", "mini", "dv", "hd", "wifi",
    "eyes", "surveil", "spy", "tuya", "smartlife", "view", "video", "dash"
]


def classify_advertisement(dev: BLEDevice, adv: AdvertisementData) -> tuple[str, str, int]:
    """Analyzes manufacturer data, UUIDs, and device name to assess threat level.

    Returns:
      (Threat Category, Description, Threat Score: 0 = Info, 1 = Low, 2 = Medium, 3 = High)
    """
    name = (dev.name or adv.local_name or "").lower()
    mfr_ids = list(adv.manufacturer_data.keys())
    uuids = [u.lower() for u in adv.service_uuids]

    # 1. Camera Name Matches
    for kw in CAMERA_NAME_KEYWORDS:
        if kw in name:
            return ("🔥 SPY CAM / IPC", f"Keyword match: '{kw}' ({dev.name or adv.local_name})", 3)

    # 2. Apple FindMy / AirTag Tracker
    if COMPANY_APPLE in adv.manufacturer_data:
        mfr_bytes = adv.manufacturer_data[COMPANY_APPLE]
        if len(mfr_bytes) >= 2:
            adv_type = mfr_bytes[0]
            # Type 0x12 = Apple FindMy Offline Location Beacon (AirTag, AirPods, FindMy Network)
            if adv_type == 0x12 and len(mfr_bytes) >= 20:
                status_byte = mfr_bytes[1] if len(mfr_bytes) > 1 else 0
                return ("🚨 APPLE AIRTAG", f"FindMy offline tracking payload (status 0x{status_byte:02x})", 3)
            # Type 0x02 = iBeacon
            elif adv_type == 0x02:
                return ("📍 IBEACON TAG", "Standard micro-proximity beacon", 2)
            elif adv_type in (0x07, 0x09):
                return ("📱 APPLE DEVICE", "AirPods / iOS continuity broadcast", 0)

    # 3. Samsung Galaxy SmartTag
    if COMPANY_SAMSUNG in adv.manufacturer_data or UUID_SAMSUNG_FIND in uuids:
        return ("🚨 SMARTTAG", "Samsung SmartTag / Galaxy Find tracker", 3)

    # 4. Tile Tracker
    if COMPANY_TILE in adv.manufacturer_data or UUID_TILE in uuids or "tile" in name:
        return ("🚨 TILE TRACKER", "Tile crowd-tracking beacon", 3)

    # 5. Tuya / SmartLife Smart Camera Beacon
    if COMPANY_TUYA in adv.manufacturer_data or UUID_TUYA_BEACON in uuids:
        return ("🔥 TUYA IOT / CAM", "Tuya smart surveillance / pairing beacon", 2)

    # 6. Google Fast Pair / Android Tracker
    if UUID_GOOGLE_FASTPAIR in uuids:
        return ("📍 GOOGLE FINDER", "Google Fast Pair / Find My Device beacon", 2)

    # 7. Default Categorization
    if name:
        return ("BLE Device", f"Named: {dev.name or adv.local_name}", 0)
    elif mfr_ids:
        return ("BLE Beacon", f"Company ID: 0x{mfr_ids[0]:04x}", 0)
    else:
        return ("Unidentified", "Anonymous raw BLE advertiser", 0)


def render_bar(rssi: float, width: int = 24) -> str:
    """Renders visual signal strength bar."""
    min_rssi, max_rssi = -95.0, -35.0
    clamped = max(min_rssi, min(max_rssi, rssi))
    frac = (clamped - min_rssi) / (max_rssi - min_rssi)
    filled = int(round(frac * width))

    if rssi >= -50:
        col = RED + BOLD
    elif rssi >= -68:
        col = YELLOW
    elif rssi >= -82:
        col = CYAN
    else:
        col = BLUE

    bar = "█" * filled + DIM + "░" * (width - filled) + RESET
    return f"{col}[{bar}{col}]{RESET}"


class BleSpyDetector:
    def __init__(self, locate_target: str = "", beep: bool = False):
        self.target = locate_target.lower()
        self.beep = beep
        self.devices = {}
        self.last_clean_time = time.time()
        self.lock = asyncio.Lock()

    def on_advertisement(self, dev: BLEDevice, adv: AdvertisementData):
        addr = dev.address.lower()
        category, desc, threat = classify_advertisement(dev, adv)
        now = time.time()

        if addr not in self.devices:
            self.devices[addr] = {
                "addr": dev.address,
                "name": dev.name or adv.local_name or "<Anonymous>",
                "category": category,
                "desc": desc,
                "threat": threat,
                "rssi": adv.rssi,
                "smooth_rssi": float(adv.rssi),
                "count": 0,
                "first_seen": now,
                "last_seen": now,
            }
        d = self.devices[addr]
        d["last_seen"] = now
        d["rssi"] = adv.rssi
        d["count"] += 1
        d["name"] = dev.name or adv.local_name or d["name"]

        # If previously unclassified but new advertisement contains name or mfr, update
        if threat > d["threat"]:
            d["category"] = category
            d["desc"] = desc
            d["threat"] = threat

        # EMA smoothing for proximity
        d["smooth_rssi"] = (0.3 * adv.rssi) + (0.7 * d["smooth_rssi"])

    def estimate_distance(self, rssi: float) -> str:
        # A = -59 dBm reference at 1m for BLE, n = 2.0
        try:
            d_m = 10.0 ** ((-59.0 - rssi) / 20.0)
            if d_m < 0.5:
                return f"< {int(d_m * 100)} cm"
            elif d_m < 1.0:
                return f"~{int(d_m * 100)} cm"
            else:
                return f"~{d_m:.1f} m"
        except Exception:
            return "~1 m"

    def render_dashboard(self):
        now = time.time()
        # Sort devices: Highest threat first, then strongest RSSI
        active = [d for d in self.devices.values() if now - d["last_seen"] < 15.0]
        active.sort(key=lambda x: (x["threat"], x["smooth_rssi"]), reverse=True)

        lines = []
        lines.append(f"{BOLD}{WHITE}╔═════════════════════════════════════════════════════════════════════════════════════════════╗{RESET}")
        lines.append(f"{BOLD}{WHITE}║               SpyScan — BLE Hidden Camera & Covert Tracker Sniffer                          ║{RESET}")
        lines.append(f"{BOLD}{WHITE}╠═════════════════════════════════════════════════════════════════════════════════════════════╣{RESET}")
        lines.append(f"{BOLD}{'Category':<19} {'Address / ID':<22} {'RSSI':<8} {'Signal Meter':<28} {'Details':<20}{RESET}")
        lines.append("─" * 93)

        threat_count = sum(1 for d in active if d["threat"] >= 2)

        if not active:
            lines.append("  Scanning for 2.4 GHz Bluetooth Low Energy advertisement packets...")
        else:
            for d in active[:10]:
                cat = d["category"]
                if d["threat"] == 3:
                    cat_badge = f"{BG_RED}{WHITE}{BOLD} {cat:<17} {RESET}"
                elif d["threat"] == 2:
                    cat_badge = f"{BG_YELLOW}{WHITE}{BOLD} {cat:<17} {RESET}"
                else:
                    cat_badge = f"{CYAN}{cat:<18}{RESET}"

                meter = render_bar(d["smooth_rssi"], width=16)
                dist = self.estimate_distance(d["smooth_rssi"])
                disp_addr = d["addr"] if len(d["addr"]) <= 20 else d["addr"][:18] + ".."
                details = f"{d['desc'][:22]} ({dist})"
                lines.append(f"{cat_badge} {disp_addr:<21} {d['rssi']:>3} dBm  {meter} {details}")

        lines.append("─" * 93)
        threat_alert = f"{RED}{BOLD}🚨 {threat_count} SUSPICIOUS SURVEILLANCE BEACON(S) IN RANGE!{RESET}" if threat_count > 0 else f"{GREEN}[✓] No covert tracking beacons detected.{RESET}"
        lines.append(f" Status: {threat_alert}")
        lines.append(f"{DIM} Total Active Devices: {len(active)} | Ctrl+C to Exit | Walk to track signal strength{RESET}")
        lines.append(f"{BOLD}{WHITE}╚═════════════════════════════════════════════════════════════════════════════════════════════╝{RESET}")

        sys.stdout.write("\033[H" + "\n".join(lines) + "\n")
        sys.stdout.flush()

        # Beep if high threat nearby
        if self.beep and any(d["threat"] == 3 and d["smooth_rssi"] >= -65 for d in active):
            sys.stdout.write("\a")
            sys.stdout.flush()

    async def run(self):
        print(f"{CYAN}Starting BLE Spy & Covert Tracker Scanner...{RESET}")
        scanner = BleakScanner(detection_callback=self.on_advertisement)
        await scanner.start()
        sys.stdout.write(HIDE_CURSOR)
        try:
            while True:
                await asyncio.sleep(0.3)
                self.render_dashboard()
        except (KeyboardInterrupt, asyncio.CancelledError):
            pass
        finally:
            await scanner.stop()
            sys.stdout.write(SHOW_CURSOR + "\n\nBLE scan stopped.\n")


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--locate", default="", help="Focus on specific device address/name")
    parser.add_argument("--beep", action="store_true", help="Audio alert when tracker or camera is close")
    args = parser.parse_args()

    detector = BleSpyDetector(locate_target=args.locate, beep=args.beep)
    try:
        asyncio.run(detector.run())
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
