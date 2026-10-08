# E2 run `20261009_011354_challenge`

- mode: **challenge**, channel: 10, bin: 100 ms, lines: 3042
- duration: 82.6 s, devices seen: 15, APs: 15

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` |  | S | 10 | 195.86 | 7.68 | 14097 | -46 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 164.50 | 209.55 | 17461 | -45 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 13.37 | 156.80 | 8649 | -40 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.30 | 0.00 | 887 | -41 |
| `c2:a5:dd:1b:60:3b` | (random) | A | 1 | 0.09 | 0.00 | 17 | -62 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.05 | 0.00 | 26 | -84 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.02 | 0.00 | 21 | -81 |
| `46:88:44:6e:4f:e4` | (random) | S | 10 | 0.01 | 0.01 | 12 | -38 |
| `f2:51:4e:f8:1a:7f` | (random) | S | 10 | 0.01 | 0.00 | 25 | -90 |
| `64:d1:54:63:44:f4` |  | A | 10 | 0.01 | 0.00 | 6 | -88 |
| `5a:62:f6:96:2d:4c` | (random) | S | 1 | 0.00 | 0.09 | 8 | -76 |
| `b0:a7:b9:23:82:ee` |  | A | 10 | 0.00 | 0.01 | 2 | -89 |

## Passive streaming signature (no stimulus needed)

| MAC | up kB/s | down kB/s | up ratio | duty >10 kB/s | CV (1 s) | STREAMER |
|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 196.3 | 7.7 | 0.96 | 1.00 | 0.51 | - |
| `5c:62:8b:66:67:d2` | 164.6 | 211.1 | 0.44 | 1.00 | 0.48 | - |
| `72:3b:01:24:b7:c8` | 13.4 | 158.0 | 0.08 | 0.55 | 0.60 | - |

Rule (provisional, not yet validated): up ≥ 10 kB/s, duty ≥ 0.9, CV ≤ 0.5, up ratio ≥ 0.9, duration ≥ 30 s.

## Challenge (K = 12, T = 5.0 s, lag = 0 ms, 10000 permutations, two-sided)

Stimulus: `011101110000`

| MAC | vendor | S (corr) | p | level | tx ON kB/s | tx OFF kB/s | ON/OFF | direction |
|---|---|---|---|---|---|---|---|---|
| `72:3b:01:24:b7:c8` | (random) | +0.321 | 0.2621 | **NONE** | 15.79 | 13.05 | 1.21 | - |
| `cc:b8:5e:ad:5c:4a` |  | +0.288 | 0.3475 | **NONE** | 202.59 | 187.92 | 1.08 | - |
| `b2:db:ca:e6:29:a6` | (random) | +0.227 | 0.4703 | **NONE** | 0.30 | 0.26 | 1.13 | - |
| `5c:62:8b:66:67:d2` |  | -0.001 | 0.9950 | **NONE** | 166.65 | 166.68 | 1.00 | - |

*Measured data from this run. Levels use the pre-registered lag; lag.png is exploratory only.*

## Per-slot uplink (kB/s) and validity checks

| MAC | 0 | 1* | 2* | 3* | 4 | 5* | 6* | 7* | 8 | 9 | 10 | 11 | trend ρ | dropouts |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `72:3b:01:24:b7:c8` | 18 | 19 | 12 | 11 | 11 | 11 | 24 | 17 | 10 | 16 | 11 | 11 | -0.38 | 0 |
| `cc:b8:5e:ad:5c:4a` | 195 | 227 | 240 | 164 | 188 | 206 | 165 | 213 | 226 | 172 | 175 | 172 | -0.36 | 0 |
| `b2:db:ca:e6:29:a6` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | -0.10 | 0 |
| `5c:62:8b:66:67:d2` | 192 | 174 | 186 | 127 | 165 | 173 | 172 | 168 | 178 | 139 | 170 | 157 | -0.50 | 0 |

`*` = MOVE/ON slot.
