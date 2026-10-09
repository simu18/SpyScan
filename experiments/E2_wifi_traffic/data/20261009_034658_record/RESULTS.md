# E2 run `stream`

- mode: **record**, channel: 10, bin: 100 ms, lines: 15255
- duration: 61.3 s, devices seen: 17, APs: 18

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` |  | S | 10 | 245.77 | 7.57 | 13068 | -46 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 74.23 | 246.83 | 7812 | -46 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.72 | 66.65 | 1567 | -40 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.33 | 0.00 | 716 | -45 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.10 | 0.01 | 43 | -84 |
| `f2:51:4e:f8:1a:7f` | (random) | S | 10 | 0.03 | 0.00 | 62 | -90 |
| `46:88:44:6e:4f:e4` | (random) | S | 10 | 0.02 | 0.01 | 15 | -30 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.02 | 0.00 | 8 | -86 |
| `e2:5e:2c:92:7b:df` | (random) | S | 10 | 0.00 | 0.00 | 8 | -86 |
| `64:d1:54:63:44:f4` |  | A | 10 | 0.00 | 0.00 | 3 | -89 |
| `00:08:22:24:d6:fb` |  | S | 10 | 0.00 | 0.00 | 8 | -78 |
| `b0:a7:b9:23:82:ee` |  | A | 10 | 0.00 | 0.04 | 2 | -90 |

## Passive streaming signature (no stimulus needed)

| MAC | up kB/s | down kB/s | up ratio | duty >10 kB/s | CV (5 s) | STREAMER |
|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 245.5 | 7.6 | 0.97 | 1.00 | 0.20 | **YES** |
| `5c:62:8b:66:67:d2` | 73.9 | 248.0 | 0.23 | 0.57 | 0.90 | - |

Rule (provisional, not yet validated): up ≥ 10 kB/s, duty ≥ 0.9, CV ≤ 0.5, up ratio ≥ 0.9, duration ≥ 30 s.

## Packet-size shape of the uplink (Method 1.3)

| MAC | <100 B | 100–699 | 700–1199 | ≥1200 B | video-like? |
|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 4% | 6% | 5% | 84% | YES |
| `5c:62:8b:66:67:d2` | 0% | 60% | 2% | 38% | - |
| `72:3b:01:24:b7:c8` | 100% | 0% | 0% | 0% | - |
| `b2:db:ca:e6:29:a6` | 100% | 0% | 0% | 0% | - |
| `f2:51:4e:f8:1a:7f` | 100% | 0% | 0% | 0% | - |

*A streaming camera's own uplink is dominated by max-size (≥1200 B) packets; idle phones/plugs send only tiny packets. Provisional: large fraction ≥ 0.5.*

## Quiet-device keep-alive periodicity (Method 3.2)

A connected-but-idle camera still sends NAT keep-alives at a fixed interval. This finds *presence*, not identity (any IoT device may heartbeat).

| MAC | frames | median gap s | regularity | RSSI |
|---|---|---|---|---|
| `00:08:22:24:d6:fb` | 7 | 12.5 | 0.92 | -78 |
| `80:af:ca:68:4c:f8` | 8 | 6.9 | 0.72 | -86 |
| `f2:51:4e:f8:1a:7f` | 49 | 5.1 | 0.56 | -90 |

*regularity = 1 − (IQR / median gap); near 1 = very periodic.*

## Focused-device frame-clock fingerprint (Method 1.2)

- packets: 12952 in 60 s → 928 bursts (15.4/s)
- median inter-burst interval: **63 ms**
- best-matching frame rate: **25 fps** (53% of intervals within ±4 ms of 1× or 2× the period)

*A human, a download or browsing has no tight 25/30 fps burst cadence; a camera does. Combine with uplink-dominant + large-packet to corroborate.*
