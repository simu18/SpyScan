# E2 run `20261009_164825_record`

- mode: **record**, channel: 10, bin: 100 ms, lines: 2467
- duration: 10.7 s, devices seen: 9, APs: 15

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` |  | S | 10 | 223.33 | 7.17 | 2092 | -53 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 7.42 | 224.45 | 695 | -49 |
| `98:5f:41:bf:7c:ff` |  | S | 10 | 3.82 | 0.70 | 120 | -74 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.80 | 0.24 | 304 | -48 |
| `cc:d8:43:ca:9f:eb` |  | A | 10 | 0.70 | 3.82 | 7 | -85 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.25 | 0.00 | 95 | -38 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.08 | 0.00 | 4 | -87 |
| `2c:9c:58:ec:ae:2b` |  | S | 10 | 0.07 | 0.01 | 25 | -65 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.06 | 0.00 | 5 | -85 |

## Passive streaming signature (no stimulus needed)

| MAC | up kB/s | down kB/s | up ratio | duty >10 kB/s | CV (5 s) | STREAMER |
|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 216.5 | 7.7 | 0.97 | 1.00 | 0.05 | - |
| `5c:62:8b:66:67:d2` | 7.3 | 240.2 | 0.03 | 0.00 | 0.07 | - |
| `98:5f:41:bf:7c:ff` | 3.7 | 0.8 | 0.83 | 0.00 | 0.30 | - |

Rule (provisional, not yet validated): up ≥ 10 kB/s, duty ≥ 0.9, CV ≤ 0.5, up ratio ≥ 0.9, duration ≥ 30 s.

## Packet-size shape of the uplink (Method 1.3)

| MAC | <100 B | 100–699 | 700–1199 | ≥1200 B | video-like? |
|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 5% | 6% | 5% | 84% | YES |
| `5c:62:8b:66:67:d2` | 0% | 100% | 0% | 0% | - |
| `72:3b:01:24:b7:c8` | 100% | 0% | 0% | 0% | - |
| `98:5f:41:bf:7c:ff` | 0% | 88% | 1% | 11% | - |
| `b2:db:ca:e6:29:a6` | 100% | 0% | 0% | 0% | - |

*A streaming camera's own uplink is dominated by max-size (≥1200 B) packets; idle phones/plugs send only tiny packets. Provisional: large fraction ≥ 0.5.*

## Focused-device frame-clock fingerprint (Method 1.2)

- packets: 1953 in 10 s → 157 bursts (15.6/s)
- median inter-burst interval: **62 ms**
- best-matching frame rate: **24 fps** (54% of intervals within ±4 ms of 1× or 2× the period)

*A human, a download or browsing has no tight 25/30 fps burst cadence; a camera does. Combine with uplink-dominant + large-packet to corroborate.*
