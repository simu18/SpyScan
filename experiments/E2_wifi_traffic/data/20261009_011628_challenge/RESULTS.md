# E2 run `20261009_011628_challenge`

- mode: **challenge**, channel: 10, bin: 100 ms, lines: 1523
- duration: 82.2 s, devices seen: 14, APs: 15

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` |  | S | 10 | 0.38 | 0.22 | 76 | -47 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.37 | 0.00 | 1077 | -41 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 0.33 | 0.88 | 107 | -48 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.12 | 0.09 | 131 | -41 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.04 | 0.01 | 30 | -79 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.04 | 0.00 | 18 | -84 |
| `f2:51:4e:f8:1a:7f` | (random) | S | 10 | 0.02 | 0.00 | 44 | -88 |
| `64:d1:54:63:44:f4` |  | A | 10 | 0.01 | 0.00 | 12 | -87 |
| `46:88:44:6e:4f:e4` | (random) | S | 10 | 0.01 | 0.01 | 13 | -37 |
| `a6:4c:bd:b6:c7:99` | (random) | S | 10 | 0.01 | 0.00 | 35 | -91 |
| `b0:a7:b9:23:82:ee` |  | A | 10 | 0.01 | 0.02 | 6 | -88 |
| `00:08:22:24:d6:fb` |  | S | 10 | 0.00 | 0.00 | 2 | -86 |

## Challenge (K = 12, T = 5.0 s, lag = 0 ms, 10000 permutations, two-sided)

Stimulus: `111000101010`

| MAC | vendor | S (corr) | p | level | tx ON kB/s | tx OFF kB/s | ON/OFF | direction |
|---|---|---|---|---|---|---|---|---|
| `5c:62:8b:66:67:d2` |  | +0.435 | 0.1340 | **NONE** | 0.78 | 0.09 | 8.91 | - |
| `b2:db:ca:e6:29:a6` | (random) | +0.248 | 0.4251 | **NONE** | 0.40 | 0.37 | 1.08 | - |
| `72:3b:01:24:b7:c8` | (random) | +0.345 | 0.4470 | **NONE** | 0.21 | 0.04 | 5.42 | - |
| `cc:b8:5e:ad:5c:4a` |  | +0.302 | 0.4765 | **NONE** | 0.97 | 0.04 | 22.87 | - |

*Measured data from this run. Levels use the pre-registered lag; lag.png is exploratory only.*

## Per-slot uplink (kB/s) and validity checks

| MAC | 0* | 1* | 2* | 3 | 4 | 5 | 6* | 7 | 8* | 9 | 10* | 11 | trend ρ | dropouts |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `5c:62:8b:66:67:d2` | 1 | 0 | 0 | 0 | 0 | 0 | 3 | 0 | 0 | 0 | 0 | 0 | -0.66 | 0 |
| `b2:db:ca:e6:29:a6` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | -0.81 | 0 |
| `72:3b:01:24:b7:c8` | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | -0.04 | 0 |
| `cc:b8:5e:ad:5c:4a` | 0 | 0 | 0 | 0 | 0 | 0 | 6 | 0 | 0 | 0 | 0 | 0 | -0.30 | 0 |

`*` = MOVE/ON slot.
