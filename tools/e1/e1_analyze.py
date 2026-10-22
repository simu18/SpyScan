#!/usr/bin/env python3
"""SpyScan E1 — optical bring-up / retroreflection log analyzer.

Parses the serial output of firmware/bringup (DIFF_AUTO sweep) and summarises,
per exposure, whether the AXIS LED produced a STABLE bright return that the
OFF-AXIS LED did not — the signature of a retroreflecting lens (vs a glint).

D line format (one per diff cycle):
  D,t_ms,led(A|O),aec,mean,sigma,thr,n_hot,peak,peak_x,peak_y,snr
M line marks a segment:
  M,t_ms,seg,sweep=..,aec=..,led=..

Usage:
  python tools/e1/e1_analyze.py run.log          # analyse a saved log
  idf.py monitor | tee run.log                   # capture while it runs
  pbpaste | python tools/e1/e1_analyze.py -      # analyse pasted text (macOS)

Verdict per exposure (AXIS):
  LENS-LIKE   peak is strong AND its location is stable across cycles,
              AND OFF-AXIS at the same exposure is weaker / not co-located.
  glint/none  strong but jumpy location, or AXIS~OFF (not direction-selective).
"""
import sys, statistics as st
from collections import defaultdict

SAT = 250          # peak >= SAT counts as saturated (8-bit luma headroom gone)
STABLE_PX = 25     # peak-location spread (px) below this = "stable"
STRONG_PEAK = 30   # median peak above this = a real bright return
MIN_CYCLES = 3

def parse(text):
    rows = []
    for ln in text.splitlines():
        ln = ln.strip()
        if not ln.startswith("D,"):
            continue
        f = ln.split(",")
        if len(f) < 12:
            continue
        try:
            rows.append(dict(t=int(f[1]), led=f[2], aec=int(f[3]),
                             mean=float(f[4]), sigma=float(f[5]), thr=float(f[6]),
                             n_hot=int(f[7]), peak=float(f[8]),
                             px=int(f[9]), py=int(f[10]), snr=float(f[11])))
        except ValueError:
            continue
    return rows

def spread(vals):
    return (max(vals) - min(vals)) if len(vals) > 1 else 0

def summarise_group(rows):
    """One (aec, led) group -> dict of robust stats."""
    peaks = [r["peak"] for r in rows]
    snrs  = [r["snr"]  for r in rows]
    nhot  = [r["n_hot"] for r in rows]
    pxs   = [r["px"] for r in rows]
    pys   = [r["py"] for r in rows]
    # location stability from the cycles whose peak is in the top half
    med_peak = st.median(peaks)
    strong = [r for r in rows if r["peak"] >= med_peak]
    sx = spread([r["px"] for r in strong]); sy = spread([r["py"] for r in strong])
    return dict(n=len(rows), med_peak=med_peak, med_snr=st.median(snrs),
                med_nhot=int(st.median(nhot)), sat=sum(p >= SAT for p in peaks),
                loc=(int(st.median(pxs)), int(st.median(pys))),
                loc_spread=max(sx, sy))

def main():
    src = sys.argv[1] if len(sys.argv) > 1 else "-"
    text = sys.stdin.read() if src == "-" else open(src, encoding="utf-8", errors="replace").read()
    rows = parse(text)
    if not rows:
        sys.exit("no D lines found")

    groups = defaultdict(list)
    for r in rows:
        groups[(r["aec"], r["led"])].append(r)

    aecs = sorted({a for a, _ in groups})
    print(f"\nparsed {len(rows)} diff cycles over {len(aecs)} exposures\n")
    print(f"{'aec':>5} {'led':>3} {'n':>2} {'med_peak':>8} {'sat':>3} {'med_snr':>7} "
          f"{'med_nhot':>8} {'peak_xy':>9} {'loc_spread':>10}")
    print("-" * 66)
    best = None
    for aec in aecs:
        for led in ("A", "O"):
            g = groups.get((aec, led))
            if not g:
                continue
            s = summarise_group(g)
            print(f"{aec:>5} {led:>3} {s['n']:>2} {s['med_peak']:>8.0f} {s['sat']:>3} "
                  f"{s['med_snr']:>7.1f} {s['med_nhot']:>8} "
                  f"{str(s['loc']):>9} {s['loc_spread']:>10}")
        print()

    print("verdict per exposure (AXIS vs OFF-AXIS):")
    print("-" * 66)
    for aec in aecs:
        a = groups.get((aec, "A")); o = groups.get((aec, "O"))
        if not a or len(a) < MIN_CYCLES:
            continue
        sa = summarise_group(a)
        so = summarise_group(o) if o else None
        stable = sa["loc_spread"] <= STABLE_PX
        strong = sa["med_peak"] >= STRONG_PEAK
        # direction-selective: AXIS stable spot NOT reproduced by OFF-AXIS nearby
        if so:
            dx = abs(sa["loc"][0] - so["loc"][0]); dy = abs(sa["loc"][1] - so["loc"][1])
            off_elsewhere = (dx > STABLE_PX or dy > STABLE_PX) or (so["loc_spread"] > STABLE_PX) \
                            or (so["med_peak"] < 0.6 * sa["med_peak"])
        else:
            off_elsewhere = True
        if stable and strong and off_elsewhere:
            verdict = "LENS-LIKE  (stable AXIS return, OFF-AXIS weaker/elsewhere)"
            score = sa["med_peak"] * (1 if sa["loc_spread"] == 0 else 30.0 / (sa["loc_spread"] + 5))
            if best is None or score > best[1]:
                best = (aec, score, sa["loc"])
        elif strong and not stable:
            verdict = "glint/noise (strong but location jumps)"
        else:
            verdict = "no clear return"
        print(f"  aec {aec:>4}: {verdict}")
        print(f"            AXIS peak~{sa['med_peak']:.0f} at {sa['loc']} spread {sa['loc_spread']}px"
              + (f" | OFF peak~{so['med_peak']:.0f} at {so['loc']} spread {so['loc_spread']}px" if so else ""))

    print()
    if best:
        print(f">> strongest lens-like candidate: exposure {best[0]}, "
              f"stable spot at pixel {best[2]}.")
        print(f"   Point-check: is there a camera lens at that spot in the camera view?")
    else:
        print(">> no lens-like retroreflection in this run "
              "(no stable AXIS-only bright return). Try a known lens, dim room, ~35 cm.")
    print()

if __name__ == "__main__":
    main()
