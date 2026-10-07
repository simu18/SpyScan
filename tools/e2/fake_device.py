#!/usr/bin/env python3
"""Fake E2 sniffer for testing the PC tools without hardware.

Opens a pseudo-terminal, prints its path, and speaks the firmware protocol.
A simulated camera (24:6f:28:aa:bb:cc) raises its uplink when the PC sends
'mark slot,<k>,1' — i.e. it plays the role of a camera filming the stimulus.
SYNTHETIC DATA ONLY — never use its output as a measurement.
"""
import os, pty, random, select, time

DEVS = [  # mac, role, tx Bps, rx Bps, responds, rssi
    ("50:c7:bf:12:34:56", "A", 70000, 0, 1.0, -35),     # router (forwards to viewer)
    ("24:6f:28:aa:bb:cc", "S", 60000, 1500, 1.8, -45),  # camera
    ("5a:11:22:33:44:55", "S", 2500, 60000, 1.8, -40),  # phone viewing the stream
    ("3c:a9:f4:01:02:03", "S", 15000, 150000, 1.0, -50),# laptop, bursty
    ("24:6f:28:10:20:30", "S", 300, 300, 1.0, -55),     # smart plug
]
BURST = [0.0, 0.25, 0.3, 1.2, 0.5]

def main():
    master, slave = pty.openpty()
    print(os.ttyname(slave), flush=True)
    t0 = time.time(); bin_ms = 100; ch = 1; stim = 0; buf = b""
    nxt = time.time() + bin_ms / 1000
    def out(s): os.write(master, (s + "\n").encode())
    out("I,0,boot,fake-e2,v0.1")
    while True:
        r, _, _ = select.select([master], [], [], 0.01)
        t_ms = int((time.time() - t0) * 1000)
        if r:
            buf += os.read(master, 1024)
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1); line = line.decode().strip()
                if line.startswith("mark "):
                    lab = line[5:]; out(f"M,{t_ms},{lab}")
                    if lab.startswith("slot,"): stim = int(lab.split(",")[2])
                    elif lab in ("end", "baseline"): stim = 0
                elif line.startswith("bin "): bin_ms = int(line[4:]); out(f"I,{t_ms},bin,{bin_ms}")
                elif line.startswith("ch "): ch = int(line[3:]); out(f"I,{t_ms},channel,{ch}")
                elif line.startswith("hop"): out(f"I,{t_ms},hop,{line[4:]}")
        if time.time() >= nxt:
            nxt += bin_ms / 1000
            for i, (mac, role, tx, rx, g, rssi) in enumerate(DEVS):
                k = g if stim else 1.0
                txb = max(0, int(tx * bin_ms / 1000 * k * (1 + BURST[i] * random.gauss(0, 1))))
                rxb = max(0, int(rx * bin_ms / 1000 * (k if i == 2 else 1)))
                if txb or rxb:
                    out(f"B,{t_ms},6,{mac},{txb},{max(1, txb // 1200)},{rxb},{rxb // 1200},"
                        f"{rssi + random.randint(-3, 3)},{role},50:c7:bf:12:34:56")
            if random.random() < 0.02: out(f"A,{t_ms},6,50:c7:bf:12:34:56,-35,HomeNet")

if __name__ == "__main__":
    main()
