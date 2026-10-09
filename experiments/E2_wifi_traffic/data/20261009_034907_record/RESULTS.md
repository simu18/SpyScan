# E2 run `closed`

- mode: **record**, channel: 10, bin: 100 ms, lines: 1397
- duration: 61.1 s, devices seen: 17, APs: 15

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` |  | S | 10 | 0.52 | 0.08 | 80 | -48 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.39 | 0.00 | 841 | -52 |
| `98:5f:41:bf:7c:ff` |  | S | 10 | 0.20 | 0.13 | 98 | -69 |
| `cc:d8:43:ca:9f:eb` |  | A | 10 | 0.12 | 0.07 | 27 | -76 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.11 | 0.00 | 24 | -87 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 0.10 | 0.96 | 47 | -49 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.05 | 0.02 | 29 | -83 |
| `f2:51:4e:f8:1a:7f` | (random) | S | 10 | 0.03 | 0.00 | 69 | -87 |
| `46:88:44:6e:4f:e4` | (random) | S | 10 | 0.03 | 0.01 | 21 | -37 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.03 | 0.00 | 64 | -39 |
| `a6:4c:bd:b6:c7:99` | (random) | S | 10 | 0.02 | 0.00 | 32 | -86 |
| `e2:5e:2c:92:7b:df` | (random) | S | 10 | 0.01 | 0.00 | 22 | -86 |

## Packet-size shape of the uplink (Method 1.3)

| MAC | <100 B | 100–699 | 700–1199 | ≥1200 B | video-like? |
|---|---|---|---|---|---|
| `b2:db:ca:e6:29:a6` | 100% | 0% | 0% | 0% | - |
| `98:5f:41:bf:7c:ff` | 61% | 37% | 2% | 0% | - |
| `cc:b8:5e:ad:5c:4a` | 21% | 60% | 0% | 19% | - |
| `f2:51:4e:f8:1a:7f` | 100% | 0% | 0% | 0% | - |
| `72:3b:01:24:b7:c8` | 100% | 0% | 0% | 0% | - |

*A streaming camera's own uplink is dominated by max-size (≥1200 B) packets; idle phones/plugs send only tiny packets. Provisional: large fraction ≥ 0.5.*

## Quiet-device keep-alive periodicity (Method 3.2)

A connected-but-idle camera still sends NAT keep-alives at a fixed interval. This finds *presence*, not identity (any IoT device may heartbeat).

| MAC | frames | median gap s | regularity | RSSI |
|---|---|---|---|---|
| `98:5f:41:bf:7c:ff` | 25 | 5.6 | 0.78 | -69 |
| `f2:51:4e:f8:1a:7f` | 49 | 4.9 | 0.71 | -87 |
| `5c:62:8b:66:67:d2` | 26 | 4.8 | 0.69 | -49 |
| `78:8c:b5:f5:e7:26` | 23 | 5.4 | 0.67 | -83 |
| `e2:5e:2c:92:7b:df` | 11 | 9.7 | 0.63 | -86 |

*regularity = 1 − (IQR / median gap); near 1 = very periodic.*
