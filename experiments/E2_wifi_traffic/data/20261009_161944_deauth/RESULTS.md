# E2 run `20261009_161944_deauth`

- mode: **deauth**, channel: 10, bin: 100 ms, lines: 675
- duration: 19.2 s, devices seen: 13, APs: 14

## Busiest transmitters

| MAC | vendor hint | role | ch | tx kB/s | rx kB/s | tx frames | RSSI |
|---|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` |  | S | 10 | 261.60 | 8.63 | 4325 | -46 |
| `5c:62:8b:66:67:d2` |  | A | 10 | 8.64 | 262.79 | 1481 | -50 |
| `72:3b:01:24:b7:c8` | (random) | S | 10 | 0.79 | 0.01 | 533 | -43 |
| `b2:db:ca:e6:29:a6` | (random) | S | 10 | 0.40 | 0.00 | 210 | -40 |
| `98:5f:41:bf:7c:ff` |  | S | 10 | 0.23 | 0.02 | 9 | -78 |
| `cc:d8:43:ca:9f:eb` |  | A | 10 | 0.03 | 0.23 | 4 | -85 |
| `78:8c:b5:f5:e7:26` |  | A | 10 | 0.02 | 0.00 | 4 | -79 |
| `f2:51:4e:f8:1a:7f` | (random) | S | 10 | 0.01 | 0.00 | 2 | -92 |
| `b0:a7:b9:23:82:ee` |  | A | 10 | 0.00 | 0.01 | 1 | -85 |
| `80:af:ca:68:4c:f8` |  | A | 10 | 0.00 | 0.00 | 1 | -91 |
| `d2:9d:99:fa:eb:b5` | (random) | S | 1 | 0.00 | 0.00 | 2 | -85 |
| `00:08:22:24:d6:fb` |  | S | 10 | 0.00 | 0.00 | 1 | -84 |

## Passive streaming signature (no stimulus needed)

| MAC | up kB/s | down kB/s | up ratio | duty >10 kB/s | CV (5 s) | STREAMER |
|---|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 262.3 | 8.7 | 0.97 | 1.00 | 0.32 | - |
| `5c:62:8b:66:67:d2` | 8.7 | 265.6 | 0.03 | 0.16 | 0.26 | - |

Rule (provisional, not yet validated): up ≥ 10 kB/s, duty ≥ 0.9, CV ≤ 0.5, up ratio ≥ 0.9, duration ≥ 30 s.

## Packet-size shape of the uplink (Method 1.3)

| MAC | <100 B | 100–699 | 700–1199 | ≥1200 B | video-like? |
|---|---|---|---|---|---|
| `cc:b8:5e:ad:5c:4a` | 4% | 6% | 4% | 85% | YES |
| `5c:62:8b:66:67:d2` | 0% | 100% | 0% | 0% | - |
| `72:3b:01:24:b7:c8` | 99% | 1% | 0% | 0% | - |
| `b2:db:ca:e6:29:a6` | 99% | 1% | 0% | 0% | - |

*A streaming camera's own uplink is dominated by max-size (≥1200 B) packets; idle phones/plugs send only tiny packets. Provisional: large fraction ≥ 0.5.*

## Active Re-Association Provocation (Method 3.3)

- Target candidate: `cc:b8:5e:ad:5c:4a`
- Reconnection latency: **293 ms**
- Post-provocation frames: **3757** (4372.6 kB)
- Re-association RSSI: **-45 dBm**
- Verification verdict: **PROVOKED (Station dropped and re-associated)**

