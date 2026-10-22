#!/usr/bin/env python3
"""SpyScan E1 — live laptop viewer for the bring-up firmware (no physical LCD).

The firmware streams a downsampled image over USB each cycle:
  F,<aec>,<led L|A|O>,<w>,<h>,<thr>,<peak_x>,<peak_y>,<base64 of w*h bytes>
LIVE sends the camera luma; DIFF sends the ON-OFF difference. This tool opens
the serial port (so run it INSTEAD of `idf.py monitor`), shows the stream live,
marks the peak, highlights pixels above threshold in red, and saves every line
to a log you can feed to e1_analyze.py.

Usage:
  python3 tools/e1/e1_view.py --port /dev/cu.usbmodem101
  python3 tools/e1/e1_view.py --port /dev/cu.usbmodem101 --log run.log
Then later:
  python3 tools/e1/e1_analyze.py run.log

Needs: pyserial, numpy, matplotlib  (pip install pyserial numpy matplotlib)
"""
import argparse, base64, datetime as dt, pathlib, sys, threading, time

try:
    import numpy as np
    import serial
    import matplotlib
    import matplotlib.pyplot as plt
except ImportError as e:
    sys.exit(f"missing dependency ({e}). Run:  pip install pyserial numpy matplotlib")

ROOT = pathlib.Path(__file__).resolve().parents[2]


class Reader(threading.Thread):
    """Reads serial in the background, keeps the latest frame + metadata."""
    def __init__(self, port, baud, logf):
        super().__init__(daemon=True)
        self.ser = serial.Serial()
        self.ser.port, self.ser.baudrate, self.ser.timeout = port, baud, 0.2
        self.ser.dtr = False          # don't reset the ESP32-S3 USB-JTAG on open
        self.ser.rts = False
        self.ser.open()
        self.logf = logf
        self.lock = threading.Lock()
        self.frame = None             # (img ndarray h x w, meta dict)
        self.last_d = ""
        self._stop = False

    def run(self):
        buf = b""
        while not self._stop:
            chunk = self.ser.read(8192)
            if not chunk:
                continue
            buf += chunk
            *lines, buf = buf.split(b"\n")
            for raw in lines:
                s = raw.decode("utf-8", "replace").strip()
                if not s:
                    continue
                if self.logf and s[0] in "IABMPDF" and s[1:2] == ",":
                    self.logf.write(s + "\n"); self.logf.flush()
                if s.startswith("F,"):
                    self._frame(s)
                elif s[0] in "DMI":
                    self.last_d = s
                    print(s)                       # stats to the terminal

    def _frame(self, s):
        try:
            f = s.split(",", 8)
            aec = int(f[1]); led = f[2]; w = int(f[3]); h = int(f[4])
            thr = float(f[5]); px = int(f[6]); py = int(f[7])
            data = base64.b64decode(f[8])
            if len(data) < w * h:
                return
            img = np.frombuffer(data[:w * h], dtype=np.uint8).reshape(h, w)
            with self.lock:
                self.frame = (img, dict(aec=aec, led=led, thr=thr, px=px, py=py, w=w, h=h))
        except Exception:
            return

    def get(self):
        with self.lock:
            return self.frame

    def close(self):
        self._stop = True
        try: self.ser.close()
        except Exception: pass


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", required=True)
    ap.add_argument("--baud", type=int, default=921600)   # ignored by USB-CDC, harmless
    ap.add_argument("--log", default="")
    a = ap.parse_args()

    if a.log:
        logp = pathlib.Path(a.log)
    else:
        d = ROOT / "experiments" / "E1_optical" / "data"
        d.mkdir(parents=True, exist_ok=True)
        logp = d / f"{dt.datetime.now():%Y%m%d_%H%M%S}_view.log"
    logf = open(logp, "w", encoding="utf-8")
    print(f"logging to {logp}")

    rd = Reader(a.port, a.baud, logf)
    rd.start()

    plt.ion()
    fig, ax = plt.subplots(figsize=(6, 4.6))
    im = ax.imshow(np.zeros((60, 80, 3), dtype=np.uint8), interpolation="nearest")
    dot, = ax.plot([], [], "o", mfc="none", mec="cyan", mew=2, ms=18)
    title = ax.set_title("waiting for data...")
    ax.set_xticks([]); ax.set_yticks([])
    fig.tight_layout()

    LED_NAME = {"L": "LIVE", "A": "AXIS (on-axis)", "O": "OFF-AXIS"}
    try:
        while plt.fignum_exists(fig.number):
            fr = rd.get()
            if fr is not None:
                img, m = fr
                g = img.astype(np.float32)
                # grayscale base, scaled for visibility
                base = np.clip(g * (1.0 if m["led"] == "L" else 6.0), 0, 255).astype(np.uint8)
                rgb = np.stack([base, base, base], axis=-1)
                if m["led"] != "L" and m["thr"] > 0:          # red = above-threshold (diff)
                    hot = img > m["thr"]
                    rgb[hot] = [255, 0, 0]
                im.set_data(rgb)
                # peak marker (full-res coords -> downsampled grid)
                bx = m["px"] * m["w"] / 320.0
                by = m["py"] * m["h"] / 240.0
                if m["led"] != "L":
                    dot.set_data([bx], [by])
                else:
                    dot.set_data([], [])
                tag = LED_NAME.get(m["led"], m["led"])
                extra = "" if m["led"] == "L" else f"  aec={m['aec']}  thr={m['thr']:.0f}  peak@({m['px']},{m['py']})"
                title.set_text(f"{tag}{extra}")
            fig.canvas.draw_idle()
            plt.pause(0.05)
    except KeyboardInterrupt:
        pass
    finally:
        rd.close(); logf.close()
        print(f"\nsaved log: {logp}\nanalyse with:  python3 {ROOT/'tools'/'e1'/'e1_analyze.py'} {logp}")


if __name__ == "__main__":
    main()
