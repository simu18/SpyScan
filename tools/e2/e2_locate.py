#!/usr/bin/env python3
"""SpyScan — Live RSSI "Warmer / Colder" Finder (Method 1.1 / Mode 2 Locator).

Connects to the ESP32-S3 sniffer and provides real-time proximity tracking
for a target Wi-Fi camera station using smoothed RSSI, trend detection,
and an interactive Geiger-counter style visual proximity meter.

Usage:
  python tools/e2/e2_locate.py --target cc:b8:5e:ad:5c:4a --channel 10
  python tools/e2/e2_locate.py --auto
  python tools/e2/e2_locate.py --target cc:b8:5e:ad:5c:4a --beep
"""
import argparse
import collections
import math
import os
import pathlib
import sys
import threading
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is required: pip install pyserial")

# ANSI Color escapes
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
BG_BLUE = "\033[44m"
CLEAR_SCREEN = "\033[2J\033[H"
HIDE_CURSOR = "\033[?25l"
SHOW_CURSOR = "\033[?25h"


def format_mac(m: str) -> str:
    parts = [p.zfill(2) for p in m.lower().replace("-", ":").split(":") if p]
    return ":".join(parts) if len(parts) == 6 else m.lower()


class SnifferClient:
    """Manages serial connection and command dispatch to the ESP32-S3."""

    def __init__(self, port: str, baud: int = 115200):
        self.ser = serial.Serial()
        self.ser.port = port
        self.ser.baudrate = baud
        self.ser.timeout = 0.1
        self.ser.dtr = False
        self.ser.rts = False
        self.ser.open()
        self.running = True
        self.callbacks = []
        self.reader_thread = threading.Thread(target=self._read_loop, daemon=True)
        self.reader_thread.start()

    def send_cmd(self, cmd: str):
        if self.ser.is_open:
            self.ser.write((cmd.strip() + "\n").encode("utf-8"))
            self.ser.flush()

    def register_callback(self, cb):
        self.callbacks.append(cb)

    def _read_loop(self):
        buf = b""
        while self.running:
            try:
                chunk = self.ser.read(2048)
                if not chunk:
                    continue
                buf += chunk
                *lines, buf = buf.split(b"\n")
                for raw in lines:
                    line = raw.decode("utf-8", "replace").strip()
                    if not line:
                        continue
                    for cb in self.callbacks:
                        cb(line)
            except Exception:
                break

    def close(self):
        self.running = False
        try:
            self.send_cmd("focus off")
            self.send_cmd("bin 100")
            time.sleep(0.1)
            self.ser.close()
        except Exception:
            pass


class CameraLocator:
    """Real-time signal tracking and visual proximity estimation."""

    def __init__(self, port: str, target_mac: str = "", channel: int = 0,
                 alpha: float = 0.25, beep: bool = False, update_hz: int = 15):
        self.port = port
        self.target = format_mac(target_mac) if target_mac else ""
        self.channel = channel
        self.alpha = alpha
        self.beep = beep
        self.update_interval = 1.0 / update_hz

        self.client = None
        self.lock = threading.Lock()

        # Signal state
        self.raw_rssi = None
        self.smooth_rssi = None
        self.peak_rssi = -100.0
        self.last_pkt_time = 0.0
        self.tx_bytes_sec = 0.0
        self.tx_frames_sec = 0.0
        self.total_frames = 0
        self.history = collections.deque(maxlen=15)  # for trend slope
        self.bin_records = collections.deque(maxlen=20)  # for throughput window

        # Device discovery if target is empty
        self.discovered = {}  # mac -> {tx_bytes, ch, rssi, last_seen}

    def _on_line(self, line: str):
        if not line or len(line) < 2 or line[1] != ",":
            return
        prefix = line[0]

        if prefix == "B":
            # B,<t_ms>,<ch>,<mac>,<tx_b>,<tx_n>,<rx_b>,<rx_n>,<rssi>,<bssid>
            parts = line.split(",")
            if len(parts) >= 9:
                try:
                    ch = int(parts[2])
                    mac = parts[3].lower()
                    tx_b = int(parts[4])
                    tx_n = int(parts[5])
                    rssi = int(parts[8])
                    now = time.time()

                    with self.lock:
                        if not self.target:
                            # Auto-discovery mode
                            if mac not in self.discovered:
                                self.discovered[mac] = {"tx_b": 0, "ch": ch, "rssi": rssi, "seen": now}
                            self.discovered[mac]["tx_b"] += tx_b
                            self.discovered[mac]["seen"] = now
                            self.discovered[mac]["rssi"] = rssi
                            self.discovered[mac]["ch"] = ch
                            return

                        if mac == self.target and tx_n > 0 and rssi != 0:
                            self.raw_rssi = rssi
                            self.last_pkt_time = now
                            self.total_frames += tx_n
                            self.bin_records.append((now, tx_b, tx_n))

                            # Update Exponential Moving Average (EMA)
                            if self.smooth_rssi is None:
                                self.smooth_rssi = float(rssi)
                            else:
                                self.smooth_rssi = (self.alpha * rssi) + ((1.0 - self.alpha) * self.smooth_rssi)

                            if self.smooth_rssi > self.peak_rssi:
                                self.peak_rssi = self.smooth_rssi

                            self.history.append((now, self.smooth_rssi))
                except (ValueError, IndexError):
                    pass

    def estimate_distance(self, rssi: float) -> str:
        """Log-distance path loss approximation for indoor 2.4 GHz."""
        # A = -43 dBm at 1m, n = 2.2 path loss exponent
        try:
            d_m = 10.0 ** ((-43.0 - rssi) / (10.0 * 2.2))
            if d_m < 0.35:
                return f"< {int(d_m * 100)} cm"
            elif d_m < 1.0:
                return f"~{int(d_m * 100)} cm"
            else:
                return f"~{d_m:.1f} m"
        except Exception:
            return "~1 m"

    def calculate_trend(self) -> tuple[str, str]:
        """Calculates slope over recent history to report Warmer / Colder."""
        if len(self.history) < 4:
            return "STEADY", WHITE
        t0, r0 = self.history[0]
        t1, r1 = self.history[-1]
        dt = t1 - t0
        if dt < 0.2:
            return "STEADY", WHITE
        delta = r1 - r0
        if delta >= 2.0:
            return f"▲▲ WARMER (+{delta:.1f} dBm)", GREEN + BOLD
        elif delta >= 0.8:
            return f"▲ WARMING (+{delta:.1f} dBm)", GREEN
        elif delta <= -2.0:
            return f"▼▼ COLDER ({delta:.1f} dBm)", RED + BOLD
        elif delta <= -0.8:
            return f"▼ COOLING ({delta:.1f} dBm)", RED
        else:
            return f"═ STEADY ({delta:+.1f} dBm)", WHITE

    def calculate_throughput(self) -> tuple[float, float]:
        """Calculates kB/s and fps over last 1.5s."""
        now = time.time()
        with self.lock:
            recent = [rec for rec in self.bin_records if now - rec[0] <= 1.5]
            if not recent:
                return 0.0, 0.0
            dt = max(0.5, recent[-1][0] - recent[0][0])
            sum_b = sum(r[1] for r in recent)
            sum_n = sum(r[2] for r in recent)
            return (sum_b / 1000.0) / dt, sum_n / dt

    def render_meter_bar(self, rssi: float, peak: float, width: int = 36) -> str:
        """Renders colored ASCII progress bar from -85 dBm (0%) to -25 dBm (100%)."""
        min_rssi = -85.0
        max_rssi = -25.0
        clamped = max(min_rssi, min(max_rssi, rssi))
        frac = (clamped - min_rssi) / (max_rssi - min_rssi)
        filled = int(round(frac * width))

        peak_clamped = max(min_rssi, min(max_rssi, peak))
        peak_idx = int(round(((peak_clamped - min_rssi) / (max_rssi - min_rssi)) * width)) - 1
        peak_idx = max(0, min(width - 1, peak_idx))

        # Color gradient based on proximity
        if rssi >= -38:
            color = RED + BOLD
        elif rssi >= -50:
            color = YELLOW + BOLD
        elif rssi >= -65:
            color = CYAN
        else:
            color = BLUE

        bar_chars = []
        for i in range(width):
            if i == peak_idx and i >= filled:
                bar_chars.append(WHITE + "│" + color)
            elif i < filled:
                bar_chars.append("█")
            else:
                bar_chars.append(DIM + "░" + RESET + color)

        return f"{color}[{''.join(bar_chars)}{RESET}{color}]{RESET}"

    def auto_discover_target(self) -> tuple[str, int]:
        """Scans for 3 seconds to find the top transmitting station."""
        print(f"\n{CYAN}[i] Scanning channels 1..13 to locate active cameras...{RESET}")
        self.client.send_cmd("hop 150")
        time.sleep(3.2)
        self.client.send_cmd("hop 0")

        with self.lock:
            candidates = sorted(
                [d for d in self.discovered.items() if d[1]["tx_b"] > 1000],
                key=lambda x: x[1]["tx_b"],
                reverse=True
            )

        if not candidates:
            sys.exit(f"{RED}[!] No active transmitting stations detected. Ensure camera is powered on.{RESET}")

        print(f"\n{BOLD}Discovered Transmitting Stations:{RESET}")
        print(f"{'#':<3} {'MAC Address':<20} {'Ch':<4} {'Total Transmit':<16} {'RSSI'}")
        print("-" * 55)
        for idx, (mac, info) in enumerate(candidates[:6], 1):
            kb = info['tx_b'] / 1000.0
            print(f"{idx:<3} {mac:<20} {info['ch']:<4} {kb:8.1f} kB        {info['rssi']} dBm")

        top_mac, top_info = candidates[0]
        print(f"\n{GREEN}[✓] Auto-selected top transmitter: {BOLD}{top_mac}{RESET} (Ch {top_info['ch']})")
        return top_mac, top_info["ch"]

    def run(self):
        print(f"{CYAN}Connecting to ESP32-S3 sniffer on {self.port}...{RESET}")
        self.client = SnifferClient(self.port)
        self.client.register_callback(self._on_line)
        time.sleep(0.5)

        if not self.target:
            self.target, detected_ch = self.auto_discover_target()
            if not self.channel:
                self.channel = detected_ch

        # Lock channel and focus on target
        if self.channel:
            self.client.send_cmd(f"ch {self.channel}")
            time.sleep(0.1)
        self.client.send_cmd(f"focus {self.target}")
        self.client.send_cmd("bin 50")  # fast 50 ms bins for fluid 20 Hz updates
        time.sleep(0.2)

        print(HIDE_CURSOR)
        last_beep_time = time.time()

        try:
            while True:
                time.sleep(self.update_interval)
                now = time.time()
                time_since_pkt = now - self.last_pkt_time if self.last_pkt_time else 999.0

                with self.lock:
                    smooth = self.smooth_rssi
                    raw = self.raw_rssi
                    peak = self.peak_rssi

                trend_text, trend_color = self.calculate_trend()
                kbps, pps = self.calculate_throughput()

                # Build terminal UI
                ui = []
                ui.append(f"{BOLD}{WHITE}╔═══════════════════════════════════════════════════════════════════════╗{RESET}")
                ui.append(f"{BOLD}{WHITE}║           SpyScan — Live Wi-Fi Camera Proximity Locator               ║{RESET}")
                ui.append(f"{BOLD}{WHITE}╠═══════════════════════════════════════════════════════════════════════╣{RESET}")
                ui.append(f"║  Target MAC: {BOLD}{CYAN}{self.target:<20}{RESET} Channel: {BOLD}{self.channel:<3}{RESET}   Status: "
                          + (f"{GREEN}STREAMING{RESET}    ║" if kbps > 10 else f"{YELLOW}IDLE / LOW{RESET}   ║"))

                if smooth is not None and time_since_pkt < 2.5:
                    bar = self.render_meter_bar(smooth, peak, width=34)
                    dist = self.estimate_distance(smooth)

                    if smooth >= -38:
                        proximity_badge = f"{BG_RED}{WHITE}{BOLD} 🔥 BURNING HOT {RESET}"
                    elif smooth >= -50:
                        proximity_badge = f"{BG_YELLOW}{WHITE}{BOLD} ⚡ WARM        {RESET}"
                    elif smooth >= -65:
                        proximity_badge = f"{BG_BLUE}{WHITE}{BOLD} 🟡 COOL        {RESET}"
                    else:
                        proximity_badge = f"{DIM} ❄️ COLD        {RESET}"

                    ui.append(f"║                                                                       ║")
                    ui.append(f"║  Signal: {bar}  {BOLD}{smooth:5.1f} dBm{RESET}  ║")
                    ui.append(f"║                                                                       ║")
                    ui.append(f"║  Proximity: {proximity_badge}   Estimated Dist: {BOLD}{dist:<10}{RESET}   ║")
                    ui.append(f"║  Peak Hold: {BOLD}{peak:5.1f} dBm{RESET}            Trend: {trend_color}{trend_text:<25}{RESET} ║")
                    ui.append(f"║  Uplink:    {BOLD}{kbps:5.1f} kB/s{RESET} ({pps:4.0f} fps) Raw RSSI: {raw:3d} dBm (EMA α={self.alpha:.2f})  ║")

                    # Audio Geiger beep
                    if self.beep and smooth >= -75:
                        beep_interval = max(0.08, min(1.2, (-25.0 - smooth) / 50.0))
                        if now - last_beep_time >= beep_interval:
                            sys.stdout.write("\a")
                            sys.stdout.flush()
                            last_beep_time = now
                else:
                    ui.append(f"║                                                                       ║")
                    ui.append(f"║  {YELLOW}Searching for packets from target... (last seen {time_since_pkt:.1f}s ago){RESET}{' ':>16}║")
                    ui.append(f"║  Make sure camera is powered on and within range.                     ║")
                    ui.append(f"║                                                                       ║")
                    ui.append(f"║                                                                       ║")
                    ui.append(f"║                                                                       ║")

                ui.append(f"{BOLD}{WHITE}╚═══════════════════════════════════════════════════════════════════════╝{RESET}")
                ui.append(f"{DIM}  Controls: [Ctrl+C] Exit  |  Walk closer/farther to track signal strength{RESET}")

                # Draw UI smoothly in place
                sys.stdout.write("\033[H" + "\n".join(ui) + "\n")
                sys.stdout.flush()

        except KeyboardInterrupt:
            pass
        finally:
            sys.stdout.write(SHOW_CURSOR)
            sys.stdout.write("\n\nExiting locator...\n")
            sys.stdout.flush()
            if self.client:
                self.client.close()


def find_sniffer_port(preferred: str = "") -> str:
    import glob
    if preferred and os.path.exists(preferred):
        return preferred
    ports = sorted(glob.glob("/dev/cu.usbmodem*"))
    if "/dev/cu.usbmodem111401" in ports:
        return "/dev/cu.usbmodem111401"
    return ports[0] if ports else "/dev/cu.usbmodem111401"


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--port", default="", help="Serial port of ESP32-S3 sniffer (auto-detects if omitted)")
    parser.add_argument("--target", default="cc:b8:5e:ad:5c:4a", help="Target STA MAC address (default: cc:b8:5e:ad:5c:4a)")
    parser.add_argument("--auto", action="store_true", help="Auto-scan and discover active camera instead of using target MAC")
    parser.add_argument("--channel", type=int, default=10, help="Channel 1..13 (default: 10)")
    parser.add_argument("--alpha", type=float, default=0.25, help="EMA smoothing factor (0.1 = very smooth, 0.5 = fast)")
    parser.add_argument("--beep", action="store_true", help="Audible Geiger-counter style proximity beeps")
    parser.add_argument("--rate", type=int, default=15, help="UI refresh rate in Hz")
    args = parser.parse_args()

    port = find_sniffer_port(args.port)
    target = "" if args.auto else args.target

    locator = CameraLocator(
        port=port,
        target_mac=target,
        channel=args.channel if not args.auto else 0,
        alpha=args.alpha,
        beep=args.beep,
        update_hz=args.rate
    )
    locator.run()


if __name__ == "__main__":
    main()
