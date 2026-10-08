# E2 run `20261008_225956_challenge`

- mode: **challenge**, channel: 10, bin: 100 ms, lines: 3248
- duration: 82.4 s, devices seen: 16, APs: 12

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `46:88:44:6e:4f:e4` | (random) | S | 10 | 62.75 | 0.12 | 7103 | -35 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 32.82 | 70.42 | 3072 | -51 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 6.74 | 13.50 | 2698 | -24 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.94 | 19.19 | 2760 | -21 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.18 | 0.03 | 59 | -75 |
| `b0:a7:b9:23:82:ee` |  | A | 10 | 0.03 | 0.00 | 5 | -86 |
| `a6:4c:bd:b6:c7:99` | (random) | S | 10 | 0.03 | 0.00 | 71 | -87 |
| `d2:cf:53:d5:b8:26` | (random) | S | 10 | 0.00 | 0.00 | 5 | -91 |
| `64:d1:54:63:44:f4` |  | A | 10 | 0.00 | 0.00 | 1 | -91 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.00 | 0.00 | 1 | -86 |
| `f4:28:9d:ce:e6:09` |  | S | 1 | 0.00 | 0.00 | 2 | -85 |
| `08:9d:f4:81:f5:38` |  | S | 1 | 0.00 | 0.00 | 1 | -85 |

## Challenge (K = 12, T = 5.0 s, lag = 0 ms, 10000 permutations)

Stimulus: `101011001001`

| MAC | vendor | S (corr) | p | level | tx ON kB/s | tx OFF kB/s | ON/OFF | direction |
|---|---|---|---|---|---|---|---|---|
| `46:88:44:6e:4f:e4` | (random) | +0.219 | 0.2391 | **NONE** | 57.04 | 55.52 | 1.03 | - |
| `78:8c:b5:f5:e7:26` |  | +0.294 | 0.2573 | **NONE** | 0.34 | 0.08 | 4.15 | - |
| `72:3b:01:24:b7:c8` | (random) | +0.147 | 0.3300 | **NONE** | 0.97 | 0.91 | 1.07 | - |
| `5c:62:8b:66:67:d2` |  | +0.147 | 0.3363 | **NONE** | 33.80 | 23.69 | 1.43 | - |
| `a6:4c:bd:b6:c7:99` | (random) | +0.237 | 0.4938 | **NONE** | 0.02 | 0.00 | 4.60 | - |
| `b2:db:ca:e6:29:a6` | (random) | -0.193 | 0.6715 | **NONE** | 5.13 | 12.83 | 0.40 | - |

*Measured data from this run. Levels use the pre-registered lag; lag.png is exploratory only.*

## Per-slot uplink (kB/s) and validity checks

| MAC | 0* | 1 | 2* | 3 | 4* | 5* | 6 | 7 | 8* | 9 | 10 | 11* | trend ρ | dropouts |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `46:88:44:6e:4f:e4` | 62 | 58 | 53 | 54 | 54 | 63 | 54 | 55 | 53 | 53 | 59 | 58 | -0.01 | 0 |
| `78:8c:b5:f5:e7:26` | 2 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | -0.31 | 0 |
| `72:3b:01:24:b7:c8` | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | -0.36 | 0 |
| `5c:62:8b:66:67:d2` | 62 | 32 | 9 | 4 | 0 | 0 | 0 | 11 | 106 | 84 | 11 | 27 | +0.20 | 3 |

`*` = MOVE/ON slot.

**Validity warnings:**

- `5c:62:8b:66:67:d2`: 3 slot(s) below 20 % of the median → stream dropout or band/channel switch during the run.
