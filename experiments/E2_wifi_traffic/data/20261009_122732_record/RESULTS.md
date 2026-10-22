# E2 run `20261009_122732_record`

- mode: **record**, channel: 10, bin: 100 ms, lines: 1638
- duration: 61.1 s, devices seen: 16, APs: 15

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `5c:62:8b:66:67:d2` |  | A | 10 | 2.27 | 0.57 | 659 | -46 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.50 | 2.08 | 1084 | -31 |
| `a6:4c:bd:b6:c7:99` | (random) | S | 10 | 0.13 | 0.03 | 172 | -86 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.07 | 0.00 | 162 | -47 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.07 | 0.13 | 20 | -75 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.02 | 0.00 | 5 | -88 |
| `cc:d8:43:ca:9f:eb` |  | A | 10 | 0.02 | 0.01 | 12 | -88 |
| `f2:51:4e:f8:1a:7f` | (random) | S | 10 | 0.01 | 0.00 | 18 | -91 |
| `26:5a:ee:af:90:48` | (random) | S | 10 | 0.01 | 0.02 | 18 | -76 |
| `98:5f:41:bf:7c:ff` |  | S | 10 | 0.00 | 0.00 | 10 | -75 |
| `cc:2d:e0:18:15:c7` |  | A | 1 | 0.00 | 0.00 | 1 | -92 |
| `b0:a7:b9:23:82:ee` |  | A | 10 | 0.00 | 0.01 | 1 | -87 |

## Passive streaming signature (no stimulus needed)

| MAC | up kB/s | down kB/s | up ratio | duty >10 kB/s | CV (5 s) | STREAMER |
|---|---|---|---|---|---|---|
| `5c:62:8b:66:67:d2` | 2.3 | 0.6 | 0.80 | 0.10 | 1.97 | - |

Rule (provisional, not yet validated): up ≥ 10 kB/s, duty ≥ 0.9, CV ≤ 0.5, up ratio ≥ 0.9, duration ≥ 30 s.

## Packet-size shape of the uplink (Method 1.3)

| MAC | <100 B | 100–699 | 700–1199 | ≥1200 B | video-like? |
|---|---|---|---|---|---|
| `b2:db:ca:e6:29:a6` | 100% | 0% | 0% | 0% | - |
| `5c:62:8b:66:67:d2` | 1% | 96% | 1% | 2% | - |
| `a6:4c:bd:b6:c7:99` | 92% | 8% | 0% | 1% | - |
| `72:3b:01:24:b7:c8` | 100% | 0% | 0% | 0% | - |

*A streaming camera's own uplink is dominated by max-size (≥1200 B) packets; idle phones/plugs send only tiny packets. Provisional: large fraction ≥ 0.5.*

## Quiet-device keep-alive periodicity (Method 3.2)

A connected-but-idle camera still sends NAT keep-alives at a fixed interval. This finds *presence*, not identity (any IoT device may heartbeat).

| MAC | frames | median gap s | regularity | RSSI |
|---|---|---|---|---|
| `78:8c:b5:f5:e7:26` | 18 | 9.0 | 0.81 | -75 |
| `f2:51:4e:f8:1a:7f` | 11 | 8.3 | 0.80 | -91 |
| `26:5a:ee:af:90:48` | 9 | 10.9 | 0.69 | -76 |

*regularity = 1 − (IQR / median gap); near 1 = very periodic.*
