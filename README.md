# SpyScan — portable hidden-camera detector

Semester project, **Embedded Systems (ITMO)**, project track 1.3.
Developer: **Rahaman Md Afifur** (P3330) · Instructor: Zinovichev E. S.

## Problem
Tiny Wi-Fi and SD-card cameras are cheap and easy to hide in rented rooms, hotels and changing rooms. Affordable detectors only say "some signal is present". They cannot give **evidence that is specific to a camera**.

## Approach (V1, this semester)
| Method | Physical basis | What it gives |
|---|---|---|
| **1. Differential optical retroreflection** | A lens focused on a sensor returns light towards its source (cat's-eye effect). The device alternates LED ON and OFF frames and subtracts them, which cancels lamps and ambient light. An off-axis LED test separates lenses from glints. | Location of lens candidates on the live preview |
| **2. Wi-Fi traffic stimulus–response** | A streaming camera's encoder bit rate follows changes in the scene. Passive 2.4 GHz monitoring records per-device uplink rate. A pseudo-random "move / still" stimulus is applied, and the device tests which transmitter's traffic correlates with it. | Which MAC is a camera, plus RSSI guidance towards it |

Hardware: ESP32-S3 (+ OV2640, red LED ring, ST7789 display, 3 buttons, Li-Po). Firmware: ESP-IDF / FreeRTOS, C.
Full specification: [`docs/tz/TZ_SpyScan_v2.md`](docs/tz/TZ_SpyScan_v2.md).

## Repository layout
```
docs/
  tz/            Technical specification (ТЗ) and its revisions
  analysis/      Problem analysis, physics, analogues, fact/hypothesis register
  architecture/  HW/SW block diagrams (PlantUML), power budget, pin map
  plan/          Work plan and schedule
  reference/     Course materials, ТЗ template, original idea, defended draft
  journal/       Dated work log (progress tracking)
experiments/     Protocols, raw data and results for E1..E4 and AT-1..AT-11
firmware/        ESP-IDF project(s)
hardware/        Schematics, wiring, BOM, enclosure CAD/STL
tools/           Python tools: serial capture, analysis, plotting
```

## Status
See [`docs/journal/LOG.md`](docs/journal/LOG.md) and the milestone tags (`v0.1-tz`, `v0.2-feasibility`, `v0.3-architecture`, …).

## Honesty rule
Every statement in this repository is labelled **Fact / Assumption / Hypothesis / Decision / Target / Measured**. No result is reported unless it was measured, and the data and script that produced it are in `experiments/`.
