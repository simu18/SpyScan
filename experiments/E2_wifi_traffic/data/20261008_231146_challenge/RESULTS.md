# E2 run `20261008_231146_challenge`

- mode: **challenge**, channel: 10, bin: 100 ms, lines: 3211
- note: A9 cover #1
- duration: 82.7 s, devices seen: 15, APs: 16

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `46:88:44:6e:4f:e4` | (random) | S | 10 | 34.45 | 0.03 | 6051 | -40 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.94 | 0.49 | 2790 | -17 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 0.52 | 35.84 | 115 | -51 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.45 | 0.00 | 1328 | -21 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.10 | 0.06 | 22 | -80 |
| `a6:4c:bd:b6:c7:99` | (random) | S | 10 | 0.05 | 0.08 | 113 | -90 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.04 | 0.00 | 27 | -86 |
| `00:08:22:24:d6:fb` |  | S | 10 | 0.01 | 0.00 | 6 | -77 |
| `38:d5:7a:f4:5a:37` |  | S | 10 | 0.00 | 0.00 | 9 | -89 |
| `44:df:65:f4:66:07` |  | A | 1 | 0.00 | 0.00 | 1 | -88 |
| `e2:5e:2c:92:7b:df` | (random) | S | 10 | 0.00 | 0.00 | 6 | -90 |
| `b0:a7:b9:23:82:ee` |  | A | 10 | 0.00 | 0.00 | 1 | -87 |

## Challenge (K = 12, T = 5.0 s, lag = 0 ms, 10000 permutations, two-sided)

Stimulus: `011000010111`  ·  ground truth: `46:88:44:6e:4f:e4`

| MAC | vendor | S (corr) | p | level | tx ON kB/s | tx OFF kB/s | ON/OFF | direction |
|---|---|---|---|---|---|---|---|---|
| `b2:db:ca:e6:29:a6` | (random) | -0.529 | 0.0818 | **NONE** | 0.43 | 0.53 | 0.80 | - |
| `a6:4c:bd:b6:c7:99` | (random) | +0.390 | 0.2235 | **NONE** | 0.06 | 0.02 | 2.64 | - |
| `5c:62:8b:66:67:d2` |  | +0.303 | 0.4225 | **NONE** | 0.86 | 0.32 | 2.73 | - |
| `72:3b:01:24:b7:c8` | (random) | +0.241 | 0.4594 | **NONE** | 1.02 | 0.96 | 1.06 | - |
| `46:88:44:6e:4f:e4` | (random) | -0.142 | 0.7010 | **NONE** | 32.93 | 34.11 | 0.97 | - |

*Measured data from this run. Levels use the pre-registered lag; lag.png is exploratory only.*

## Per-slot uplink (kB/s) and validity checks

| MAC | 0 | 1* | 2* | 3 | 4 | 5 | 6 | 7* | 8 | 9* | 10* | 11* | trend ρ | dropouts |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `b2:db:ca:e6:29:a6` | 0 | 0 | 0 | 1 | 1 | 1 | 1 | 0 | 0 | 1 | 0 | 0 | -0.16 | 0 |
| `a6:4c:bd:b6:c7:99` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | -0.47 | 0 |
| `5c:62:8b:66:67:d2` | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 3 | 1 | 0 | +0.32 | 0 |
| `72:3b:01:24:b7:c8` | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | -0.68 | 0 |

`*` = MOVE/ON slot.
