# E2 run `20261007_191926_challenge`

- mode: **challenge**, channel: 10, bin: 100 ms, lines: 1055
- duration: 72.5 s, devices seen: 10, APs: 13

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.23 | 0.00 | 604 | -27 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 0.18 | 0.32 | 101 | -46 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.08 | 0.15 | 215 | -37 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.08 | 0.00 | 30 | -84 |
| `38:d5:7a:f4:5a:37` |  | S | 10 | 0.05 | 0.00 | 117 | -94 |
| `d2:cf:53:d5:b8:26` | (random) | S | 10 | 0.00 | 0.00 | 2 | -93 |
| `f4:28:9d:ce:e6:09` |  | S | 1 | 0.00 | 0.00 | 1 | -90 |
| `46:fb:cf:a9:f4:22` | (random) | A | 10 | 0.00 | 0.05 | 0 | nan |
| `52:f3:8a:4b:ff:ac` | (random) | S | 10 | 0.00 | 0.01 | 0 | nan |
| `c4:16:88:9a:06:5c` |  | A | 1 | 0.00 | 0.00 | 0 | -87 |

## Challenge (K = 12, T = 5.0 s, lag = 0 ms, 10000 permutations)

Stimulus: `101100110001`

| MAC | vendor | S (corr) | p | level | tx ON kB/s | tx OFF kB/s | ON/OFF | direction |
|---|---|---|---|---|---|---|---|---|
| `72:3b:01:24:b7:c8` | (random) | +0.233 | 0.2913 | **NONE** | 0.12 | 0.05 | 2.36 | - |
| `5c:62:8b:66:67:d2` |  | +0.224 | 0.3144 | **NONE** | 0.27 | 0.09 | 2.83 | - |
| `38:d5:7a:f4:5a:37` |  | +0.035 | 0.4545 | **NONE** | 0.06 | 0.05 | 1.09 | - |
| `b2:db:ca:e6:29:a6` | (random) | -0.175 | 0.7169 | **NONE** | 0.22 | 0.25 | 0.88 | - |

*Measured data from this run. Levels use the pre-registered lag; lag.png is exploratory only.*
