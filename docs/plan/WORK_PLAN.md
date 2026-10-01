# Work plan

Each stage follows the same loop: **analyse → explain → alternatives → implications → unknowns → experiment → decide → update architecture**.

| # | Stage | Key outputs | Planned | Course checkpoint |
|---|---|---|---|---|
| 0 | Study of materials, critique of the draft | Analysis, decisions D-M1/D-M2 | 01.10 | — |
| 1 | ТЗ v2 + repository | `docs/tz/TZ_SpyScan_v2.md`, GitHub repo shared with the instructor, BOM/estimate | 01.10 – 09.10 | **ТЗ (10 pts)** |
| 2 | Feasibility experiments | E2 (Wi-Fi traffic, owned DevKit), E1 (retroreflection), E3 (off-axis), E4 (load) | 05.10 – 25.10 | — |
| 3 | Architecture | HW block diagram, wiring, interfaces, **power budget**; SW modules and FreeRTOS tasks (PlantUML); enclosure concept | 19.10 – 01.11 | **Architecture (15 pts)** |
| 4 | HW bring-up | Breadboard prototype, bring-up checklist | 26.10 – 08.11 | — |
| 5 | Firmware | Drivers, synced acquisition, Wi-Fi monitor, UI, developer mode | 02.11 – 29.11 | — |
| 6 | Algorithms + calibration | Detectors, thresholds from data, datasets | 09.11 – 29.11 | — |
| — | Interim demo | Bench + key function + processed data | ≈ 16–23.11 (TBC) | **Interim demo (15 pts)** |
| 7 | Enclosure + integration | 3D model, print, assembly, battery | 16.11 – 04.12 | — |
| 8 | Test programme | AT-1 … AT-11, test report | 30.11 – 08.12 | — |
| 9 | PIKT-Fest / report / defense | Slides, demo scenario, final report | Dec | **Report (20) + Defense (10)** |

## Immediate next steps
1. Push the repo to GitHub and send the link and ТЗ v2 to the instructor (ezinovichev@itmo.ru / @scarletshroud).
2. Order: ESP32-S3 camera board (≥ 8 MB PSRAM), ST7789 display, red LEDs, MOSFETs, Li-Po + charger, 1 mini Wi-Fi camera (2.4 GHz, STA mode), 1 ESP32-CAM.
3. While waiting for parts: E2 on the owned ESP32-S3 DevKit (promiscuous sniffer → per-MAC byte rate over USB → Python plots).
