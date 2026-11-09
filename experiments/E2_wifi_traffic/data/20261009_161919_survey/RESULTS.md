# E2 run `20261009_161919_survey`

- mode: **survey**, channel: hop, bin: 100 ms, lines: 64
- duration: 5.1 s, devices seen: 10, APs: 28
- survey rates are estimated per on-channel time (duration / 13); use `record` on one channel for accurate rates

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` |  | S | 10 | 259.13 | 9.25 | 88 | -45 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 9.25 | 260.77 | 33 | -49 |
| `e0:bb:0c:07:63:0b` |  | A | 4 | 1.41 | 0.00 | 3 | -76 |
| `7e:b3:d1:fb:f6:0a` | (random) | S | 9 | 1.21 | 0.00 | 17 | -90 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 1.00 | 0.00 | 14 | -42 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.64 | 0.00 | 9 | -43 |
| `5a:62:f6:96:2d:4c` | (random) | S | 1 | 0.15 | 0.00 | 2 | -85 |
| `30:87:d9:5e:85:c8` |  | A | 9 | 0.00 | 1.21 | 0 | nan |
| `b6:dd:26:24:a1:48` | (random) | S | 4 | 0.00 | 0.28 | 0 | nan |
| `c2:a5:dd:1b:60:3b` | (random) | A | 1 | 0.00 | 0.15 | 0 | -68 |

## Passive streaming signature (no stimulus needed)

| MAC | up kB/s | down kB/s | up ratio | duty >10 kB/s | CV (5 s) | STREAMER |
|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 15.5 | 0.7 | 0.96 | 0.20 | 2.00 | - |

Rule (provisional, not yet validated): up ≥ 10 kB/s, duty ≥ 0.9, CV ≤ 0.5, up ratio ≥ 0.9, duration ≥ 30 s.

## Packet-size shape of the uplink (Method 1.3)

| MAC | <100 B | 100–699 | 700–1199 | ≥1200 B | video-like? |
|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 5% | 6% | 5% | 85% | - |

*A streaming camera's own uplink is dominated by max-size (≥1200 B) packets; idle phones/plugs send only tiny packets. Provisional: large fraction ≥ 0.5.*
