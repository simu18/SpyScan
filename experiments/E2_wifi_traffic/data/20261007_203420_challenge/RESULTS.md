# E2 run `20261007_203420_challenge`

- mode: **challenge**, channel: 10, bin: 100 ms, lines: 1850
- duration: 82.4 s, devices seen: 11, APs: 11

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.46 | 0.02 | 1344 | -54 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.31 | 0.02 | 902 | -31 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 0.05 | 0.77 | 10 | -56 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.03 | 0.00 | 15 | -84 |
| `98:5f:41:bf:7c:ff` |  | S | 1 | 0.02 | 0.00 | 11 | -69 |
| `8e:26:78:2e:9e:0c` | (random) | S | 10 | 0.01 | 0.00 | 23 | -74 |
| `f4:28:9d:ce:e6:09` |  | S | 1 | 0.00 | 0.00 | 4 | -87 |
| `0c:ef:15:aa:11:03` |  | A | 10 | 0.00 | 0.01 | 0 | nan |
| `52:4b:9a:a2:db:8e` | (random) | S | 10 | 0.00 | 0.00 | 0 | nan |
| `c4:16:88:9a:06:5c` |  | A | 1 | 0.00 | 0.00 | 0 | nan |
| `cc:d8:43:ca:9f:eb` |  | A | 1 | 0.00 | 0.02 | 0 | nan |

## Challenge (K = 12, T = 5.0 s, lag = 0 ms, 10000 permutations)

Stimulus: `111100100010`  ·  ground truth: `b2:db:ca:e6:29:a6`

| MAC | vendor | S (corr) | p | level | tx ON kB/s | tx OFF kB/s | ON/OFF | direction |
|---|---|---|---|---|---|---|---|---|
| `72:3b:01:24:b7:c8` | (random) | +0.495 | 0.0523 | **NONE** | 0.44 | 0.37 | 1.21 | - |
| `b2:db:ca:e6:29:a6` | (random) | +0.067 | 0.4214 | **NONE** | 0.31 | 0.30 | 1.01 | - |

*Measured data from this run. Levels use the pre-registered lag; lag.png is exploratory only.*

## Per-slot uplink (kB/s) and validity checks

| MAC | 0* | 1* | 2* | 3* | 4 | 5 | 6* | 7 | 8 | 9 | 10* | 11 | trend ρ | dropouts |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `72:3b:01:24:b7:c8` | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | -0.27 | 0 |
| `b2:db:ca:e6:29:a6` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | -0.10 | 0 |

`*` = MOVE/ON slot.
