# E2 run `20261009_155601_record`

- mode: **record**, channel: 10, bin: 100 ms, lines: 1835
- duration: 61.1 s, devices seen: 11, APs: 14

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` |  | S | 10 | 203.19 | 7.41 | 10862 | -47 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 8.55 | 203.99 | 4103 | -50 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.80 | 1.14 | 1733 | -48 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.06 | 0.00 | 36 | -79 |
| `f2:51:4e:f8:1a:7f` | (random) | S | 10 | 0.04 | 0.00 | 89 | -92 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.02 | 0.00 | 9 | -88 |
| `b0:a7:b9:23:82:ee` |  | A | 10 | 0.00 | 0.04 | 1 | -89 |
| `c4:16:88:9a:06:5c` |  | A | 1 | 0.00 | 0.00 | 1 | -85 |
| `00:08:22:24:d6:fb` |  | S | 10 | 0.00 | 0.00 | 2 | -81 |
| `d2:9d:99:fa:eb:b5` | (random) | S | 1 | 0.00 | 0.00 | 1 | -84 |
| `c2:a5:dd:1b:60:3b` | (random) | A | 1 | 0.00 | 0.00 | 0 | -75 |

## Passive streaming signature (no stimulus needed)

| MAC | up kB/s | down kB/s | up ratio | duty >10 kB/s | CV (5 s) | STREAMER |
|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 202.0 | 7.4 | 0.96 | 1.00 | 0.11 | **YES** |
| `5c:62:8b:66:67:d2` | 8.5 | 204.3 | 0.04 | 0.10 | 0.42 | - |

Rule (provisional, not yet validated): up ≥ 10 kB/s, duty ≥ 0.9, CV ≤ 0.5, up ratio ≥ 0.9, duration ≥ 30 s.

## Packet-size shape of the uplink (Method 1.3)

| MAC | <100 B | 100–699 | 700–1199 | ≥1200 B | video-like? |
|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 5% | 7% | 5% | 83% | YES |
| `5c:62:8b:66:67:d2` | 0% | 99% | 0% | 1% | - |
| `72:3b:01:24:b7:c8` | 100% | 0% | 0% | 0% | - |
| `f2:51:4e:f8:1a:7f` | 100% | 0% | 0% | 0% | - |

*A streaming camera's own uplink is dominated by max-size (≥1200 B) packets; idle phones/plugs send only tiny packets. Provisional: large fraction ≥ 0.5.*

## Quiet-device keep-alive periodicity (Method 3.2)

A connected-but-idle camera still sends NAT keep-alives at a fixed interval. This finds *presence*, not identity (any IoT device may heartbeat).

| MAC | frames | median gap s | regularity | RSSI |
|---|---|---|---|---|
| `f2:51:4e:f8:1a:7f` | 64 | 10.0 | 0.66 | -92 |
| `80:af:ca:68:4c:f8` | 9 | 7.3 | 0.61 | -88 |
| `78:8c:b5:f5:e7:26` | 33 | 5.8 | 0.57 | -79 |

*regularity = 1 − (IQR / median gap); near 1 = very periodic.*
