# E2 run `20261007_192348_challenge`

- mode: **challenge**, channel: 10, bin: 100 ms, lines: 2061
- duration: 72.5 s, devices seen: 11, APs: 10

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 8.88 | 5.09 | 2387 | -28 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 5.11 | 10.08 | 1682 | -48 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 1.20 | 0.01 | 2920 | -32 |
| `e6:eb:0e:16:9b:da` | (random) | A | 10 | 0.37 | 0.00 | 17 | -67 |
| `38:d5:7a:f4:5a:37` |  | S | 10 | 0.06 | 0.00 | 139 | -93 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.01 | 0.00 | 6 | -86 |
| `f4:28:9d:ce:e6:09` |  | S | 1 | 0.00 | 0.00 | 4 | -87 |
| `46:fb:cf:a9:f4:22` | (random) | A | 10 | 0.00 | 0.06 | 0 | nan |
| `52:f3:8a:4b:ff:ac` | (random) | S | 10 | 0.00 | 0.00 | 0 | nan |
| `6a:00:a8:6f:a4:98` | (random) | S | 10 | 0.00 | 0.37 | 0 | nan |
| `c4:16:88:9a:06:5c` |  | A | 1 | 0.00 | 0.00 | 0 | nan |

## Challenge (K = 12, T = 5.0 s, lag = 0 ms, 10000 permutations)

Stimulus: `000011101011`

| MAC | vendor | S (corr) | p | level | tx ON kB/s | tx OFF kB/s | ON/OFF | direction |
|---|---|---|---|---|---|---|---|---|
| `b2:db:ca:e6:29:a6` | (random) | +0.434 | 0.1736 | **NONE** | 19.59 | 1.47 | 13.34 | - |
| `72:3b:01:24:b7:c8` | (random) | +0.286 | 0.1859 | **NONE** | 1.23 | 1.06 | 1.16 | - |
| `5c:62:8b:66:67:d2` |  | +0.363 | 0.1956 | **NONE** | 10.13 | 1.95 | 5.18 | - |
| `38:d5:7a:f4:5a:37` |  | -0.566 | 0.9766 | **NONE** | 0.01 | 0.06 | 0.14 | - |

*Measured data from this run. Levels use the pre-registered lag; lag.png is exploratory only.*
