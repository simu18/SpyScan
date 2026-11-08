#!/usr/bin/env python3
"""SpyScan — Sub-1 GHz RF Bug Hunter (Method 4 / CC1101 TSCM Suite).

Hunts for analog audio transmitters, covert bugs, wireless microphones,
and digital bursts across the 315 / 433 / 868 MHz ISM bands using the
Texas Instruments CC1101 transceiver on the ESP32-S3 DevKit.

Features:
  - Real-time ANSI spectrum dashboard with live Geiger proximity meter.
  - Distinguishes continuous carrier analog audio bugs (100% duty cycle, low variance)
    from digital bursts (car keyfobs, weather sensors, doorbells) and ambient noise.
  - Proximity distance estimation using Sub-GHz log-distance path loss modeling.
  - Audio Geiger proximity chirps (--beep).
  - Multi-band switching (433.92 MHz, 868.30 MHz, 315.00 MHz, or channel sweep).
  - Automated session logging, publication-grade figure generation, and scientific report output.
  - Built-in --demo mode for offline validation, presentation slides, and algorithm verification.

Usage:
  python tools/rf/subghz_bug_hunter.py
  python tools/rf/subghz_bug_hunter.py --beep
  python tools/rf/subghz_bug_hunter.py --freq 868
  python tools/rf/subghz_bug_hunter.py --demo
"""
import argparse
import collections
import datetime as dt
import glob
import math
import os
import pathlib
import select
import sys
import termios
import threading
import time
import tty

try:
    import numpy as np
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

ROOT = pathlib.Path(__file__).resolve().parents[2]
DATA_DIR = ROOT / "experiments" / "E3_subghz"

# ANSI Terminal Colors
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
BG_GREEN = "\033[42m"
BG_YELLOW = "\033[43m"
BG_MAGENTA = "\033[45m"
CLEAR_SCREEN = "\033[2J\033[H"
HIDE_CURSOR = "\033[?25l"
SHOW_CURSOR = "\033[?25h"


class NonBlockingInput:
    """Captures single keypresses from terminal without requiring Enter."""
    def __init__(self):
        self.old_settings = None
        self.is_tty = sys.stdin.isatty()

    def __enter__(self):
        if self.is_tty:
            try:
                self.old_settings = termios.tcgetattr(sys.stdin)
                tty.setcbreak(sys.stdin.fileno())
            except Exception:
                pass
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.is_tty and self.old_settings:
            try:
                termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.old_settings)
            except Exception:
                pass

    def get_char(self):
        if not self.is_tty:
            return None
        r, _, _ = select.select([sys.stdin], [], [], 0)
        if r:
            return sys.stdin.read(1)
        return None


class SubGhzReceiver(threading.Thread):
    def __init__(self, port: str, baud: int = 115200, log_file=None):
        super().__init__(daemon=True)
        self.ser = serial.Serial()
        self.ser.port, self.ser.baudrate, self.ser.timeout = port, baud, 0.1
        self.ser.dtr = False
        self.ser.rts = False
        self.ser.open()
        self.log_file = log_file
        self.running = True
        self.lock = threading.Lock()
        self.latest_sample = None
        self.chip_status = "UNKNOWN"
        self.chip_version = 0
        self.history = []

    def send_cmd(self, cmd: str):
        if self.ser and self.ser.is_open:
            self.ser.write((cmd.strip() + "\n").encode("utf-8"))
            self.ser.flush()

    def run(self):
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
                    if self.log_file:
                        self.log_file.write(line + "\n")
                        self.log_file.flush()
                    self._parse_line(line)
            except Exception:
                break

    def _parse_line(self, line: str):
        with self.lock:
            # S,<t_ms>,<freq_khz>,<rssi_instant>,<rssi_mean>,<rssi_peak>,<variance*10>,<duty_pct>,<threat_code>,<threat_name>
            if line.startswith("S,"):
                parts = line.split(",")
                if len(parts) >= 10:
                    try:
                        t_ms = int(parts[1])
                        freq_khz = int(parts[2])
                        rssi_inst = int(parts[3])
                        rssi_mean = int(parts[4])
                        rssi_peak = int(parts[5])
                        variance = float(parts[6]) / 10.0
                        duty = int(parts[7])
                        threat_code = int(parts[8])
                        threat_name = parts[9]
                        sample = {
                            "t": time.time(), "t_ms": t_ms, "freq_khz": freq_khz,
                            "rssi_inst": rssi_inst, "rssi_mean": rssi_mean,
                            "rssi_peak": rssi_peak, "variance": variance,
                            "duty": duty, "threat_code": threat_code, "threat_name": threat_name
                        }
                        self.latest_sample = sample
                        self.history.append(sample)
                    except (ValueError, IndexError):
                        pass
            elif "cc1101,OK" in line:
                self.chip_status = "ONLINE"
            elif "cc1101,NOT_DETECTED" in line:
                self.chip_status = "NOT_DETECTED"

    def close(self):
        self.running = False
        try:
            self.ser.close()
        except Exception:
            pass


def auto_detect_dev_port() -> str:
    """Finds the ESP32-S3 DevKit port automatically."""
    ports = sorted(glob.glob("/dev/cu.usbmodem*"))
    if "/dev/cu.usbmodem111401" in ports:
        return "/dev/cu.usbmodem111401"
    return ports[0] if ports else "/dev/cu.usbmodem111401"


def estimate_subghz_distance(rssi: float) -> str:
    """Estimates physical distance for 433 MHz RF bug in indoor environment."""
    # A = -38 dBm at 1 m, n = 2.4 path loss exponent
    try:
        d_m = 10.0 ** ((-38.0 - rssi) / (10.0 * 2.4))
        if d_m < 0.3:
            return f"< 30 cm"
        elif d_m < 1.0:
            return f"~{int(d_m * 100)} cm"
        else:
            return f"~{d_m:.1f} m"
    except Exception:
        return "~1 m"


def render_ascii_waterfall(history: list, width: int = 50, rows: int = 8) -> list:
    """Renders a rolling time-series waterfall of RF RSSI levels."""
    CHARS = " ░▒▓█"
    lines = []
    recent = list(history)[-rows:]
    for sample in recent:
        rssi = sample.get("rssi_inst", -110)
        # Map -105 dBm (0) to -30 dBm (1.0)
        norm = max(0.0, min(1.0, (rssi + 105.0) / 75.0))
        filled = int(norm * width)
        color = RED if rssi >= -50 else (YELLOW if rssi >= -70 else (CYAN if rssi >= -85 else BLUE))
        bar = "█" * filled + "░" * (width - filled)
        tag = f"{sample.get('freq_khz', 433920)/1000:6.2f} MHz"
        lines.append(f"  {DIM}{tag}{RESET} {color}[{bar}]{RESET} {BOLD}{rssi:4d} dBm{RESET}")
    while len(lines) < rows:
        lines.insert(0, f"  {DIM}  ------ MHz [░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░] ---- dBm{RESET}")
    return lines


def generate_plots_and_report(session_dir: pathlib.Path, history: list, is_demo: bool = False):
    """Generates the publication figure and Markdown report for Method 4."""
    if not history or not HAS_MATPLOTLIB:
        return None, None

    t0 = history[0]["t"]
    t_axis = [s["t"] - t0 for s in history]
    rssi_inst = [s["rssi_inst"] for s in history]
    rssi_mean = [s["rssi_mean"] for s in history]
    duties = [s["duty"] for s in history]
    threats = [s["threat_code"] for s in history]

    plt.style.use("dark_background")
    fig = plt.figure(figsize=(15, 5), dpi=160)

    # Panel 1: Temporal RSSI & Proximity Surge
    ax1 = fig.add_subplot(1, 3, 1)
    ax1.plot(t_axis, rssi_inst, color="#00ffff", alpha=0.5, label="Instant RSSI (dBm)")
    ax1.plot(t_axis, rssi_mean, color="#ffaa00", linewidth=2.2, label="Filtered RSSI (dBm)")
    ax1.axhline(-75, color="#ff4444", linestyle="--", alpha=0.8, label="Carrier Threshold (-75 dBm)")
    ax1.set_title("Sub-1 GHz RF Carrier Power Tracking", fontsize=11, fontweight="bold", color="#ffaa00")
    ax1.set_xlabel("Elapsed Time (s)")
    ax1.set_ylabel("Received Signal Strength (dBm)")
    ax1.set_ylim(-115, -25)
    ax1.grid(True, alpha=0.25)
    ax1.legend(loc="lower right", fontsize=8.5)

    # Panel 2: Carrier Duty Cycle Evolution
    ax2 = fig.add_subplot(1, 3, 2)
    ax2.plot(t_axis, duties, color="#00ff66", linewidth=2.0, label="Duty Cycle (%)")
    ax2.axhline(80, color="#ff00ff", linestyle=":", linewidth=2, label="Analog Bug Limit (80%)")
    ax2.set_title("Transmitter Carrier Duty Cycle Analysis", fontsize=11, fontweight="bold", color="#00ff66")
    ax2.set_xlabel("Elapsed Time (s)")
    ax2.set_ylabel("Carrier Duty Cycle (%)")
    ax2.set_ylim(-5, 105)
    ax2.grid(True, alpha=0.25)
    ax2.legend(loc="lower right", fontsize=8.5)

    # Panel 3: Statistical Threat Classification
    ax3 = fig.add_subplot(1, 3, 3)
    categories = ["Ambient Floor", "Digital Burst", "Analog Bug"]
    counts = [threats.count(0), threats.count(1), threats.count(2)]
    colors = ["#4488ff", "#ffbb00", "#ff0055"]
    bars = ax3.bar(categories, counts, color=colors, edgecolor="#ffffff", linewidth=0.8)
    for bar in bars:
        yval = bar.get_height()
        ax3.text(bar.get_x() + bar.get_width() / 2.0, yval + 1, f"{int(yval)}", ha="center", va="bottom", fontsize=9, fontweight="bold")
    ax3.set_title("TSCM Threat Classification Breakdown", fontsize=11, fontweight="bold", color="#ff0055")
    ax3.set_ylabel("Epoch Count")
    ax3.grid(True, alpha=0.25, axis="y")

    fig.tight_layout()
    plot_path = session_dir / "subghz_bug_analysis.png"
    plt.savefig(plot_path, dpi=160, bbox_inches="tight")
    plt.close()

    # Markdown Report
    peak_rssi = max(rssi_inst) if rssi_inst else -100
    avg_rssi = float(np.mean(rssi_inst)) if rssi_inst else -100
    bug_detected = threats.count(2) > 5

    report_path = session_dir / "SUBGHZ_BUG_REPORT.md"
    with open(report_path, "w", encoding="utf-8") as f:
        f.write("# SpyScan Sub-1 GHz RF Bug Hunter Validation Report\n\n")
        f.write(f"- **Generated:** {dt.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"- **Mode:** {'Simulation Demo (--demo)' if is_demo else 'Physical Hardware Scan'}\n")
        f.write(f"- **Monitored Band:** `433.92 MHz` ISM (TI CC1101 Transceiver)\n")
        f.write(f"- **Maximum Peak RSSI:** `{peak_rssi}` dBm\n")
        f.write(f"- **Mean Signal Level:** `{avg_rssi:.1f}` dBm\n")
        f.write(f"- **Analog Bug Confirmed:** `{'YES (Continuous Carrier)' if bug_detected else 'NO'}`\n\n")
        f.write("## 1. Physical Principle & TSCM Methodology\n\n")
        f.write("Covert audio transmitters ('bugs'), wireless microphones, and analog wiretaps often transmit outside standard Wi-Fi frequencies to avoid leaving IP logs on local routers. These devices utilize the unlicensed Sub-1 GHz ISM bands (**433.92 MHz**, **868.3 MHz**, or **315 MHz**).\n\n")
        f.write("### Continuous Carrier vs Packet Discrimination\n")
        f.write("- **Digital Devices (Keyfobs, Sensors):** Transmit brief, bursty packets (10–40 ms duration) with duty cycles $< 15\\%$.\n")
        f.write("- **Analog Audio Bugs:** Modulate sound continuously over FM or AM carriers. The transmitter exhibits a **100% duty cycle** and extremely stable carrier amplitude ($\\\\sigma < 3.5\\text{ dB}$).\n\n")
        f.write("## 2. Experimental Data Analysis\n\n")
        f.write("![Sub-1 GHz Analysis](subghz_bug_analysis.png)\n\n")
        f.write("### Telemetry Metrics Table\n\n")
        f.write("| Parameter | Ambient Background | Digital Keyfob | Analog Bug | Unit |\n")
        f.write("|---|---|---|---|---|\n")
        f.write(f"| **Signal Level (RSSI)** | < -95 | -65 to -45 | **{peak_rssi}** | dBm |\n")
        f.write(f"| **Duty Cycle** | 0% | 5–25% | **> 85%** | % |\n")
        f.write("| **Carrier Stability ($\\\\sigma$)** | High | High | **< 3.5** | dB |\n")
        f.write(f"| **Verdict** | `CLEAR` | `BURST_PACKET` | `{ '🚨 ANALOG BUG' if bug_detected else 'CLEAR' }` | State |\n\n")
        f.write("## 3. Defense Conclusion\n\n")
        f.write("> [!IMPORTANT]\n")
        f.write("> **Sub-1 GHz TSCM Verdict:** Successfully demonstrates multi-spectrum coverage beyond 2.4 GHz Wi-Fi, providing counter-surveillance detection of hidden RF transmitters and audio bugs.\n")

    return plot_path, report_path


def run_demo(session_dir: pathlib.Path):
    """Executes a simulated demonstration of the Sub-1 GHz RF Bug Hunter."""
    print(f"\n{BOLD}{CYAN}=== SPYSCAN METHOD 4: SUB-1 GHz RF BUG HUNTER (DEMO MODE) ==={RESET}")
    print(f"{DIM}Simulating ambient 433.92 MHz room scan followed by approaching an active analog audio bug...{RESET}\n")

    history = collections.deque(maxlen=60)
    total_steps = 35
    t_start = time.time()

    for step in range(total_steps):
        t_now = t_start + step * 0.15
        is_bug_active = (step >= 12)

        if is_bug_active:
            # Walking closer to analog audio bug: RSSI rises from -72 to -36 dBm
            prog = min(1.0, (step - 12) / 18.0)
            base_rssi = -72.0 + (prog * 36.0)
            noise = np.random.normal(0, 1.2)
            rssi_val = int(base_rssi + noise)
            peak_val = max(rssi_val, int(base_rssi + 2))
            duty_val = int(min(100, 88 + np.random.randint(-4, 9)))
            variance_val = float(np.random.uniform(1.2, 2.4))
            threat_code = 2
            threat_name = "ANALOG_BUG"
        elif step in [4, 5, 8]:
            # Digital keyfob pulse
            rssi_val = -52 + np.random.randint(-3, 4)
            peak_val = rssi_val
            duty_val = 15
            variance_val = 8.5
            threat_code = 1
            threat_name = "BURST_PACKET"
        else:
            # Ambient RF noise floor
            rssi_val = -98 + np.random.randint(-3, 4)
            peak_val = -94
            duty_val = 0
            variance_val = 0.8
            threat_code = 0
            threat_name = "CLEAR"

        sample = {
            "t": t_now, "t_ms": int((t_now - t_start) * 1000), "freq_khz": 433920,
            "rssi_inst": rssi_val, "rssi_mean": rssi_val, "rssi_peak": peak_val,
            "variance": variance_val, "duty": duty_val, "threat_code": threat_code, "threat_name": threat_name
        }
        history.append(sample)

        # UI Rendering
        if threat_code == 2:
            status_badge = f"{BG_RED}{WHITE}{BOLD} 🚨 THREAT DETECTED: ANALOG AUDIO BUG (CONTINUOUS CARRIER) {RESET}"
            dist_str = estimate_subghz_distance(rssi_val)
        elif threat_code == 1:
            status_badge = f"{BG_YELLOW}{WHITE}{BOLD} 📡 INTERMITTENT BURST: KEYFOB / REMOTE TRANSMITTER {RESET}"
            dist_str = estimate_subghz_distance(rssi_val)
        else:
            status_badge = f"{BG_GREEN}{WHITE}{BOLD} ❄️ AMBIENT RF FLOOR: NO ACTIVE TRANSMITTERS {RESET}"
            dist_str = "> 3 m"

        sys.stdout.write(CLEAR_SCREEN)
        print(f"{BOLD}╔═══════════════════════════════════════════════════════════════════════════╗{RESET}")
        print(f"{BOLD}║      SPYSCAN METHOD 4: SUB-1 GHz RF BUG HUNTER (TI CC1101 TRANSCEIVER)    ║{RESET}")
        print(f"{BOLD}╚═══════════════════════════════════════════════════════════════════════════╝{RESET}")
        print(f"Status: {status_badge}")
        print(f"Radio: {CYAN}CC1101 Online (Part=0x00, Ver=0x14){RESET} | Freq: {BOLD}433.92 MHz{RESET} (Channel BW: 203 kHz)")
        print(f"Metrics: Instant={rssi_val:4d} dBm | Peak={peak_val:4d} dBm | Duty={duty_val:3d}% | Variance={variance_val:3.1f} dB")
        print(f"Estimated Proximity: {BOLD}{dist_str}{RESET}  |  Classification: {MAGENTA}{BOLD}{threat_name}{RESET}")
        print("-" * 75)
        print(f"{BOLD}Rolling RF Carrier Waterfall (Recent 8 Epochs):{RESET}")
        waterfall = render_ascii_waterfall(history, width=42, rows=8)
        for w_line in waterfall:
            print(w_line)
        print("-" * 75)
        print(f"Step {step+1}/{total_steps} | Simulated Time: {step*0.15:.1f}s")

        time.sleep(0.08)

    print(f"\n{BOLD}{GREEN}✓ Simulation sequence completed.{RESET}")
    print(f"Generating publication plots and scientific report in {session_dir}...")
    plot_p, rpt_p = generate_plots_and_report(session_dir, list(history), is_demo=True)
    print(f"{GREEN}✓ Analysis Plot Saved:{RESET} {plot_p}")
    print(f"{GREEN}✓ Defense Report Saved:{RESET} {rpt_p}")


def run_live(port: str, baud: int, freq_mhz: int, beep: bool, session_dir: pathlib.Path):
    """Connects to physical ESP32-S3 DevKit and monitors CC1101."""
    log_path = session_dir / "session.log"
    log_file = open(log_path, "w", encoding="utf-8")

    receiver = SubGhzReceiver(port, baud, log_file)
    receiver.start()

    print(f"{BOLD}{GREEN}Connected to ESP32-S3 sniffer on {port}.{RESET}")
    print(f"{CYAN}Configuring CC1101 frequency to {freq_mhz} MHz...{RESET}")
    time.sleep(0.5)
    receiver.send_cmd(f"subghz {freq_mhz}")

    last_beep_time = 0.0

    with NonBlockingInput() as nbi:
        try:
            while True:
                ch = nbi.get_char()
                if ch:
                    if ch in ["q", "Q", "\x03"]:
                        break
                    elif ch == "1":
                        receiver.send_cmd("subghz 433")
                    elif ch == "2":
                        receiver.send_cmd("subghz 868")
                    elif ch == "3":
                        receiver.send_cmd("subghz 315")
                    elif ch == "s":
                        receiver.send_cmd("subghz sweep")
                    elif ch == "o":
                        receiver.send_cmd("subghz off")

                with receiver.lock:
                    sample = receiver.latest_sample
                    status = receiver.chip_status
                    hist = list(receiver.history)

                if sample:
                    rssi_val = sample["rssi_inst"]
                    peak_val = sample["rssi_peak"]
                    duty_val = sample["duty"]
                    variance_val = sample["variance"]
                    threat_code = sample["threat_code"]
                    threat_name = sample["threat_name"]
                    freq_khz = sample["freq_khz"]

                    if threat_code == 2:
                        status_badge = f"{BG_RED}{WHITE}{BOLD} 🚨 THREAT DETECTED: ANALOG AUDIO BUG (CONTINUOUS CARRIER) {RESET}"
                        dist_str = estimate_subghz_distance(rssi_val)
                    elif threat_code == 1:
                        status_badge = f"{BG_YELLOW}{WHITE}{BOLD} 📡 INTERMITTENT BURST: KEYFOB / REMOTE TRANSMITTER {RESET}"
                        dist_str = estimate_subghz_distance(rssi_val)
                    else:
                        status_badge = f"{BG_GREEN}{WHITE}{BOLD} ❄️ AMBIENT RF FLOOR: NO ACTIVE TRANSMITTERS {RESET}"
                        dist_str = "> 3 m"

                    sys.stdout.write(CLEAR_SCREEN)
                    print(f"{BOLD}╔═══════════════════════════════════════════════════════════════════════════╗{RESET}")
                    print(f"{BOLD}║      SPYSCAN METHOD 4: SUB-1 GHz RF BUG HUNTER (TI CC1101 TRANSCEIVER)    ║{RESET}")
                    print(f"{BOLD}╚═══════════════════════════════════════════════════════════════════════════╝{RESET}")
                    print(f"Status: {status_badge}")
                    print(f"Hardware: {CYAN}CC1101 {status}{RESET} | Frequency: {BOLD}{freq_khz/1000:.2f} MHz{RESET} (IF: 152 kHz)")
                    print(f"Metrics: Instant={rssi_val:4d} dBm | Peak={peak_val:4d} dBm | Duty={duty_val:3d}% | Variance={variance_val:3.1f} dB")
                    print(f"Estimated Proximity: {BOLD}{dist_str}{RESET}  |  Classification: {MAGENTA}{BOLD}{threat_name}{RESET}")
                    print("-" * 75)
                    print(f"{BOLD}Rolling RF Carrier Waterfall (Recent 8 Epochs):{RESET}")
                    waterfall = render_ascii_waterfall(hist, width=42, rows=8)
                    for w_line in waterfall:
                        print(w_line)
                    print("-" * 75)
                    print(f"{BOLD}Controls:{RESET} [1]=433 MHz [2]=868 MHz [3]=315 MHz [s]=Sweep [o]=Off [q]=Quit & Report")

                    # Geiger proximity beep
                    if beep and rssi_val >= -75:
                        now = time.time()
                        interval = max(0.08, min(1.2, (-25.0 - rssi_val) / 50.0))
                        if now - last_beep_time >= interval:
                            sys.stdout.write("\a")
                            sys.stdout.flush()
                            last_beep_time = now

                time.sleep(0.1)

        except KeyboardInterrupt:
            pass
        finally:
            receiver.close()
            log_file.close()

    print(f"\n{GREEN}Session ended. Processing data and generating report...{RESET}")
    if receiver.history:
        plot_p, rpt_p = generate_plots_and_report(session_dir, receiver.history, is_demo=False)
        if plot_p:
            print(f"{GREEN}✓ Analysis Plot Saved:{RESET} {plot_p}")
        if rpt_p:
            print(f"{GREEN}✓ Defense Report Saved:{RESET} {rpt_p}")
    else:
        print("No telemetry frames captured during session.")


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--port", default="", help="Serial port of ESP32-S3 DevKit (auto-detects if omitted)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default 115200)")
    parser.add_argument("--freq", type=int, default=433, choices=[315, 433, 868], help="Operating frequency in MHz (default 433)")
    parser.add_argument("--beep", action="store_true", help="Audible Geiger-counter style proximity beeps")
    parser.add_argument("--demo", action="store_true", help="Run simulated demonstration for testing & reporting")
    args = parser.parse_args()

    port = auto_detect_dev_port() if not args.port else args.port
    timestamp = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    session_dir = DATA_DIR / f"{timestamp}_subghz"
    session_dir.mkdir(parents=True, exist_ok=True)

    if args.demo:
        run_demo(session_dir)
    else:
        if not HAS_SERIAL:
            sys.exit("pyserial is not installed: run `pip install pyserial`")
        run_live(port, args.baud, args.freq, args.beep, session_dir)


if __name__ == "__main__":
    main()
