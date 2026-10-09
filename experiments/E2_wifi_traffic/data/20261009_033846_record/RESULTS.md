# E2 run `rec`

- mode: **record**, channel: 10, bin: 100 ms, lines: 2274
- duration: 61.4 s, devices seen: 16, APs: 14

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` |  | S | 10 | 211.94 | 7.76 | 11384 | -46 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 30.48 | 213.02 | 5487 | -45 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.71 | 22.69 | 1563 | -40 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.34 | 0.00 | 749 | -44 |
| `cc:d8:43:ca:9f:eb` |  | A | 10 | 0.09 | 0.01 | 20 | -78 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.07 | 0.00 | 16 | -88 |
| `44:df:65:f4:66:07` |  | A | 1 | 0.05 | 0.00 | 6 | -86 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.04 | 0.00 | 25 | -75 |
| `f2:51:4e:f8:1a:7f` | (random) | S | 10 | 0.04 | 0.00 | 70 | -86 |
| `46:88:44:6e:4f:e4` | (random) | S | 10 | 0.03 | 0.02 | 19 | -36 |
| `98:5f:41:bf:7c:ff` |  | S | 10 | 0.01 | 0.00 | 14 | -70 |
| `e2:5e:2c:92:7b:df` | (random) | S | 10 | 0.00 | 0.00 | 10 | -87 |

## Passive streaming signature (no stimulus needed)

| MAC | up kB/s | down kB/s | up ratio | duty >10 kB/s | CV (5 s) | STREAMER |
|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 212.1 | 7.8 | 0.96 | 1.00 | 0.08 | **YES** |
| `5c:62:8b:66:67:d2` | 30.6 | 214.4 | 0.12 | 0.56 | 1.18 | - |

Rule (provisional, not yet validated): up ≥ 10 kB/s, duty ≥ 0.9, CV ≤ 0.5, up ratio ≥ 0.9, duration ≥ 30 s.

## Packet-size shape of the uplink (Method 1.3)

| MAC | <100 B | 100–699 | 700–1199 | ≥1200 B | video-like? |
|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 5% | 7% | 5% | 83% | YES |
| `5c:62:8b:66:67:d2` | 0% | 80% | 2% | 18% | - |
| `72:3b:01:24:b7:c8` | 100% | 0% | 0% | 0% | - |
| `b2:db:ca:e6:29:a6` | 100% | 0% | 0% | 0% | - |
| `f2:51:4e:f8:1a:7f` | 100% | 0% | 0% | 0% | - |

*A streaming camera's own uplink is dominated by max-size (≥1200 B) packets; idle phones/plugs send only tiny packets. Provisional: large fraction ≥ 0.5.*

## Quiet-device keep-alive periodicity (Method 3.2)

A connected-but-idle camera still sends NAT keep-alives at a fixed interval. This finds *presence*, not identity (any IoT device may heartbeat).

| MAC | frames | median gap s | regularity | RSSI |
|---|---|---|---|---|
| `f2:51:4e:f8:1a:7f` | 43 | 5.6 | 0.68 | -86 |
| `98:5f:41:bf:7c:ff` | 8 | 25.2 | 0.64 | -70 |
| `46:88:44:6e:4f:e4` | 12 | 6.5 | 0.57 | -36 |
| `e2:5e:2c:92:7b:df` | 7 | 9.7 | 0.52 | -87 |

*regularity = 1 − (IQR / median gap); near 1 = very periodic.*
