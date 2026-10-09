#!/usr/bin/env python3
"""SpyScan E2 — capture tool for the ESP32-S3 sniffer firmware.

Modes
  survey     hop channels 1..13 and print the busiest devices (find the camera's channel)
  record     lock a channel and log for N seconds (baseline / free recording)
  challenge  lock a channel, run the MOVE / STILL stimulus protocol, then analyse

Every run creates  experiments/E2_wifi_traffic/data/<timestamp>_<mode>/
  raw.csv    host_time_s,line   (every line received from the device, untouched)
  meta.json  parameters, stimulus sequence, notes

Examples
  python tools/e2/e2_capture.py --port /dev/cu.usbserial-0001 survey --seconds 40
  python tools/e2/e2_capture.py --port /dev/cu.usbserial-0001 challenge --channel 6 --k 12 --t 5 \
         --note "ESP32-CAM 2 m, phone viewing stream"
"""
import argparse
import datetime as dt
import json
import pathlib
import random
import sys
import threading
import time

try:
    import serial  # pyserial
except ImportError:
    sys.exit("pyserial missing:  pip install pyserial numpy pandas matplotlib")

ROOT = pathlib.Path(__file__).resolve().parents[2]
DATA = ROOT / "experiments" / "E2_wifi_traffic" / "data"


class Device:
    """Background reader that logs every line with the host timestamp."""

    def __init__(self, port: str, baud: int, raw_path: pathlib.Path):
        # Keep DTR/RTS low on open: on the ESP32-S3 USB Serial/JTAG port a DTR/RTS
        # toggle can reset the chip (or drop it into download mode).
        self.ser = serial.Serial()
        self.ser.port, self.ser.baudrate, self.ser.timeout = port, baud, 0.2
        self.ser.dtr = False
        self.ser.rts = False
        self.ser.open()
        self.f = open(raw_path, "w", encoding="utf-8", newline="")
        self.f.write("host_time_s,line\n")
        self.lines = 0
        self.last_info = ""
        self.watch = ""            # MAC whose uplink is shown live (lower case)
        self._tx_hist = []         # (host_time, tx_bytes) for the watched MAC
        self._stop = False
        self.t = threading.Thread(target=self._run, daemon=True)
        self.t.start()

    def _run(self):
        buf = b""
        while not self._stop:
            chunk = self.ser.read(4096)
            if not chunk:
                continue
            buf += chunk
            *lines, buf = buf.split(b"\n")
            now = time.time()
            for raw in lines:
                s = raw.decode("utf-8", "replace").strip()
                if not s or s[0] not in "IABMP" or s[1:2] != ",":
                    continue                       # skip boot noise / garbage
                esc = s.replace('"', '""')
                self.f.write(f'{now:.3f},"{esc}"\n')
                self.lines += 1
                if s[0] == "I":
                    self.last_info = s
                elif s[0] == "B" and self.watch and f",{self.watch}," in s:
                    try:
                        self._tx_hist.append((now, int(s.split(",")[4])))
                    except (IndexError, ValueError):
                        pass

    def watch_kBps(self, window_s: float = 2.0) -> float:
        """Uplink of the watched MAC over the last window, kB/s."""
        t = time.time()
        self._tx_hist = [(h, b) for h, b in self._tx_hist if t - h <= window_s]
        return sum(b for _, b in self._tx_hist) / window_s / 1000

    def cmd(self, text: str):
        self.ser.write((text + "\n").encode())
        self.ser.flush()

    def close(self):
        self._stop = True
        self.t.join(timeout=1)
        self.f.close()
        self.ser.close()


def big(text: str, sub: str = ""):
    """Very visible prompt in a plain terminal."""
    sys.stdout.write("\033[2J\033[H")      # clear screen
    bar = "#" * 60
    print(bar)
    print()
    print(f"        {text}")
    print()
    print(bar)
    if sub:
        print(sub)
    sys.stdout.write("\a")                 # terminal bell
    sys.stdout.flush()


def balanced_prbs(k: int, seed: int):
    if k % 2:
        raise SystemExit("K must be even (balanced stimulus)")
    if k < 10:
        print("WARNING: K < 10 -> a p < 0.01 result is mathematically impossible "
              "(see docs/architecture/ARCHITECTURE.md §3.2).")
    s = [1] * (k // 2) + [0] * (k // 2)
    random.Random(seed).shuffle(s)
    return s


def run_dir(mode: str) -> pathlib.Path:
    d = DATA / f"{dt.datetime.now():%Y%m%d_%H%M%S}_{mode}"
    d.mkdir(parents=True, exist_ok=True)
    return d


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", required=True)
    ap.add_argument("--baud", type=int, default=921600)
    ap.add_argument("--bin", type=int, default=100, help="bin length on the device, ms")
    ap.add_argument("--note", default="", help="free text stored in meta.json (setup, distances, devices)")
    sub = ap.add_subparsers(dest="mode", required=True)

    s = sub.add_parser("survey")
    s.add_argument("--seconds", type=int, default=40)
    s.add_argument("--dwell", type=int, default=500, help="hop dwell per channel, ms")

    r = sub.add_parser("record")
    r.add_argument("--channel", type=int, required=True)
    r.add_argument("--seconds", type=int, default=60)
    r.add_argument("--focus", default="", help="MAC to capture per-packet (periodicity, Method 1.2)")

    c = sub.add_parser("challenge")
    c.add_argument("--channel", type=int, required=True)
    c.add_argument("--k", type=int, default=12, help="number of slots (even, >= 10)")
    c.add_argument("--t", type=float, default=5.0, help="slot duration, s")
    c.add_argument("--baseline", type=float, default=20.0,
                   help="STILL warm-up before the first slot, s (video streams need ~20 s to stabilise)")
    c.add_argument("--seed", type=int, default=None)
    c.add_argument("--stimulus", choices=["motion", "light", "cover", "flash"], default="motion",
                   help="motion: move/still in view; light: room light on/off; "
                        "cover: cover/uncover the lens; flash: phone flashlight into the lens on/off")
    c.add_argument("--target", default="", help="ground-truth camera MAC (for the record), optional")

    a = ap.parse_args()
    # Open the port BEFORE creating the run folder, so a failure leaves no empty folder.
    try:
        probe = serial.Serial(a.port, a.baud, timeout=0.2)
        probe.close()
    except serial.SerialException as e:
        sys.exit(f"Cannot open {a.port}: {e}\n"
                 "-> Close any other serial monitor (idf.py monitor, PlatformIO, Arduino), check the cable,\n"
                 "   and list ports with:  ls /dev/cu.*")
    out = run_dir(a.mode)
    meta = {"mode": a.mode, "started": dt.datetime.now().isoformat(timespec="seconds"),
            "port": a.port, "bin_ms": a.bin, "note": a.note}

    dev = Device(a.port, a.baud, out / "raw.csv")
    time.sleep(0.5)
    dev.cmd("reset")            # clear the device table so APs/devices are re-reported in THIS run
    dev.cmd(f"bin {a.bin}")

    try:
        if a.mode == "survey":
            dev.cmd(f"hop {a.dwell}")
            meta.update(seconds=a.seconds, dwell_ms=a.dwell)
            t0 = time.time()
            while time.time() - t0 < a.seconds:
                print(f"\rsurvey {time.time() - t0:5.1f}/{a.seconds}s  lines={dev.lines}  {dev.last_info[:70]}", end="")
                time.sleep(0.5)
            print()

        elif a.mode == "record":
            dev.cmd(f"ch {a.channel}")
            meta.update(channel=a.channel, seconds=a.seconds, focus=a.focus)
            if a.focus:
                time.sleep(0.2)
                dev.cmd(f"focus {a.focus}")      # per-packet capture of this MAC
            dev.cmd("mark record_start")
            t0 = time.time()
            while time.time() - t0 < a.seconds:
                print(f"\rrecord {time.time() - t0:5.1f}/{a.seconds}s  lines={dev.lines}", end="")
                time.sleep(0.5)
            if a.focus:
                dev.cmd("focus off")
            dev.cmd("mark record_end")
            print()

        else:  # challenge
            seed = a.seed if a.seed is not None else int(time.time()) & 0xFFFF
            stim = balanced_prbs(a.k, seed)
            on_txt, off_txt = {
                "motion": ("MOVE  (wave / walk in view)", "STAY STILL"),
                "light":  ("LIGHTS ON", "LIGHTS OFF"),
                "cover":  ("COVER THE LENS  (hand / cup)", "UNCOVER - lens free"),
                "flash":  ("FLASHLIGHT INTO THE LENS", "FLASHLIGHT OFF"),
            }[a.stimulus]
            meta.update(channel=a.channel, k=a.k, t_s=a.t, baseline_s=a.baseline, seed=seed,
                        stimulus=a.stimulus, stim=stim, target=a.target)
            dev.cmd(f"ch {a.channel}")
            time.sleep(0.3)

            dev.watch = a.target.lower()
            dev.cmd("mark baseline")
            t_end = time.time() + a.baseline
            while time.time() < t_end:
                live = (f"   target uplink {dev.watch_kBps():6.1f} kB/s" if dev.watch else
                        "   (tip: pass --target <MAC> to see its live uplink)")
                big(off_txt, f"warm-up  {t_end - time.time():4.1f} s   (challenge starts next){live}")
                time.sleep(0.5)
            if dev.watch and dev.watch_kBps() < 5:
                print(f"\nWARNING: target {dev.watch} uplink is only {dev.watch_kBps():.1f} kB/s - "
                      "is it really streaming on channel", a.channel, "(2.4 GHz)?")

            for k, sk in enumerate(stim):
                dev.cmd(f"mark slot,{k},{sk}")
                t_end = time.time() + a.t
                while time.time() < t_end:
                    live = f"   target {dev.watch_kBps():6.1f} kB/s" if dev.watch else ""
                    big(on_txt if sk else off_txt,
                        f"slot {k + 1}/{a.k}   {t_end - time.time():4.1f} s left   lines={dev.lines}{live}")
                    time.sleep(0.25)
            dev.cmd("mark end")
            time.sleep(0.5)
            big("DONE", f"data in {out}")
    finally:
        time.sleep(0.5)
        meta["ended"] = dt.datetime.now().isoformat(timespec="seconds")
        meta["lines"] = dev.lines
        dev.close()
        (out / "meta.json").write_text(json.dumps(meta, indent=2))

    print(f"\nSaved {dev.lines} lines to {out}")
    # analyse immediately
    try:
        from e2_analyze import analyse
    except ImportError:
        sys.path.insert(0, str(pathlib.Path(__file__).parent))
        from e2_analyze import analyse
    analyse(out)


if __name__ == "__main__":
    main()
