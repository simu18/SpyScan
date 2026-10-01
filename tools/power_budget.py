#!/usr/bin/env python3
"""SpyScan V1 power budget (pre-measurement estimate).

Every load current below is an ESTIMATE taken from typical datasheet-class
values and must be replaced by values measured with a USB power meter
(acceptance test AT-10).  Run:  python3 tools/power_budget.py
"""

# ---------------- assumptions (edit after measurement) -----------------
V_BAT_NOM = 3.7      # V, Li-Po nominal
CAP_MAH = 2000       # mAh, rated
USABLE = 0.80        # usable fraction (cut-off ~3.3 V, ageing, temperature)
ETA_BOOST = 0.85     # IP5306-type boost efficiency at ~0.3-0.6 A (estimate)
V_BUS = 5.0          # V, boost output
TARGET_H = 1.5       # NR-P6 target

# 3.3 V rail loads, mA (fed by on-board LDO from 5 V -> I_5V == I_3V3)
LOADS_3V3 = {
    #                       optical  wireless  idle(menu)
    "ESP32-S3 CPU 240MHz":  (60,      50,       30),   # A: dual-core active / light
    "Wi-Fi RX (promisc.)":  (0,       95,       0),    # A: 802.11n RX class current
    "OV2640 active":        (40,      0,        0),    # A: ~125 mW class @ QVGA
    "PSRAM":                (15,      5,        3),    # A
    "ILI9341 logic+BL":     (35,      35,       35),   # A: 4-LED backlight
    "Misc (pull-ups, LED)": (5,       5,        5),
}
# 5 V rail loads (LED ring driven directly from 5 V via 150 ohm resistors)
LED_MA_EACH = 20
LEDS_ON_AXIS = 6
DUTY_OPTICAL = 0.5   # ON/OFF alternation

MODES = ["optical", "wireless", "idle"]


def battery_current(mode_idx: int) -> dict:
    i3v3 = sum(v[mode_idx] for v in LOADS_3V3.values())
    i_led = LEDS_ON_AXIS * LED_MA_EACH * DUTY_OPTICAL if mode_idx == 0 else 0
    i5 = i3v3 + i_led                           # LDO: input current = output current
    p5 = V_BUS * i5 / 1000                      # W at 5 V bus
    i_bat = p5 / ETA_BOOST / V_BAT_NOM * 1000   # mA from battery
    return {"I_3V3": i3v3, "I_LED": i_led, "I_5V": i5, "P_5V_W": p5, "I_BAT": i_bat}


def main() -> None:
    usable_mah = CAP_MAH * USABLE
    print(f"Battery {CAP_MAH} mAh x {USABLE:.0%} usable = {usable_mah:.0f} mAh, "
          f"boost eta = {ETA_BOOST:.0%}\n")
    print(f"{'mode':10s} {'I3V3':>6s} {'ILED':>6s} {'I5V':>6s} {'P5V,W':>7s} {'Ibat':>6s} {'life,h':>7s}")
    rows = {}
    for i, m in enumerate(MODES):
        r = battery_current(i)
        life = usable_mah / r["I_BAT"]
        rows[m] = (r, life)
        print(f"{m:10s} {r['I_3V3']:6.0f} {r['I_LED']:6.0f} {r['I_5V']:6.0f} "
              f"{r['P_5V_W']:7.2f} {r['I_BAT']:6.0f} {life:7.1f}")
    # mixed demo profile: 45 % optical, 45 % wireless, 10 % menu
    mix = 0.45 * rows["optical"][0]["I_BAT"] + 0.45 * rows["wireless"][0]["I_BAT"] \
        + 0.10 * rows["idle"][0]["I_BAT"]
    life_mix = usable_mah / mix
    print(f"\nmixed (45/45/10): I_bat = {mix:.0f} mA -> {life_mix:.1f} h "
          f"(target {TARGET_H} h, margin x{life_mix / TARGET_H:.1f})")
    worst = 1.5 * mix
    print(f"pessimistic (+50 % on all loads): {worst:.0f} mA -> {usable_mah / worst:.1f} h")
    print("\nPeak check: Wi-Fi TX bursts (AP scan) reach ~300-350 mA on 3.3 V for ms;"
          " LDO + boost must supply >= 0.6 A at 5 V -> IP5306 (2.1 A) OK; 470 uF bulk at board 5 V.")


if __name__ == "__main__":
    main()
