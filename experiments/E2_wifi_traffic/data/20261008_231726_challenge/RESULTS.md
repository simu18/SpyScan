# E2 run `20261008_231726_challenge`

- mode: **challenge**, channel: 10, bin: 100 ms, lines: 3348
- note: light on off  #1
- duration: 82.5 s, devices seen: 16, APs: 15

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `46:88:44:6e:4f:e4` | (random) | S | 10 | 33.30 | 0.05 | 5410 | -27 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 13.99 | 34.54 | 2074 | -49 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.89 | 13.94 | 2617 | -17 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.35 | 0.00 | 1040 | -22 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.09 | 0.00 | 45 | -82 |
| `a6:4c:bd:b6:c7:99` | (random) | S | 10 | 0.07 | 0.00 | 177 | -89 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.06 | 0.08 | 35 | -74 |
| `b0:a7:b9:23:82:ee` |  | A | 10 | 0.01 | 0.00 | 1 | -86 |
| `00:08:22:24:d6:fb` |  | S | 10 | 0.01 | 0.00 | 10 | -77 |
| `8a:ce:16:78:6f:dc` | (random) | A | 10 | 0.01 | 0.00 | 5 | -92 |
| `3c:f0:11:24:00:90` |  | S | 10 | 0.00 | 0.00 | 7 | -91 |
| `f4:28:9d:ce:e6:09` |  | S | 1 | 0.00 | 0.00 | 2 | -87 |

## Challenge (K = 12, T = 5.0 s, lag = 0 ms, 10000 permutations, two-sided)

Stimulus: `100101011010`  ·  ground truth: `46:88:44:6e:4f:e4`

| MAC | vendor | S (corr) | p | level | tx ON kB/s | tx OFF kB/s | ON/OFF | direction |
|---|---|---|---|---|---|---|---|---|
| `46:88:44:6e:4f:e4` | (random) | -0.256 | 0.3121 | **NONE** | 29.53 | 32.26 | 0.92 | - |
| `5c:62:8b:66:67:d2` |  | -0.223 | 0.4730 | **NONE** | 7.20 | 10.38 | 0.69 | - |
| `b2:db:ca:e6:29:a6` | (random) | +0.167 | 0.6057 | **NONE** | 0.36 | 0.34 | 1.07 | - |
| `a6:4c:bd:b6:c7:99` | (random) | -0.207 | 0.6182 | **NONE** | 0.05 | 0.07 | 0.79 | - |
| `72:3b:01:24:b7:c8` | (random) | +0.116 | 0.7319 | **NONE** | 0.90 | 0.87 | 1.03 | - |

*Measured data from this run. Levels use the pre-registered lag; lag.png is exploratory only.*

## Per-slot uplink (kB/s) and validity checks

| MAC | 0* | 1 | 2 | 3* | 4 | 5* | 6 | 7* | 8* | 9 | 10* | 11 | trend ρ | dropouts |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `46:88:44:6e:4f:e4` | 27 | 27 | 29 | 39 | 39 | 27 | 29 | 28 | 28 | 28 | 29 | 42 | +0.38 | 0 |
| `5c:62:8b:66:67:d2` | 23 | 19 | 20 | 0 | 2 | 7 | 6 | 6 | 4 | 6 | 4 | 9 | -0.38 | 1 |
| `b2:db:ca:e6:29:a6` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | +0.10 | 0 |
| `a6:4c:bd:b6:c7:99` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | -0.24 | 0 |

`*` = MOVE/ON slot.

**Validity warnings:**

- `5c:62:8b:66:67:d2`: 1 slot(s) below 20 % of the median → stream dropout or band/channel switch during the run.
