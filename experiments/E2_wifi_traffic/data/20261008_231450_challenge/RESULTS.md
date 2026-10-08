# E2 run `20261008_231450_challenge`

- mode: **challenge**, channel: 10, bin: 100 ms, lines: 3346
- note: A9 cover #2
- duration: 82.6 s, devices seen: 17, APs: 15

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `46:88:44:6e:4f:e4` | (random) | S | 10 | 28.90 | 0.06 | 5288 | -28 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 5.46 | 30.28 | 535 | -49 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.91 | 1.09 | 2697 | -17 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.47 | 4.31 | 1222 | -21 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.13 | 0.00 | 42 | -85 |
| `a6:4c:bd:b6:c7:99` | (random) | S | 10 | 0.04 | 0.00 | 116 | -91 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.03 | 0.05 | 22 | -81 |
| `38:d5:7a:f4:5a:37` |  | S | 10 | 0.00 | 0.00 | 12 | -90 |
| `00:08:22:24:d6:fb` |  | S | 10 | 0.00 | 0.00 | 5 | -79 |
| `d2:cf:53:d5:b8:26` | (random) | S | 10 | 0.00 | 0.00 | 3 | -91 |
| `08:9d:f4:81:f5:38` |  | S | 1 | 0.00 | 0.00 | 1 | -92 |
| `e2:5e:2c:92:7b:df` | (random) | S | 10 | 0.00 | 0.00 | 1 | -95 |

## Challenge (K = 12, T = 5.0 s, lag = 0 ms, 10000 permutations, two-sided)

Stimulus: `110100100011`  ·  ground truth: `46:88:44:6e:4f:e4`

| MAC | vendor | S (corr) | p | level | tx ON kB/s | tx OFF kB/s | ON/OFF | direction |
|---|---|---|---|---|---|---|---|---|
| `72:3b:01:24:b7:c8` | (random) | +0.319 | 0.2997 | **NONE** | 0.99 | 0.91 | 1.08 | - |
| `46:88:44:6e:4f:e4` | (random) | -0.274 | 0.3932 | **NONE** | 29.24 | 29.98 | 0.98 | - |
| `5c:62:8b:66:67:d2` |  | -0.069 | 0.5850 | **NONE** | 5.99 | 7.89 | 0.76 | - |
| `a6:4c:bd:b6:c7:99` | (random) | +0.045 | 0.8720 | **NONE** | 0.04 | 0.04 | 1.08 | - |
| `b2:db:ca:e6:29:a6` | (random) | +0.004 | 0.9892 | **NONE** | 0.49 | 0.49 | 1.00 | - |

*Measured data from this run. Levels use the pre-registered lag; lag.png is exploratory only.*

## Per-slot uplink (kB/s) and validity checks

| MAC | 0* | 1* | 2 | 3* | 4 | 5 | 6* | 7 | 8 | 9 | 10* | 11* | trend ρ | dropouts |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `72:3b:01:24:b7:c8` | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | +0.32 | 0 |
| `46:88:44:6e:4f:e4` | 29 | 31 | 29 | 29 | 33 | 29 | 29 | 29 | 32 | 28 | 29 | 29 | -0.20 | 0 |
| `5c:62:8b:66:67:d2` | 2 | 0 | 1 | 0 | 3 | 42 | 33 | 1 | 0 | 1 | 0 | 1 | -0.29 | 0 |
| `a6:4c:bd:b6:c7:99` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | +0.24 | 0 |

`*` = MOVE/ON slot.
