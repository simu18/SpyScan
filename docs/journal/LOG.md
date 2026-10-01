# Work log

Dated entries, newest first. Each entry: what was done · decisions · open questions · next step.

## 2026-10-02 — Parts list, architecture draft, simulation
- Parts list finalised for ordering (see the BOM in the ТЗ §4.1). The display changed from ST7789 to **ILI9341 2.4"** because the Wokwi simulator supports it, so the simulated UI is the real UI. Added test targets: an ESP32-CAM and a 2.4 GHz mini Wi-Fi camera.
- Architecture draft 0.1 (`docs/architecture/ARCHITECTURE.md`): HW block diagram, interface table, draft pin map (to verify on the real board), LED driver calculation, power budget (`tools/power_budget.py`: ≈ 5.3 h estimated vs the 1.5 h target), FreeRTOS task design, timing analysis, UI state machine, risks. All diagrams in PlantUML.
- Wokwi HMI simulation (`sim/wokwi/`): ESP32-S3 + ILI9341 + 3 buttons + LEDs + buzzer + battery/distance pots; real `ui_fsm.c` and `challenge.c` running on synthetic data.
- PC tests (`sim/test/`): 22 UI transition checks pass; Monte-Carlo test of the challenge detector.
- **Findings from the simulation (synthetic data, design-level):**
  - K ≥ 10 slots is mandatory, because the minimum achievable p is 1/C(K, K/2).
  - A phone viewing the stream also correlates with the stimulus, so the **direction feature** (uplink vs downlink) is added to separate source from viewer (hypothesis H4).
- Found: the ТЗ edit removed the requirement IDs and the [F]/[A]/[T] legend, but sections 2.4.1, 6.3 and Appendix B still reference them. To fix in ТЗ v2.1.
- **Next:** E2 sniffer firmware for the owned DevKit; Python capture and analysis; ESP-IDF install on the Mac.

## 2026-10-01 — Project restart, ТЗ v2
- Studied the course requirements, the ТЗ template, the original idea (CamGuard 360) and the defended draft ТЗ.
- Defense feedback (as recalled by the developer): the goal must state the **problem**, not describe the device. The goal was rewritten in ТЗ v2 §1.3.
- **Decision D-M1:** method 1 changed from a brightness threshold to *differential ON/OFF retroreflection imaging + off-axis test*. Reason: a brightness threshold cannot reject lamps, indicator LEDs or glints.
- **Decision D-M2:** method 2 changed from an AP/BLE list to *passive Wi-Fi traffic monitoring + stimulus–traffic correlation*. Reason: client-mode cameras are invisible to an AP scan, and published work (DeWiCam, Singh et al., Lumos) shows that traffic responds to the scene.
- Language: English. Repository: GitHub. Funding: self-funded; lab access for equipment and the 3D printer.
- Hardware owned: ESP32-S3 DevKit, Ra-02 LoRa 433, CC1101 433 (the 433 MHz modules are not used in V1).
- **Next:** send ТЗ v2 + repo link to the instructor; start E2 (Wi-Fi traffic) on the owned DevKit; order the camera board, display, LEDs and test cameras.
