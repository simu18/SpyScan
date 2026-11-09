# E2 run `20261023_024931_survey`

- mode: **survey**, channel: hop, bin: 100 ms, lines: 106
- duration: 15.8 s, devices seen: 19, APs: 26
- survey rates are estimated per on-channel time (duration / 13); use `record` on one channel for accurate rates

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` |  | S | 10 | 137.46 | 6.22 | 150 | -37 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 101.96 | 144.18 | 170 | -47 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 6.72 | 95.73 | 83 | -36 |
| `6a:de:c6:6b:8e:12` | (random) | S | 1 | 0.60 | 0.00 | 23 | -76 |
| `a6:4c:bd:b6:c7:99` | (random) | S | 2 | 0.12 | 0.00 | 5 | -82 |
| `80:af:ca:68:4c:f8` |  | A | 9 | 0.07 | 0.00 | 1 | -82 |
| `a6:8d:f0:4a:eb:78` | (random) | S | 1 | 0.05 | 0.00 | 2 | -85 |
| `3e:4e:c3:c9:e5:0e` | (random) | S | 8 | 0.05 | 0.00 | 2 | -88 |
| `ba:06:0d:93:c3:c8` | (random) | S | 5 | 0.05 | 0.00 | 2 | -87 |
| `2a:f5:5d:f2:11:3e` | (random) | S | 1 | 0.02 | 0.00 | 1 | -80 |
| `92:fe:e0:6b:a8:b6` | (random) | S | 11 | 0.02 | 0.00 | 1 | -90 |
| `70:3a:2d:f4:31:9a` |  | S | 4 | 0.02 | 0.00 | 1 | -82 |

## Passive streaming signature (no stimulus needed)

| MAC | up kB/s | down kB/s | up ratio | duty >10 kB/s | CV (5 s) | STREAMER |
|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 11.1 | 0.5 | 0.96 | 0.20 | 0.80 | - |
| `5c:62:8b:66:67:d2` | 8.3 | 11.7 | 0.41 | 0.13 | 0.71 | - |

Rule (provisional, not yet validated): up ≥ 10 kB/s, duty ≥ 0.9, CV ≤ 0.5, up ratio ≥ 0.9, duration ≥ 30 s.

## Packet-size shape of the uplink (Method 1.3)

| MAC | <100 B | 100–699 | 700–1199 | ≥1200 B | video-like? |
|---|---|---|---|---|---|
| `5c:62:8b:66:67:d2` | 0% | 47% | 5% | 48% | - |
| `cc:b8:5e:ad:5c:4a` | 5% | 8% | 8% | 79% | - |
| `72:3b:01:24:b7:c8` | 16% | 84% | 0% | 0% | - |

*A streaming camera's own uplink is dominated by max-size (≥1200 B) packets; idle phones/plugs send only tiny packets. Provisional: large fraction ≥ 0.5.*
