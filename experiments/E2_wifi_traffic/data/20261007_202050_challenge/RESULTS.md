# E2 run `20261007_202050_challenge`

- mode: **challenge**, channel: 10, bin: 100 ms, lines: 1819
- duration: 72.5 s, devices seen: 6, APs: 10

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 48.87 | 5.94 | 11667 | -30 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 5.94 | 51.19 | 992 | -52 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 2.32 | 0.00 | 1612 | -42 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.01 | 0.00 | 9 | -82 |
| `f4:28:9d:ce:e6:09` |  | S | 1 | 0.00 | 0.00 | 3 | -88 |
| `c4:16:88:9a:06:5c` |  | A | 1 | 0.00 | 0.00 | 0 | nan |

## Challenge (K = 12, T = 5.0 s, lag = 0 ms, 10000 permutations)

Stimulus: `011101011000`  ·  ground truth: `b2:db:ca:e6:29:a6`

| MAC | vendor | S (corr) | p | level | tx ON kB/s | tx OFF kB/s | ON/OFF | direction |
|---|---|---|---|---|---|---|---|---|
| `5c:62:8b:66:67:d2` |  | -0.252 | 0.5224 | **NONE** | 1.34 | 8.11 | 0.16 | - |
| `b2:db:ca:e6:29:a6` | (random) | -0.442 | 0.9437 | **NONE** | 42.04 | 68.11 | 0.62 | - |
| `72:3b:01:24:b7:c8` | (random) | -0.303 | 0.9672 | **NONE** | 0.43 | 4.92 | 0.09 | - |

*Measured data from this run. Levels use the pre-registered lag; lag.png is exploratory only.*
