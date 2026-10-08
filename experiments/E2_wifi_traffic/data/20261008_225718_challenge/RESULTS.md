# E2 run `20261008_225718_challenge`

- mode: **challenge**, channel: 10, bin: 100 ms, lines: 3433
- duration: 82.4 s, devices seen: 14, APs: 11

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `46:88:44:6e:4f:e4` | (random) | S | 10 | 50.05 | 0.09 | 6227 | -28 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 24.77 | 51.73 | 2681 | -46 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 1.01 | 21.19 | 2962 | -21 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.67 | 3.49 | 1458 | -28 |
| `a6:4c:bd:b6:c7:99` | (random) | S | 10 | 0.35 | 0.00 | 290 | -85 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.02 | 0.35 | 19 | -74 |
| `38:d5:7a:f4:5a:37` |  | S | 10 | 0.01 | 0.00 | 22 | -90 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.01 | 0.00 | 4 | -84 |
| `64:d1:54:63:44:f4` |  | A | 10 | 0.00 | 0.00 | 3 | -88 |
| `8a:ce:16:78:6f:dc` | (random) | A | 10 | 0.00 | 0.01 | 2 | -92 |
| `00:08:22:24:d6:fb` |  | S | 10 | 0.00 | 0.00 | 2 | -82 |
| `e2:5e:2c:92:7b:df` | (random) | S | 10 | 0.00 | 0.00 | 2 | -90 |

## Challenge (K = 12, T = 5.0 s, lag = 0 ms, 10000 permutations)

Stimulus: `111001001100`

| MAC | vendor | S (corr) | p | level | tx ON kB/s | tx OFF kB/s | ON/OFF | direction |
|---|---|---|---|---|---|---|---|---|
| `72:3b:01:24:b7:c8` | (random) | +0.264 | 0.1905 | **NONE** | 1.08 | 1.01 | 1.07 | - |
| `b2:db:ca:e6:29:a6` | (random) | +0.157 | 0.5000 | **NONE** | 0.58 | 0.47 | 1.24 | - |
| `a6:4c:bd:b6:c7:99` | (random) | -0.261 | 0.7751 | **NONE** | 0.27 | 0.62 | 0.44 | - |
| `5c:62:8b:66:67:d2` |  | -0.338 | 0.8551 | **NONE** | 19.86 | 33.73 | 0.59 | - |
| `46:88:44:6e:4f:e4` | (random) | -0.606 | 0.9805 | **NONE** | 43.51 | 59.23 | 0.73 | - |

*Measured data from this run. Levels use the pre-registered lag; lag.png is exploratory only.*

## Per-slot uplink (kB/s) and validity checks

| MAC | 0* | 1* | 2* | 3 | 4 | 5* | 6 | 7 | 8* | 9* | 10 | 11 | trend ρ | dropouts |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `72:3b:01:24:b7:c8` | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | -0.12 | 0 |
| `b2:db:ca:e6:29:a6` | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 2 | 0 | 0 | +0.50 | 0 |
| `a6:4c:bd:b6:c7:99` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 1 | 2 | 1 | +0.79 | 0 |
| `5c:62:8b:66:67:d2` | 4 | 7 | 15 | 45 | 49 | 47 | 42 | 52 | 45 | 1 | 1 | 13 | -0.13 | 3 |

`*` = MOVE/ON slot.

**Validity warnings:**

- `5c:62:8b:66:67:d2`: 3 slot(s) below 20 % of the median → stream dropout or band/channel switch during the run.
