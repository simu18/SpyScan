# Experiments

Each experiment lives in its own folder `E<n>_<name>/` containing:

```
PROTOCOL.md   question, hypothesis, set-up (photo), variables, procedure, number of trials
data/         raw data (CSV / frames), never edited by hand
analysis.py   script that produces every figure and number in RESULTS.md
RESULTS.md    measured results + conclusion + effect on the ТЗ/architecture
```

Every data file carries its metadata: date, firmware commit, distance, angle, lux, target, distractors.

| ID | Question | Status |
|---|---|---|
| E1 | Differential retroreflection signal vs distance, LED offset and ambient light → set k, N, r | planned |
| E2 | Can an ESP32-S3 see a camera's uplink, and does it follow a motion/light stimulus? | **protocol + firmware + tools ready** → [PROTOCOL](E2_wifi_traffic/PROTOCOL.md) |
| E3 | [H1] On-axis/off-axis response ratio: lens vs specular distractors | planned |
| E4 | Camera + Wi-Fi monitor on one ESP32-S3: frame/packet loss, CPU, heap | planned |
