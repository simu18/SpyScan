# Work log

Dated entries, newest first. Each entry: what was done · decisions · open questions · next step.

## 2026-10-01 — Project restart, ТЗ v2
- Studied the course requirements, the ТЗ template, the original idea (CamGuard 360) and the defended draft ТЗ.
- Defense feedback (as recalled by the developer): the goal must state the **problem**, not describe the device. The goal was rewritten in ТЗ v2 §1.3.
- **Decision D-M1:** method 1 changed from a brightness threshold to *differential ON/OFF retroreflection imaging + off-axis test*. Reason: a brightness threshold cannot reject lamps, indicator LEDs or glints.
- **Decision D-M2:** method 2 changed from an AP/BLE list to *passive Wi-Fi traffic monitoring + stimulus–traffic correlation*. Reason: client-mode cameras are invisible to an AP scan, and published work (DeWiCam, Singh et al., Lumos) shows that traffic responds to the scene.
- Language: English. Repository: GitHub. Funding: self-funded; lab access for equipment and the 3D printer.
- Hardware owned: ESP32-S3 DevKit, Ra-02 LoRa 433, CC1101 433 (the 433 MHz modules are not used in V1).
- **Next:** send ТЗ v2 + repo link to the instructor; start E2 (Wi-Fi traffic) on the owned DevKit; order the camera board, display, LEDs and test cameras.
