# E2 run `20261008_231943_challenge`

- mode: **challenge**, channel: 10, bin: 100 ms, lines: 3401
- note: NULL run
- duration: 82.4 s, devices seen: 12, APs: 12

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `46:88:44:6e:4f:e4` | (random) | S | 10 | 56.01 | 0.06 | 6656 | -25 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 3.72 | 57.35 | 456 | -50 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.93 | 3.66 | 2750 | -18 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.40 | 0.00 | 1164 | -21 |
| `d2:cf:53:d5:b8:26` | (random) | S | 10 | 0.25 | 0.00 | 733 | -90 |
| `a6:4c:bd:b6:c7:99` | (random) | S | 10 | 0.09 | 0.00 | 218 | -87 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.05 | 0.10 | 34 | -76 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.01 | 0.00 | 7 | -86 |
| `3c:f0:11:24:00:90` |  | S | 10 | 0.01 | 0.00 | 31 | -90 |
| `b0:a7:b9:23:82:ee` |  | A | 10 | 0.01 | 0.01 | 1 | -88 |
| `00:08:22:24:d6:fb` |  | S | 10 | 0.00 | 0.00 | 1 | -79 |
| `8a:ce:16:78:6f:dc` | (random) | A | 10 | 0.00 | 0.25 | 0 | -92 |

## Challenge (K = 12, T = 5.0 s, lag = 0 ms, 10000 permutations, two-sided)

Stimulus: `011011101000`  ·  ground truth: `46:88:44:6e:4f:e4`

| MAC | vendor | S (corr) | p | level | tx ON kB/s | tx OFF kB/s | ON/OFF | direction |
|---|---|---|---|---|---|---|---|---|
| `5c:62:8b:66:67:d2` |  | -0.454 | 0.0637 | **NONE** | 0.29 | 8.18 | 0.04 | - |
| `a6:4c:bd:b6:c7:99` | (random) | +0.321 | 0.2971 | **NONE** | 0.10 | 0.06 | 1.69 | - |
| `46:88:44:6e:4f:e4` | (random) | +0.297 | 0.3706 | **NONE** | 60.29 | 56.14 | 1.07 | - |
| `b2:db:ca:e6:29:a6` | (random) | +0.158 | 0.6251 | **NONE** | 0.40 | 0.38 | 1.07 | - |
| `72:3b:01:24:b7:c8` | (random) | -0.044 | 0.9000 | **NONE** | 0.90 | 0.90 | 0.99 | - |
| `d2:cf:53:d5:b8:26` | (random) | +0.000 | 1.0000 | **NONE** | 0.00 | 0.00 | inf | - |

*Measured data from this run. Levels use the pre-registered lag; lag.png is exploratory only.*

## Per-slot uplink (kB/s) and validity checks

| MAC | 0 | 1* | 2* | 3 | 4* | 5* | 6* | 7 | 8* | 9 | 10 | 11 | trend ρ | dropouts |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `5c:62:8b:66:67:d2` | 0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 | 0 | 7 | 9 | 31 | +0.39 | 0 |
| `a6:4c:bd:b6:c7:99` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | -0.20 | 0 |
| `46:88:44:6e:4f:e4` | 42 | 53 | 70 | 63 | 56 | 62 | 66 | 57 | 55 | 59 | 62 | 54 | +0.06 | 0 |
| `b2:db:ca:e6:29:a6` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 | 0 | -0.06 | 0 |

`*` = MOVE/ON slot.
