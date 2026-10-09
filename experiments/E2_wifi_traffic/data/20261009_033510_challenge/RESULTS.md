# E2 run `20261009_033510_challenge`

- mode: **challenge**, channel: 10, bin: 100 ms, lines: 3187
- duration: 82.5 s, devices seen: 16, APs: 18

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` |  | S | 10 | 263.64 | 7.97 | 18820 | -45 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 22.14 | 264.75 | 6955 | -43 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.76 | 14.16 | 2220 | -38 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.35 | 0.00 | 1022 | -42 |
| `f2:51:4e:f8:1a:7f` | (random) | S | 10 | 0.07 | 0.00 | 181 | -88 |
| `b0:a7:b9:23:82:ee` |  | A | 10 | 0.05 | 0.07 | 7 | -88 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.04 | 0.00 | 39 | -77 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.02 | 0.00 | 8 | -87 |
| `46:88:44:6e:4f:e4` | (random) | S | 10 | 0.02 | 0.01 | 17 | -38 |
| `64:d1:54:63:44:f4` |  | A | 10 | 0.00 | 0.00 | 2 | -90 |
| `98:5f:41:bf:7c:ff` |  | S | 10 | 0.00 | 0.00 | 6 | -70 |
| `e2:5e:2c:92:7b:df` | (random) | S | 10 | 0.00 | 0.00 | 4 | -89 |

## Passive streaming signature (no stimulus needed)

| MAC | up kB/s | down kB/s | up ratio | duty >10 kB/s | CV (5 s) | STREAMER |
|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 263.9 | 8.0 | 0.97 | 1.00 | 0.23 | **YES** |
| `5c:62:8b:66:67:d2` | 22.2 | 266.4 | 0.08 | 0.37 | 1.29 | - |

Rule (provisional, not yet validated): up ≥ 10 kB/s, duty ≥ 0.9, CV ≤ 0.5, up ratio ≥ 0.9, duration ≥ 30 s.

## Packet-size shape of the uplink (Method 1.3)

| MAC | <100 B | 100–699 | 700–1199 | ≥1200 B | bimodal? |
|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 4% | 6% | 5% | 85% | yes |
| `5c:62:8b:66:67:d2` | 0% | 87% | 1% | 12% | - |
| `72:3b:01:24:b7:c8` | 100% | 0% | 0% | 0% | yes |
| `b2:db:ca:e6:29:a6` | 100% | 0% | 0% | 0% | yes |
| `f2:51:4e:f8:1a:7f` | 100% | 0% | 0% | 0% | - |

*Video fragments into max-size packets + tiny ACKs → tiny and large bins dominate, middle bins near zero (bimodal). Provisional: tiny+large ≥ 0.8 and mid ≤ 0.1.*

## Challenge (K = 12, T = 5.0 s, lag = 0 ms, 10000 permutations, two-sided)

Stimulus: `001111010001`

| MAC | vendor | S (corr) | p | level | tx ON kB/s | tx OFF kB/s | ON/OFF | direction |
|---|---|---|---|---|---|---|---|---|
| `5c:62:8b:66:67:d2` |  | +0.321 | 0.2757 | **NONE** | 37.44 | 16.05 | 2.33 | - |
| `72:3b:01:24:b7:c8` | (random) | -0.345 | 0.3072 | **NONE** | 0.73 | 0.78 | 0.94 | - |
| `cc:b8:5e:ad:5c:4a` |  | -0.231 | 0.4481 | **NONE** | 244.97 | 268.58 | 0.91 | - |
| `f2:51:4e:f8:1a:7f` | (random) | +0.225 | 0.4518 | **NONE** | 0.08 | 0.06 | 1.40 | - |
| `b2:db:ca:e6:29:a6` | (random) | +0.154 | 0.6220 | **NONE** | 0.35 | 0.33 | 1.07 | - |

*Measured data from this run. Levels use the pre-registered lag; lag.png is exploratory only.*

## Per-slot uplink (kB/s) and validity checks

| MAC | 0 | 1 | 2* | 3* | 4* | 5* | 6 | 7* | 8 | 9 | 10 | 11* | trend ρ | dropouts |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `5c:62:8b:66:67:d2` | 8 | 7 | 21 | 54 | 9 | 123 | 53 | 10 | 10 | 6 | 11 | 8 | -0.03 | 0 |
| `72:3b:01:24:b7:c8` | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | +0.47 | 0 |
| `cc:b8:5e:ad:5c:4a` | 255 | 302 | 350 | 224 | 223 | 188 | 290 | 272 | 327 | 183 | 255 | 212 | -0.31 | 0 |
| `f2:51:4e:f8:1a:7f` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | -0.50 | 0 |

`*` = MOVE/ON slot.
