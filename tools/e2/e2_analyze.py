#!/usr/bin/env python3
"""SpyScan E2 — analysis of a capture folder (raw.csv + meta.json).

  python tools/e2/e2_analyze.py experiments/E2_wifi_traffic/data/<run>

Writes into the run folder:
  devices.csv     per-MAC totals (tx/rx rate, frames, RSSI, role, vendor hint)
  aps.csv         access points seen (BSSID, SSID, channel)
  results.csv     challenge only: per-MAC correlation, permutation p, level, role
  RESULTS.md      human-readable summary (paste into experiments/E2_*/RESULTS.md)
  traffic.png     challenge/record: tx rate of the busiest devices + stimulus shading
  lag.png         challenge only: correlation vs. assumed response lag (exploratory)

Statistics (identical to the firmware module challenge.c):
  x_k = uplink (tx) bytes/s of a device in slot k, s_k in {0,1}
  S   = Pearson corr(x, s); p = (1 + #{S_perm >= S}) / (1 + N_perm), balanced permutations
  HIGH p < 0.01, LOW p < 0.05, else NONE.  The lag is fixed BEFORE looking at the data
  (default 0 s); the lag sweep is exploratory and must not be used to pick the p-value.
"""
import csv
import io
import json
import pathlib
import sys

import numpy as np
import pandas as pd

N_PERM = 10000
P_HIGH, P_LOW = 0.01, 0.05
MIN_FRAMES = 50          # devices with fewer transmitted frames in the run are ignored

# Small OUI hint table (first 3 bytes). A hint only — never evidence by itself.
OUI = {
    "24:6f:28": "Espressif", "30:ae:a4": "Espressif", "3c:71:bf": "Espressif",
    "a4:cf:12": "Espressif", "84:f3:eb": "Espressif", "ec:fa:bc": "Espressif",
    "7c:df:a1": "Espressif", "34:85:18": "Espressif", "48:27:e2": "Espressif",
    "50:c7:bf": "TP-Link", "f4:f2:6d": "TP-Link", "ec:08:6b": "TP-Link",
    "3c:a9:f4": "Intel", "00:1b:21": "Intel",
}


def vendor(mac: str) -> str:
    first = int(mac[:2], 16)
    if first & 0x02:
        return "(random)"
    return OUI.get(mac[:8].lower(), "")


def load(run: pathlib.Path):
    rows_b, rows_a, rows_m = [], [], []
    with open(run / "raw.csv", encoding="utf-8") as f:
        rd = csv.reader(f)
        next(rd, None)
        for host_t, line in rd:
            p = line.split(",")
            try:
                if p[0] == "B" and len(p) >= 11:
                    rows_b.append((int(p[1]), int(p[2]), p[3], int(p[4]), int(p[5]),
                                   int(p[6]), int(p[7]), int(p[8]), p[9], p[10]))
                elif p[0] == "A" and len(p) >= 6:
                    rows_a.append((int(p[1]), int(p[2]), p[3], int(p[4]), ",".join(p[5:])))
                elif p[0] == "M" and len(p) >= 3:
                    rows_m.append((int(p[1]), ",".join(p[2:])))
            except ValueError:
                continue                          # corrupted line (serial noise)
    b = pd.DataFrame(rows_b, columns=["t_ms", "ch", "mac", "tx_b", "tx_n", "rx_b", "rx_n", "rssi", "role", "bssid"])
    a = pd.DataFrame(rows_a, columns=["t_ms", "ch", "bssid", "rssi", "ssid"]).drop_duplicates("bssid")
    m = pd.DataFrame(rows_m, columns=["t_ms", "label"])
    meta = json.loads((run / "meta.json").read_text()) if (run / "meta.json").exists() else {}
    return b, a, m, meta


def device_table(b: pd.DataFrame, dur_s: float) -> pd.DataFrame:
    if b.empty:
        return pd.DataFrame()
    g = b.groupby("mac")
    d = pd.DataFrame({
        "tx_kBps": g.tx_b.sum() / dur_s / 1000,
        "rx_kBps": g.rx_b.sum() / dur_s / 1000,
        "tx_frames": g.tx_n.sum(),
        "rx_frames": g.rx_n.sum(),
        "rssi_med": g.rssi.apply(lambda r: float(np.median(r[r != 0])) if (r != 0).any() else np.nan),
        "role": g.role.agg(lambda r: r.mode().iat[0]),
        "bssid": g.bssid.agg(lambda r: r.mode().iat[0]),
        "ch": g.ch.agg(lambda r: r.mode().iat[0]),
    })
    d["vendor"] = [vendor(m) for m in d.index]
    return d.sort_values("tx_kBps", ascending=False)


def slots_from_markers(m: pd.DataFrame):
    """Returns list of (k, s, t_start_ms, t_end_ms)."""
    sl = []
    marks = m.sort_values("t_ms").reset_index(drop=True)
    for i, row in marks.iterrows():
        if row.label.startswith("slot,"):
            _, k, s = row.label.split(",")[:3]
            t_end = marks.t_ms.iat[i + 1] if i + 1 < len(marks) else None
            sl.append((int(k), int(s), int(row.t_ms), t_end))
    return [x for x in sl if x[3] is not None]


def slot_rates(b: pd.DataFrame, mac: str, slots, lag_ms: int = 0, col: str = "tx_b"):
    d = b[b.mac == mac]
    x = []
    for _, _, t0, t1 in slots:
        sel = d[(d.t_ms > t0 + lag_ms) & (d.t_ms <= t1 + lag_ms)]
        x.append(sel[col].sum() / ((t1 - t0) / 1000.0))
    return np.array(x, dtype=float)


def _spearman(a, b):
    ra = np.argsort(np.argsort(a)).astype(float)
    rb = np.argsort(np.argsort(b)).astype(float)
    return corr(rb, ra)


def corr(x, s):
    x = x - x.mean()
    s = s - s.mean()
    den = np.sqrt((x * x).sum() * (s * s).sum())
    return float((x * s).sum() / den) if den > 0 else 0.0


def perm_test(x, s, rng, n=N_PERM):
    obs = corr(x, s)
    perms = np.array([rng.permutation(s) for _ in range(n)], dtype=float)
    xc = x - x.mean()
    pc = perms - perms.mean(axis=1, keepdims=True)
    den = np.sqrt((xc * xc).sum() * (pc * pc).sum(axis=1))
    with np.errstate(invalid="ignore", divide="ignore"):
        c = np.where(den > 0, (pc @ xc) / den, 0.0)
    p = (1 + int((c >= obs - 1e-12).sum())) / (1 + n)
    return obs, p


def _direction(role, lvl, tx_mean, rx_mean):
    """Hypothesis H4: a correlated SOURCE is uplink-dominant, a viewer downlink-dominant."""
    if lvl == "NONE":
        return "-"
    if role == "A":
        return "AP (forwarding)"
    return "SOURCE" if tx_mean > rx_mean else "viewer/relay"


def level(p):
    return "HIGH" if p < P_HIGH else ("LOW" if p < P_LOW else "NONE")


def analyse(run, lag_ms: int = 0, top: int = 6):
    run = pathlib.Path(run)
    b, a, m, meta = load(run)
    md = io.StringIO()
    md.write(f"# E2 run `{run.name}`\n\n")
    md.write(f"- mode: **{meta.get('mode', '?')}**, channel: {meta.get('channel', 'hop')}, "
             f"bin: {meta.get('bin_ms', '?')} ms, lines: {meta.get('lines', '?')}\n")
    if meta.get("note"):
        md.write(f"- note: {meta['note']}\n")
    if b.empty:
        md.write("\nNo traffic bins recorded.\n")
        (run / "RESULTS.md").write_text(md.getvalue())
        print(md.getvalue())
        return

    dur_s = max((b.t_ms.max() - b.t_ms.min()) / 1000.0, 1e-3)
    # In survey (hop) mode each channel is observed only ~1/13 of the time, so rates
    # are normalised by the on-channel observation time, not the wall-clock duration.
    obs_s = dur_s / 13.0 if meta.get("mode") == "survey" else dur_s
    dev = device_table(b, obs_s)
    dev.to_csv(run / "devices.csv")
    if not a.empty:
        a.to_csv(run / "aps.csv", index=False)

    md.write(f"- duration: {dur_s:.1f} s, devices seen: {len(dev)}, APs: {len(a)}\n")
    if meta.get("mode") == "survey":
        md.write("- survey rates are estimated per on-channel time (duration / 13); "
                 "use `record` on one channel for accurate rates\n")
    md.write("\n")
    md.write("## Busiest transmitters\n\n| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |\n|---|---|---|---|---|---|---|---|\n")
    for mac, r in dev.head(12).iterrows():
        md.write(f"| `{mac}` | {r.vendor} | {r.role} | {r.ch} | {r.tx_kBps:.2f} | {r.rx_kBps:.2f} | "
                 f"{r.tx_frames} | {r.rssi_med:.0f} |\n")

    slots = slots_from_markers(m) if not m.empty else []
    if meta.get("mode") == "challenge" and slots:
        s = np.array([x[1] for x in slots], dtype=float)
        rng = np.random.default_rng(12345)
        res = []
        for mac, r in dev.iterrows():
            if r.tx_frames < MIN_FRAMES:
                continue
            x = slot_rates(b, mac, slots, lag_ms, "tx_b")
            xr = slot_rates(b, mac, slots, lag_ms, "rx_b")
            S, p = perm_test(x, s, rng)
            on, off = x[s == 1].mean(), x[s == 0].mean()
            res.append(dict(mac=mac, vendor=r.vendor, role_frames=r.role,
                            S=S, p=p, level=level(p),
                            tx_on_kBps=on / 1000, tx_off_kBps=off / 1000,
                            ratio_on_off=(on / off) if off > 0 else np.inf,
                            direction=_direction(r.role, level(p), x.mean(), xr.mean())))
        res = pd.DataFrame(res).sort_values("p") if res else pd.DataFrame()
        res.to_csv(run / "results.csv", index=False)

        md.write(f"\n## Challenge (K = {len(slots)}, T = {meta.get('t_s')} s, lag = {lag_ms} ms, "
                 f"{N_PERM} permutations)\n\n")
        md.write(f"Stimulus: `{''.join(str(int(v)) for v in s)}`"
                 f"{'  ·  ground truth: `' + meta['target'] + '`' if meta.get('target') else ''}\n\n")
        md.write("| MAC | vendor | S (corr) | p | level | tx ON kB/s | tx OFF kB/s | ON/OFF | direction |\n"
                 "|---|---|---|---|---|---|---|---|---|\n")
        for _, r in res.iterrows():
            md.write(f"| `{r.mac}` | {r.vendor} | {r.S:+.3f} | {r.p:.4f} | **{r.level}** | "
                     f"{r.tx_on_kBps:.2f} | {r.tx_off_kBps:.2f} | {r.ratio_on_off:.2f} | {r.direction} |\n")
        md.write("\n*Measured data from this run. Levels use the pre-registered lag; "
                 "lag.png is exploratory only.*\n")

        # ---- validity diagnostics (do not change the level, they flag runs to repeat) ----
        md.write("\n## Per-slot uplink (kB/s) and validity checks\n\n")
        md.write("| MAC | " + " | ".join(f"{k}{'*' if s[k] else ''}" for k in range(len(s)))
                 + " | trend ρ | dropouts |\n|---|" + "---|" * len(s) + "---|---|\n")
        warn = []
        for _, r in res.head(4).iterrows():
            x = slot_rates(b, r.mac, slots, lag_ms, "tx_b") / 1000
            rho = _spearman(np.arange(len(x)), x)
            med = np.median(x)
            drops = int((x < 0.2 * med).sum()) if med > 1 else 0
            md.write(f"| `{r.mac}` | " + " | ".join(f"{v:.0f}" for v in x) + f" | {rho:+.2f} | {drops} |\n")
            if med > 5 and abs(rho) > 0.6:
                warn.append(f"`{r.mac}`: strong time trend (ρ = {rho:+.2f}). The stream was ramping up or down, "
                            "so the slots are not exchangeable → repeat with a longer warm-up.")
            if drops:
                warn.append(f"`{r.mac}`: {drops} slot(s) below 20 % of the median → stream dropout or "
                            "band/channel switch during the run.")
        md.write("\n`*` = MOVE/ON slot.\n")
        if warn:
            md.write("\n**Validity warnings:**\n\n" + "".join(f"- {w}\n" for w in warn))

        _plots(run, b, dev, slots, res, top)
    elif meta.get("mode") in ("record", "challenge"):
        _plots(run, b, dev, slots, None, top)

    (run / "RESULTS.md").write_text(md.getvalue())
    print(md.getvalue())


def _plots(run, b, dev, slots, res, top):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        return
    macs = list(res.mac.head(top)) if res is not None and len(res) else list(dev.head(top).index)
    if not macs:
        return
    t0 = b.t_ms.min()
    fig, axes = plt.subplots(len(macs), 1, figsize=(10, 1.8 * len(macs)), sharex=True, squeeze=False)
    for ax, mac in zip(axes[:, 0], macs):
        d = b[b.mac == mac].copy()
        d["t"] = (d.t_ms - t0) / 1000.0
        d["w"] = (d.t // 0.5) * 0.5                       # 0.5 s resampling for readability
        r = d.groupby("w").tx_b.sum() / 0.5 / 1000
        ax.plot(r.index, r.values, lw=1.0, color="#1f5fa8")
        for _, s_, ts, te in slots:
            if s_:
                ax.axvspan((ts - t0) / 1000, (te - t0) / 1000, color="#e8a33d", alpha=0.25, lw=0)
        lab = mac
        if res is not None and len(res):
            rr = res[res.mac == mac].iloc[0]
            lab += f"  p={rr.p:.3f} {rr.level}"
        ax.set_ylabel("kB/s", fontsize=8)
        ax.set_title(lab, fontsize=9, loc="left")
        ax.tick_params(labelsize=8)
        ax.spines[["top", "right"]].set_visible(False)
    axes[-1, 0].set_xlabel("time, s   (orange = MOVE / ON slots)")
    fig.tight_layout()
    fig.savefig(run / "traffic.png", dpi=130)
    plt.close(fig)

    if res is not None and len(res) and slots:
        s = np.array([x[1] for x in slots], dtype=float)
        lags = np.arange(0, 3001, 250)
        fig, ax = plt.subplots(figsize=(6, 3))
        for mac in macs[:4]:
            cs = [corr(slot_rates(b, mac, slots, int(l)), s) for l in lags]
            ax.plot(lags / 1000, cs, marker="o", ms=3, lw=1, label=mac[-8:])
        ax.set_xlabel("assumed response lag, s")
        ax.set_ylabel("corr(tx rate, stimulus)")
        ax.set_title("Exploratory lag sweep (do not use to pick p)", fontsize=9)
        ax.legend(fontsize=7)
        ax.spines[["top", "right"]].set_visible(False)
        fig.tight_layout()
        fig.savefig(run / "lag.png", dpi=130)
        plt.close(fig)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    analyse(sys.argv[1], lag_ms=int(sys.argv[2]) if len(sys.argv) > 2 else 0)
