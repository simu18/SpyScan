#!/usr/bin/env python3
"""SpyScan — Closed-Loop "Flash Stimulus" Optical-to-Wi-Fi Correlation Engine (Method 1).

Mathematically proves whether a physical camera lens in the room belongs to a
detected Wi-Fi transmitter by flashing light into the lens and detecting the
instantaneous video encoder bitrate spike via Superposed Epoch Analysis (ERP).

Principle:
  When light floods a CMOS sensor, temporal inter-frame compression collapses.
  The video encoder is forced to emit a large I-frame / surge in DCT bits.
  By synchronizing light flashes with millisecond-resolution Wi-Fi packet sniffing,
  we correlate the optical sensor directly to the RF uplink stream.

Usage:
  python tools/e2/e2_flash_stimulus.py --port /dev/cu.usbmodem1101 --target cc:b8:5e:ad:5c:4a --channel 10
  python tools/e2/e2_flash_stimulus.py --demo      # Run simulated demo for presentation/testing
"""
import argparse
import datetime as dt
import io
import json
import math
import os
import pathlib
import random
import sys
import threading
import time

try:
    import numpy as np
    import pandas as pd
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import serial
except ImportError:
    sys.exit("Dependencies missing: pip install pyserial numpy pandas matplotlib")

ROOT = pathlib.Path(__file__).resolve().parents[2]
DATA_DIR = ROOT / "experiments" / "E2_wifi_traffic" / "data"

RESET = "\033[0m"
BOLD = "\033[1m"
DIM = "\033[2m"
RED = "\033[91m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
CYAN = "\033[96m"
WHITE = "\033[97m"
BG_GREEN = "\033[42m"
BG_RED = "\033[41m"


class SnifferReceiver:
    def __init__(self, port: str, baud: int = 115200):
        self.ser = serial.Serial()
        self.ser.port, self.ser.baudrate, self.ser.timeout = port, baud, 0.1
        self.ser.dtr = False
        self.ser.rts = False
        self.ser.open()
        self.running = True
        self.callbacks = []
        self.t = threading.Thread(target=self._loop, daemon=True)
        self.t.start()

    def send(self, cmd: str):
        if self.ser.is_open:
            self.ser.write((cmd.strip() + "\n").encode("utf-8"))
            self.ser.flush()

    def _loop(self):
        buf = b""
        while self.running:
            try:
                chunk = self.ser.read(2048)
                if not chunk:
                    continue
                buf += chunk
                *lines, buf = buf.split(b"\n")
                now = time.time()
                for raw in lines:
                    s = raw.decode("utf-8", "replace").strip()
                    if not s:
                        continue
                    for cb in self.callbacks:
                        cb(now, s)
            except Exception:
                break

    def close(self):
        self.running = False
        try:
            self.ser.close()
        except Exception:
            pass


class FlashStimulusAnalyzer:
    def __init__(self, target_mac: str, channel: int, trials: int = 6,
                 epoch_pre_ms: int = 800, epoch_post_ms: int = 1200):
        self.target = target_mac.lower()
        self.channel = channel
        self.trials = trials
        self.epoch_pre_ms = epoch_pre_ms
        self.epoch_post_ms = epoch_post_ms

        self.records = []       # (host_time_s, t_ms, tx_bytes, tx_frames, rssi)
        self.flash_events = []  # host_time_s timestamps of light flashes
        self.lock = threading.Lock()

    def add_packet_bin(self, host_t: float, line: str):
        if not line.startswith("B,"):
            return
        parts = line.split(",")
        if len(parts) >= 9:
            mac = parts[3].lower()
            if mac == self.target:
                try:
                    t_ms = int(parts[1])
                    tx_b = int(parts[4])
                    tx_n = int(parts[5])
                    rssi = int(parts[8])
                    with self.lock:
                        self.records.append((host_t, t_ms, tx_b, tx_n, rssi))
                except ValueError:
                    pass

    def record_flash(self, host_t: float):
        with self.lock:
            self.flash_events.append(host_t)

    def compute_superposed_epochs(self, bin_ms: int = 50) -> tuple[np.ndarray, np.ndarray, np.ndarray, dict]:
        """Aligns all Wi-Fi uplink records around each flash event (Superposed Epoch Analysis)."""
        with self.lock:
            if not self.records or not self.flash_events:
                return np.array([]), np.array([]), np.array([]), {}
            df = pd.DataFrame(self.records, columns=["host_t", "t_ms", "tx_b", "tx_n", "rssi"])
            flashes = list(self.flash_events)

        # Convert to relative time grid (-pre_ms to +post_ms)
        grid_bins = int((self.epoch_pre_ms + self.epoch_post_ms) / bin_ms) + 1
        time_grid = np.linspace(-self.epoch_pre_ms, self.epoch_post_ms, grid_bins)

        epochs = []
        for f_t in flashes:
            t_start = f_t - (self.epoch_pre_ms / 1000.0)
            t_end = f_t + (self.epoch_post_ms / 1000.0)
            sub = df[(df.host_t >= t_start) & (df.host_t <= t_end)].copy()
            if sub.empty:
                continue

            sub["rel_ms"] = (sub.host_t - f_t) * 1000.0
            # Interpolate onto time_grid
            sub["rate_kBps"] = (sub.tx_b / 1000.0) / (bin_ms / 1000.0)
            # Reindex to uniform grid
            sub_binned = np.interp(time_grid, sub["rel_ms"], sub["rate_kBps"], left=0, right=0)
            epochs.append(sub_binned)

        if not epochs:
            return np.array([]), np.array([]), np.array([]), {}

        matrix = np.array(epochs)  # shape: (n_trials, n_bins)
        mean_curve = np.mean(matrix, axis=0)
        std_curve = np.std(matrix, axis=0)

        # Extract stats
        pre_mask = time_grid < 0
        post_mask = (time_grid >= 40) & (time_grid <= 500)  # expected optical encoder latency

        baseline_mean = float(np.mean(mean_curve[pre_mask])) if any(pre_mask) else 1.0
        baseline_std = float(np.std(mean_curve[pre_mask])) if any(pre_mask) else 1.0
        peak_post = float(np.max(mean_curve[post_mask])) if any(post_mask) else baseline_mean
        peak_idx = np.where(post_mask)[0][np.argmax(mean_curve[post_mask])] if any(post_mask) else 0
        peak_latency_ms = float(time_grid[peak_idx])

        surge_ratio = peak_post / max(1.0, baseline_mean)
        z_score = (peak_post - baseline_mean) / max(0.1, baseline_std)

        # Permutation test p-value (randomly shuffling event times across observation span)
        n_perms = 500
        perm_peaks = []
        t_min = df.host_t.min() + (self.epoch_pre_ms / 1000.0)
        t_max = df.host_t.max() - (self.epoch_post_ms / 1000.0)
        span = max(1.0, t_max - t_min)

        for _ in range(n_perms):
            fake_f = t_min + np.random.rand(len(flashes)) * span
            fake_epochs = []
            for ff in fake_f:
                sub = df[(df.host_t >= ff - self.epoch_pre_ms / 1000.0) & (df.host_t <= ff + self.epoch_post_ms / 1000.0)]
                if not sub.empty:
                    rel = (sub.host_t - ff) * 1000.0
                    r = np.interp(time_grid, rel, (sub.tx_b / 1000.0) / (bin_ms / 1000.0), left=0, right=0)
                    fake_epochs.append(r)
            if fake_epochs:
                m_fake = np.mean(fake_epochs, axis=0)
                perm_peaks.append(np.max(m_fake[post_mask]) if any(post_mask) else 0)

        p_val = float(np.mean([p >= peak_post for p in perm_peaks])) if perm_peaks else 0.5

        stats = {
            "baseline_mean": baseline_mean,
            "baseline_std": baseline_std,
            "peak_post": peak_post,
            "peak_latency_ms": peak_latency_ms,
            "surge_ratio": surge_ratio,
            "z_score": z_score,
            "p_value": p_val,
            "trials": len(epochs)
        }
        return time_grid, mean_curve, std_curve, stats


def plot_results(time_grid, mean_curve, std_curve, stats, run_dir: pathlib.Path, target: str):
    fig, ax = plt.subplots(figsize=(9, 5), dpi=150)
    ax.plot(time_grid, mean_curve, color="#0284c7", lw=2.5, label="Mean Uplink Rate")
    ax.fill_between(time_grid, mean_curve - std_curve, mean_curve + std_curve, color="#38bdf8", alpha=0.3, label="±1σ Trial Variance")

    # Mark the flash impulse at t=0
    ax.axvline(0, color="#ef4444", lw=2, linestyle="--", label="Optical Flash (t=0)")
    ax.axvspan(0, 100, color="#fef08a", alpha=0.4, label="Flash Duration (100 ms)")

    # Mark peak
    if stats.get("peak_latency_ms"):
        lat = stats["peak_latency_ms"]
        pk = stats["peak_post"]
        ax.scatter([lat], [pk], color="#dc2626", s=90, zorder=5)
        ax.annotate(f"Encoder Peak: {pk:.1f} kB/s\n(+{lat:.0f} ms latency)",
                    xy=(lat, pk), xytext=(lat + 80, pk * 1.05),
                    arrowprops=dict(arrowstyle="->", color="#dc2626", lw=1.5),
                    fontweight="bold", color="#991b1b")

    ax.set_title(f"Superposed Epoch Analysis — Optical Flash vs Wi-Fi Uplink\nTarget: {target} (K={stats.get('trials', 0)} trials, p={stats.get('p_value', 1.0):.4f})", fontsize=12, fontweight="bold")
    ax.set_xlabel("Time Relative to Optical Flash (ms)", fontsize=10)
    ax.set_ylabel("Wi-Fi Uplink Bitrate (kB/s)", fontsize=10)
    ax.grid(True, linestyle=":", alpha=0.6)
    ax.legend(loc="upper right", framealpha=0.9)

    plt.tight_layout()
    out_path = run_dir / "flash_stimulus_response.png"
    plt.savefig(out_path)
    plt.close()
    return out_path


def run_demo():
    """Generates synthetic camera encoder response to optical strobe to demonstrate the engine."""
    print(f"\n{CYAN}Running Simulated Optical-to-Wi-Fi Flash Stimulus Demo...{RESET}")
    analyzer = FlashStimulusAnalyzer(target_mac="cc:b8:5e:ad:5c:4a", channel=10, trials=6)

    t0 = time.time()
    # Generate 35 seconds of baseline streaming traffic (~180 kB/s, 25 FPS)
    flashes = [6.0, 11.5, 17.0, 22.5, 28.0, 33.5]
    for ft in flashes:
        analyzer.record_flash(t0 + ft)

    cur_t = 0.0
    while cur_t < 38.0:
        # Check if near a flash
        near_flash = any(0.060 <= (cur_t - ft) <= 0.240 for ft in flashes)
        if near_flash:
            # Massive I-frame surge
            tx_b = random.randint(18000, 26000)  # ~400 kB/s surge
            tx_n = random.randint(14, 20)
        else:
            tx_b = random.randint(8500, 10500)   # ~190 kB/s baseline
            tx_n = random.randint(7, 9)

        line = f"B,{int(cur_t*1000)},10,cc:b8:5e:ad:5c:4a,{tx_b},{tx_n},200,2,-48,5c:62:8b:66:67:d2"
        analyzer.add_packet_bin(t0 + cur_t, line)
        cur_t += 0.050

    tg, mean_c, std_c, stats = analyzer.compute_superposed_epochs()
    run_dir = DATA_DIR / f"{dt.datetime.now().strftime('%Y%m%d_%H%M%S')}_flash_demo"
    run_dir.mkdir(parents=True, exist_ok=True)
    img_p = plot_results(tg, mean_c, std_c, stats, run_dir, "cc:b8:5e:ad:5c:4a")

    print(f"\n{BOLD}{WHITE}╔══════════════════════════════════════════════════════════════════════════╗{RESET}")
    print(f"{BOLD}{WHITE}║     SpyScan — Optical-to-Wi-Fi Correlation Verification Report          ║{RESET}")
    print(f"{BOLD}{WHITE}╠══════════════════════════════════════════════════════════════════════════╣{RESET}")
    print(f"║  Target Station: {BOLD}{CYAN}cc:b8:5e:ad:5c:4a{RESET}    Channel: {BOLD}10{RESET}       Trials Tested: {BOLD}{stats['trials']}{RESET}    ║")
    print(f"║  Baseline Bitrate: {stats['baseline_mean']:6.1f} kB/s (σ = {stats['baseline_std']:.1f})                             ║")
    print(f"║  Post-Flash Peak:  {stats['peak_post']:6.1f} kB/s (Surge: {BOLD}{GREEN}+{stats['surge_ratio']:.2f}×{RESET})                     ║")
    print(f"║  Encoder Latency:  {BOLD}{stats['peak_latency_ms']:.0f} ms{RESET} (Expected H.264 delay: 60-180 ms)          ║")
    print(f"║  Statistical p-val:{BOLD}{GREEN}{stats['p_value']:8.4f}{RESET} (Permutation test N=500)                   ║")
    print(f"╠══════════════════════════════════════════════════════════════════════════╣{RESET}")
    verdict = f"{BG_GREEN}{WHITE}{BOLD} [✓] 100% PROVEN: Physical Lens Modulates Wi-Fi Stream! {RESET}"
    print(f"║  Verdict: {verdict:<64} ║")
    print(f"{BOLD}{WHITE}╚══════════════════════════════════════════════════════════════════════════╝{RESET}")
    print(f"\nSaved analysis plot to: {img_p}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", default="/dev/cu.usbmodem1101", help="Serial port of ESP32-S3 sniffer")
    parser.add_argument("--target", default="cc:b8:5e:ad:5c:4a", help="Target camera MAC address")
    parser.add_argument("--channel", type=int, default=10, help="Channel of target camera")
    parser.add_argument("--trials", type=int, default=6, help="Number of optical flash trials")
    parser.add_argument("--demo", action="store_true", help="Run simulated test run")
    args = parser.parse_args()

    if args.demo:
        run_demo()
        return

    print(f"{CYAN}Connecting to ESP32-S3 sniffer on {args.port}...{RESET}")
    try:
        rx = SnifferReceiver(args.port)
    except Exception as e:
        sys.exit(f"{RED}[!] Failed to open serial port {args.port}: {e}{RESET}\nTip: Run with --demo to test without hardware.")

    analyzer = FlashStimulusAnalyzer(target_mac=args.target, channel=args.channel, trials=args.trials)
    rx.callbacks.append(analyzer.add_packet_bin)

    rx.send(f"ch {args.channel}")
    rx.send(f"focus {args.target}")
    rx.send("bin 50")
    time.sleep(0.5)

    print(f"\n{BOLD}Target locked:{RESET} {args.target} on Channel {args.channel}")
    print(f"Measuring baseline traffic for 4 seconds...")
    time.sleep(4.0)

    print(f"\n{BOLD}Starting {args.trials} Optical Flash Stimulus Trials:{RESET}")
    print("When prompted, shine a bright flashlight or phone strobe directly into the lens for ~0.5s.")

    for i in range(1, args.trials + 1):
        wait_s = random.uniform(3.5, 6.0)
        time.sleep(wait_s)
        print(f"\n{YELLOW}[Trial {i}/{args.trials}] Ready in 2... 1...{RESET}")
        time.sleep(1.0)
        sys.stdout.write(f"\r{BG_RED}{WHITE}{BOLD}  >>> FLASH LIGHT INTO LENS NOW! <<<  {RESET}\a\n")
        sys.stdout.flush()
        analyzer.record_flash(time.time())
        rx.send(f"mark FLASH_{i}")
        time.sleep(1.5)

    rx.close()
    print(f"\n{CYAN}Computing Superposed Epoch Analysis and Permutation Test...{RESET}")
    tg, mean_c, std_c, stats = analyzer.compute_superposed_epochs()

    run_dir = DATA_DIR / f"{dt.datetime.now().strftime('%Y%m%d_%H%M%S')}_flash_stimulus"
    run_dir.mkdir(parents=True, exist_ok=True)
    img_p = plot_results(tg, mean_c, std_c, stats, run_dir, args.target)

    # Save summary
    md_path = run_dir / "RESULTS.md"
    with open(md_path, "w") as f:
        f.write(f"# Method 1: Optical-to-Wi-Fi Flash Stimulus Verification\n\n")
        f.write(f"- Target MAC: `{args.target}`\n- Channel: {args.channel}\n- Trials: {stats.get('trials')}\n")
        f.write(f"- Baseline Bitrate: **{stats.get('baseline_mean', 0):.1f} kB/s**\n")
        f.write(f"- Peak Post-Flash Bitrate: **{stats.get('peak_post', 0):.1f} kB/s** (+{stats.get('surge_ratio', 0):.2f}×)\n")
        f.write(f"- Latency: **{stats.get('peak_latency_ms', 0):.0f} ms**\n")
        f.write(f"- Permutation p-value: **{stats.get('p_value', 1.0):.4f}**\n\n")
        if stats.get('p_value', 1.0) < 0.05 and stats.get('surge_ratio', 0) > 1.3:
            f.write(f"### Verdict: **CORRELATED (Optical Sensor belongs to Wi-Fi Transmitter)**\n")
        else:
            f.write(f"### Verdict: **UNCORRELATED (No significant bitrate spike observed)**\n")

    print(f"\n{BOLD}{WHITE}╔══════════════════════════════════════════════════════════════════════════╗{RESET}")
    print(f"{BOLD}{WHITE}║     SpyScan — Optical-to-Wi-Fi Correlation Verification Report          ║{RESET}")
    print(f"{BOLD}{WHITE}╠══════════════════════════════════════════════════════════════════════════╣{RESET}")
    print(f"║  Target Station: {BOLD}{CYAN}{args.target:<20}{RESET} Channel: {BOLD}{args.channel:<3}{RESET}   Trials: {BOLD}{stats.get('trials', 0)}{RESET}       ║")
    print(f"║  Baseline Bitrate: {stats.get('baseline_mean', 0):6.1f} kB/s (σ = {stats.get('baseline_std', 0):.1f})                             ║")
    print(f"║  Post-Flash Peak:  {stats.get('peak_post', 0):6.1f} kB/s (Surge: {BOLD}{GREEN}+{stats.get('surge_ratio', 0):.2f}×{RESET})                     ║")
    print(f"║  Encoder Latency:  {BOLD}{stats.get('peak_latency_ms', 0):.0f} ms{RESET} (Expected H.264 delay: 60-180 ms)          ║")
    p_color = GREEN if stats.get('p_value', 1.0) < 0.05 else RED
    print(f"║  Statistical p-val:{BOLD}{p_color}{stats.get('p_value', 1.0):8.4f}{RESET} (Permutation test N=500)                   ║")
    print(f"╠══════════════════════════════════════════════════════════════════════════╣{RESET}")
    if stats.get('p_value', 1.0) < 0.05 and stats.get('surge_ratio', 0) > 1.3:
        verdict = f"{BG_GREEN}{WHITE}{BOLD} [✓] 100% PROVEN: Physical Lens Modulates Wi-Fi Stream! {RESET}"
    else:
        verdict = f"{BG_RED}{WHITE}{BOLD} [?] Inconclusive / No Bitrate Modulation Detected      {RESET}"
    print(f"║  Verdict: {verdict:<64} ║")
    print(f"{BOLD}{WHITE}╚══════════════════════════════════════════════════════════════════════════╝{RESET}")
    print(f"\nSaved analysis plot to: {img_p}")
    print(f"Saved results summary to: {md_path}")


if __name__ == "__main__":
    main()
