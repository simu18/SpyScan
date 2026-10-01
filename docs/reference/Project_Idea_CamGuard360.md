# **CamGuard 360**

## **Portable Multimodal Hidden-Camera Detection System**

### **1\. Project vision**

**CamGuard 360** is a portable embedded device intended to help users find hidden cameras in places such as:

* hotels;  
* changing rooms;  
* offices;  
* private rooms;  
* meeting rooms;  
* temporary accommodation.

The long-term goal is not to create another simple RF detector or another flashlight that makes camera lenses glow.

The final vision is:

> **One portable device that combines several independent detection methods and uses them together to identify different types of hidden cameras.**

A camera may be difficult to detect using one technique but detectable using another.

For example:

Wi-Fi camera  
→ wireless scanning may detect it

Offline SD-card camera  
→ wireless scanning may fail  
→ optical or EM analysis may detect it

Camera hidden behind a small opening  
→ optical detection may become difficult  
→ thermal or EM detection may provide additional evidence

Powered-off camera  
→ RF/EM/thermal methods may fail  
→ optical detection may still work  
→ future NLJD-type technology could potentially detect electronics

Therefore CamGuard is designed around **multimodal detection** rather than one sensor.

---

# **2\. Main problem**

Many inexpensive hidden-camera detectors currently rely mainly on two concepts.

### **RF detection**

They search for radio-frequency transmissions.

This can help detect:

Wi-Fi cameras  
Bluetooth devices  
wireless video transmitters  
other transmitting electronics

But a camera may simply record video to:

microSD card  
internal storage  
wired recorder

and transmit nothing.

In that case RF detection alone is insufficient.

---

### **Optical lens detection**

Many consumer detectors use:

LEDs  
↓  
camera lens  
↓  
bright reflection  
↓  
user sees bright dot

The user normally has to look through a small viewing window while manually scanning the room.

Problems include:

human has to notice the reflection  
shiny screws may reflect  
glass reflects  
polished metal reflects  
LEDs reflect  
small lenses are easy to miss

CamGuard should eventually automate and improve this process.

---

# **3\. Core philosophy**

CamGuard does not depend on one method.

The system will eventually use several sources of evidence:

              CAMGUARD 360

                     Room  
                      │  
        ┌─────────────┼─────────────┐  
        │             │                  │  
        ▼             ▼               ▼  
    Optical       Wireless/RF      EM  
    analysis        analysis     analysis  
        │             │                      │  
        └─────────────┼─────────────┘  
                      │  
                 Optional future  
                 thermal sensing  
                      │  
                      ▼  
                 Sensor Fusion  
                      │  
                      ▼  
               Confidence Score  
                      │  
                      ▼  
             User warning/result

Instead of:

> **RF detected \= camera**

the final system should be able to say something more meaningful, such as:

Possible hidden camera

Optical evidence       HIGH  
Wireless evidence      NONE  
EM recording evidence  HIGH

Likely type:  
Offline/local-storage camera

Confidence:  
91%  
---

# **4\. Development strategy**

The project will be developed in stages.

This is very important.

We **will not attempt the hardest features first**.

For the semester prototype we will build the simplest reliable version first.

Then advanced features can be added gradually.

---

# **VERSION 1**

## **Semester prototype   easiest reliable version**

This is the version that should be considered the **required project**.

It will contain two main detection methods.

### **Method 1   Optical lens-reflection detection**

### **Method 2   Wi-Fi/Bluetooth scanning**

The goal is:

> **Build something complete, demonstrable and reliable before attempting experimental research features.**

---

# **5\. V1 Feature 1   Optical lens detection**

The device will contain:

LED illumination  
\+  
camera  
\+  
display

The basic principle is:

Our LED  
   ↓  
camera lens  
   ↓  
reflection  
   ↓  
our camera

Camera lenses can produce a strong reflection when illumination is close to the viewing axis.

The first prototype does not need complicated AI.

### **Basic V1 implementation**

The device camera captures the scene.

The LEDs illuminate the scene.

The software searches for bright reflective points.

Possible objects:

camera lens  
screw  
glass  
metal  
decorative object  
LED indicator

The prototype may initially show these suspicious points to the user rather than perfectly deciding which one is a camera.

Example interface:

OPTICAL SCAN

3 reflective objects detected

Candidate 1  ●  
Candidate 2  ●  
Candidate 3  ●

Move closer for inspection

This is enough for a first working prototype.

---

# **6\. Improved optical detection**

Once the simple version works, we can improve it.

Instead of one LED, several LEDs can be placed around the camera.

Example:

            LED  
              ●

       ●             ●

             📷

       ●             ●

              ●

Each LED can be activated individually.

For example:

Frame 1 → LED 1  
Frame 2 → LED 2  
Frame 3 → LED 3  
Frame 4 → LED 4  
Frame 5 → LED 5  
Frame 6 → LED 6

The system compares how each reflection behaves.

A lens may react differently from:

flat metal  
glass  
screw  
plastic  
mirror

The software can analyze:

brightness  
reflection size  
roundness  
position  
response to different LEDs  
response consistency

This can later allow:

Possible lens: 87%

rather than simply:

Bright reflection detected  
---

# **7\. Future optical wavelengths**

We discussed possibly testing different illumination types.

For example:

visible red  
850 nm infrared  
940 nm infrared  
white light

Different cameras and materials may respond differently.

The device could eventually compare multiple wavelengths.

Example:

Candidate A

Visible reflection    HIGH  
850 nm response       HIGH  
940 nm response       MEDIUM

Lens probability      89%

This is an experimental improvement, not required for the first prototype.

---

# **8\. V1 Feature 2   Wi-Fi scanning**

The device will scan nearby Wi-Fi networks/devices.

Possible information:

SSID  
MAC/BSSID where available  
signal strength  
channel  
frequency band  
manufacturer information where available

The system should NOT simply claim:

> “This Wi-Fi device is definitely a camera.”

Instead it can identify suspicious wireless activity.

Example:

Nearby wireless devices

Hotel\_WiFi          \-71 dBm  
SamsungTV           \-63 dBm  
Unknown Device      \-42 dBm

Unknown Device  
SIGNAL: STRONG

If the user walks toward the device:

\-77 dBm  
\-68 dBm  
\-55 dBm  
\-43 dBm  
\-32 dBm

the system can help localize it.

---

# **9\. Bluetooth scanning**

The same embedded hardware can search for Bluetooth Low Energy devices.

The device may display:

Device name  
MAC/address where available  
RSSI  
advertising data  
manufacturer data

Example:

BLE SCAN

Apple Watch      \-71 dBm  
Unknown BLE      \-36 dBm  
TV Remote        \-80 dBm

Again, an unknown Bluetooth device is **not automatically a camera**.

It is simply another clue.

---

# **10\. Wireless localization**

Signal strength can be used as a rough directional/localization aid.

For example:

Weak  
\-82 dBm

↓ move closer

\-68 dBm

↓ move closer

\-51 dBm

↓ move closer

\-37 dBm

The interface could display:

SIGNAL

████████░░

STRONG

This gives the device an easy interactive demonstration.

---

# **11\. V1 user interface**

The first prototype could have:

CAMGUARD 360

\[ Optical Scan \]

\[ Wireless Scan \]

\[ Bluetooth Scan \]

\[ Scan History \]

\[ Settings \]

Optical result:

OPTICAL SCAN

Potential reflective object

Confidence:  
███████░░░

\[Inspect\]  
\[Ignore\]

Wireless result:

WIRELESS SCAN

Unknown Device

Signal:  
█████████░

Very Strong  
---

# **12\. V1 hardware concept**

The simplest architecture could be:

                  CAMGUARD V1

                     Processor  
                        │  
           ┌────────────┴────────────┐  
           │                         │  
           ▼                         ▼  
     Optical module           Wireless module  
      Camera \+ LEDs             Wi-Fi/BLE  
           │                         │  
           └────────────┬────────────┘  
                        │  
                     Display  
                        │  
                     Battery

Possible components:

Main processor  
Raspberry Pi / similar SBC

Controller  
ESP32-S3

Optical camera  
Pi Camera / USB camera / NoIR camera

LEDs  
red / IR / white LEDs

LED driver/MOSFET circuit

LCD/TFT display

Li-ion/LiPo battery

USB-C charging

Power management

3D-printed enclosure

The exact components will be finalized after testing and discussing availability with the laboratory.

---

# **13\. Why use ESP32**

ESP32 can handle:

Wi-Fi scanning  
BLE scanning  
LED control  
buttons  
sensor communication  
real-time tasks

It is inexpensive and appropriate for an embedded project.

---

# **14\. Why possibly use Raspberry Pi**

Raspberry Pi can handle the heavier operations:

camera processing  
OpenCV  
image analysis  
graphical interface  
SDR processing later  
data logging  
advanced algorithms

Therefore the eventual architecture could contain both:

ESP32  
↓  
real-time hardware control

Raspberry Pi  
↓  
high-level analysis

However, if V1 can be completed using only one processor, we can simplify it.

---

# **15\. VERSION 2**

## **Automatic optical analysis**

Once V1 works reliably, the next improvement is automatic image processing.

The camera captures an image:

image  
 ↓  
brightness detection  
 ↓  
candidate extraction  
 ↓  
shape analysis  
 ↓  
reflection analysis  
 ↓  
possible lens

The screen could place a bounding box around the object.

Example:

┌──────────────────────────────┐  
│                              │  
│       Digital clock          │  
│                              │  
│          ┌────┐              │  
│          │ ●  │              │  
│          └────┘              │  
│                              │  
│ POSSIBLE CAMERA LENS         │  
└──────────────────────────────┘  
---

# **16\. False-positive reduction**

One of the long-term challenges is differentiating:

camera lens

from:

screws  
mirrors  
glass  
LEDs  
metal  
jewelry  
plastic reflections  
TV surfaces

Possible features for classification:

reflection intensity  
shape  
size  
symmetry  
multi-angle response  
IR response  
visible-light response  
location consistency  
temporal consistency

Later this could use:

rule-based classification

or:

machine learning

depending on time.

---

# **17\. VERSION 3**

## **General RF detection**

Wi-Fi and Bluetooth scanning only detect standard wireless protocols.

Some cameras might transmit using other RF technologies.

Therefore a later version can include an SDR receiver.

Architecture:

Antenna  
   ↓  
SDR  
   ↓  
Spectrum analysis  
   ↓  
Signal detection

The user could view:

Frequency activity

900 MHz     ███  
1.2 GHz     █  
2.4 GHz     ███████  
5.8 GHz     ████

The system could search for unusual RF sources.

---

# **18\. SDR**

A Software Defined Radio can convert RF signals into digital data that software can analyze.

Possible prototype hardware:

RTL-SDR class receiver

Later:

HackRF  
USRP  
other wideband SDR

depending on cost and laboratory equipment.

This feature is **not required for V1**.

---

# **19\. Directional RF localization**

A directional antenna could help identify where an RF source is located.

Example:

← weak

↑ medium

→ strong

The user rotates the device.

CamGuard measures signal strength.

Eventually it could display:

Likely direction

      ↗

This could apply to both general wireless detection and advanced EM detection.

---

# **20\. VERSION 4**

# **Offline SD-card camera detection**

This is one of the most interesting long-term features of CamGuard.

Consider this camera:

Camera  
 │  
 ├── sensor  
 ├── processor  
 ├── encoder  
 └── microSD

Wi-Fi OFF  
Bluetooth OFF  
Internet OFF

Traditional wireless scanning sees nothing.

However, internally the camera is still performing:

image capture  
↓  
image processing  
↓  
video encoding  
↓  
memory operations  
↓  
SD-card writing

Electronic activity can produce electromagnetic emissions.

We discussed studying whether these emissions can be detected using SDR hardware.

---

# **21\. EM side-channel detection**

The future EM system would consist of:

Camera being tested  
       ↓  
electromagnetic leakage  
       ↓  
directional/wideband antenna  
       ↓  
SDR receiver  
       ↓  
signal processing  
       ↓  
camera-likelihood analysis

However, simply detecting an electromagnetic signal is not enough.

There may be:

routers  
phones  
chargers  
TVs  
computers  
power supplies

nearby.

Therefore we need a way to determine whether the signal actually reacts like a camera.

---

# **22\. Active visual stimulation**

This is the important advanced idea.

CamGuard intentionally changes the scene using a controlled light source.

Example:

FLASH  
↓  
room becomes bright  
↓  
camera image changes  
↓  
video encoder activity changes  
↓  
memory activity changes  
↓  
EM emission may change

CamGuard simultaneously records the EM signal.

Then it repeats:

FLASH 1  
FLASH 2  
FLASH 3  
FLASH 4

If the same EM source repeatedly responds at the same time:

visual stimulus  
        ↕  
EM response

that becomes stronger evidence of an actively recording camera.

---

# **23\. Better stimulus pattern**

Instead of simply:

ON OFF ON OFF

CamGuard could use a known pseudo-random light pattern.

Example:

1 0 1 1 0 0 1 0 1 0 0 1

Then compare the measured EM activity against the exact stimulus sequence.

This could reduce accidental correlation with other electronics.

---

# **24\. EM signal-processing pipeline**

A future software pipeline might look like:

SDR samples  
↓  
frequency filtering  
↓  
FFT / spectral analysis  
↓  
candidate-frequency detection  
↓  
time-domain extraction  
↓  
stimulus synchronization  
↓  
correlation analysis  
↓  
confidence score

More advanced processing could eventually include:

wavelets  
feature extraction  
statistical classification  
TinyML

if needed.

---

# **25\. Offline-camera localization**

Once a camera-like EM signal is found, a directional antenna could help find it.

Example:

Signal strength

35%  
↓  
58%  
↓  
73%  
↓  
91%

The user follows the increasing signal.

The interface might display:

CAMERA-LIKE EM SOURCE

Signal  
█████████░

Move →  
---

# **26\. VERSION 5**

## **Thermal detection**

A powered camera generates heat from:

image sensor  
processor  
video encoder  
memory  
voltage regulator

Therefore a thermal sensor may provide another detection channel.

Possible sensors discussed:

MLX90640  
32 × 24 thermal array

or a more capable thermal camera such as:

FLIR Lepton

A thermal view could show:

clock  
normal area  
normal area  
HOT SPOT ← suspicious electronics

But heat alone does not prove that something is a camera.

It could be:

power supply  
router  
charger  
sensor  
microcontroller

Therefore thermal sensing should be treated as supporting evidence.

---

# **27\. VERSION 6**

# **Sensor fusion**

This is the eventual core intelligence of CamGuard 360\.

Each subsystem produces evidence.

Example:

Optical          0.91  
Wi-Fi            0.00  
Bluetooth        0.00  
RF               0.22  
EM correlation   0.94  
Thermal          0.71

The fusion algorithm combines them.

Result:

CAMERA LIKELY

Confidence:  
93%

Possible type:  
Offline/local-storage recording camera

Another example:

Optical          0.15  
Wi-Fi            0.91  
EM               0.70  
Thermal          0.76

Possible type:  
Wireless camera

Another:

Optical          0.10  
Wi-Fi            0.00  
EM               0.03  
Thermal          0.78

Likely:  
Electronic device  
Insufficient evidence of camera

This could greatly reduce false alarms.

---

# **28\. Camera-type estimation**

Eventually CamGuard could classify suspected devices into categories.

For example:

Wireless camera

or:

Offline/local-storage camera

or:

Possible camera lens  
Recording status unknown

or:

Electronic device detected  
No strong camera evidence

This is more useful than simply making a beeping sound.

---

# **29\. Confidence scoring**

The final system could use levels such as:

LOW  
MEDIUM  
HIGH  
VERY HIGH

Example:

CAMERA SUSPECTED

Confidence  
█████████░  
91%

Evidence

Optical        ✓  
Wi-Fi          ✗  
Bluetooth      ✗  
EM response    ✓  
Thermal        ✓  
---

# **30\. Guided room scanning**

CamGuard could guide the user through scanning a room.

Example:

ROOM SCAN

Progress:  
██████░░░░ 60%

Move slowly →

It could remember scanned directions/areas.

Potential future scanning workflow:

Stage 1  
Wireless scan

Stage 2  
Optical scan

Stage 3  
Deep RF/EM scan

Stage 4  
Suspicious-object inspection  
---

# **31\. Quick Scan vs Deep Scan**

A future interface could provide two modes.

### **Quick Scan**

Uses:

Wi-Fi  
Bluetooth  
optical reflection

Advantages:

fast  
low power  
simple

### **Deep Scan**

Adds:

SDR analysis  
active stimulus  
EM correlation  
thermal  
sensor fusion

Advantages:

better detection coverage

but consumes more power and time.

---

# **32\. Detection history**

The device could keep records such as:

Scan 001  
14:32

Optical candidates: 2  
Wireless candidates: 4  
Strongest unknown RF: \-42 dBm  
Final result: Suspicious

Data could be stored in:

CSV  
JSON

for testing and research.

---

# **33\. Developer/research mode**

For university experiments we can provide a technical mode.

Example:

DEVELOPER MODE

Raw optical frames  
Raw SDR spectrum  
FFT plot  
RSSI  
Correlation coefficient  
Sensor readings  
Timestamps

This will be useful during defense because we can show the professor the **processed data**, not just a final warning.

---

# **34\. Consumer mode**

The eventual product should hide most technical complexity.

Example:

CAMGUARD

\[SCAN ROOM\]

Then:

Scanning...

Then:

⚠ Possible camera found

Direction →  
Confidence HIGH

\[Inspect\]

The underlying system can be technically complicated while remaining easy to use.

---

# **35\. Battery system**

The final device should be portable.

Potential system:

Li-ion / LiPo battery  
↓  
battery-management board  
↓  
voltage regulation  
↓  
processor  
SDR  
camera  
display  
LEDs

Future modes could manage power.

Example:

Quick Scan  
low power

Deep Scan  
high power  
---

# **36\. Enclosure**

The final semester prototype should ideally have a 3D-printed enclosure.

Possible physical layout:

FRONT

 ┌──────────────────┐  
 │   ○ ○ ○ ○ ○      │  
 │ ○    CAMERA   ○   │  
 │   ○ ○ ○ ○ ○      │  
 │                  │  
 │     DISPLAY      │  
 │                  │  
 │    \[SCAN\]        │  
 └──────────────────┘

TOP/SIDE

directional antenna  
USB-C  
power switch

The goal is for it to eventually look like an actual portable instrument rather than a breadboard.

---

# **37\. Possible complete future hardware**

Long-term CamGuard could contain:

Main processor  
Raspberry Pi / Compute Module / custom SoC

Real-time MCU  
ESP32-S3 / STM32 / RP2040

Optical camera  
IR-sensitive digital camera

LED array  
visible \+ NIR

High-power white stimulus LEDs

Wi-Fi/Bluetooth radio

SDR receiver

Broadband antenna

Directional antenna

Thermal camera

LCD touchscreen

Battery

USB-C charging

Power-management circuitry

microSD storage

3D-printed/custom enclosure

Not all of this should be purchased for V1.

---

# **38\. Future NLJD concept**

We also discussed one very advanced method:

**Non-Linear Junction Detection   NLJD.**

Unlike passive RF detection, NLJD actively transmits RF energy and looks for harmonic responses from semiconductor junctions.

In principle this can detect electronics even when they are powered off.

Potential benefit:

camera OFF  
battery removed  
wireless OFF

could still contain semiconductor electronics.

However, this is **not planned for the semester prototype**.

Reasons:

special RF transmitter required  
harmonic receivers required  
antenna design  
RF isolation  
large number of false positives  
regulatory concerns  
higher cost  
much more complex electronics

This is a possible future research project by itself.

---

# **39\. What CamGuard cannot guarantee**

This limitation must always remain clear.

CamGuard should **never claim:**

> “Detects every hidden camera.”

Different cameras may defeat different methods.

Examples:

### **Completely powered-off camera**

EM:

no activity

Thermal:

no heat

RF:

no transmission

Optical could still work if the lens is visible.

---

### **Heavily shielded recording camera**

EM leakage may be greatly reduced.

---

### **Deeply concealed lens**

Optical reflection may not reach the detector.

---

### **Sleeping motion-triggered camera**

It may produce little processing activity until triggered.

---

### **Low-power architecture**

Some cameras may not produce the same useful EM patterns as other designs.

Therefore the system should combine evidence rather than promise universal detection.

---

# **40\. Main research question**

The long-term research question is:

> **Can an affordable portable embedded device combine multiple physical detection methods to provide broader and more reliable hidden-camera detection than simple consumer RF or optical detectors?**

A more specific future research question is:

> **Can an inexpensive embedded SDR system identify an actively recording offline camera by correlating controlled visual stimulation with electromagnetic side-channel emissions?**

---

# **41\. Semester V1 objective**

For the current semester, however, we keep the objective much simpler:

> **Develop a portable hidden-camera detector prototype using optical lens-reflection detection and Wi-Fi/Bluetooth scanning.**

Everything else is an extension.

That distinction is extremely important.

---

# **42\. Semester prototype success criteria**

The prototype should be considered successful if we can demonstrate something like:

### **Test environment**

Hide:

1 Wi-Fi camera  
1 camera with visible lens  
1 router  
1 phone  
1 charger  
several reflective objects

### **Wireless demonstration**

CamGuard scans:

5 wireless devices

and shows signal strength.

As we approach the camera:

\-75  
\-62  
\-51  
\-40  
\-31 dBm

### **Optical demonstration**

Point CamGuard around the room.

The device detects a suspicious lens reflection.

Display:

POSSIBLE CAMERA LENS

That's enough for a working first prototype.

---

# **43\. Advanced demonstration later**

If we successfully develop the EM feature, the advanced demo becomes significantly more impressive.

Hide:

Camera A  
Wi-Fi streaming

Camera B  
microSD only  
Wi-Fi OFF

Device C  
router

Device D  
charger

CamGuard scans.

Wireless mode finds Camera A.

Optical mode may find its lens.

Then Camera B is tested.

There is:

no Wi-Fi  
no Bluetooth

CamGuard performs:

EM scan  
↓  
candidate frequency  
↓  
active visual stimulus  
↓  
FLASH  
FLASH  
FLASH  
FLASH  
↓  
repeatable EM response

Result:

⚠ CAMERA-LIKE RECORDING ACTIVITY

Wireless transmission:  
NONE

EM correlation:  
HIGH

Possible:  
OFFLINE RECORDING CAMERA

That remains our eventual **wow-factor demonstration**.

---

# **44\. Development roadmap**

I would preserve the project roadmap like this:

CAMGUARD 360 ROADMAP

V1  
Optical reflection detection  
\+  
Wi-Fi/BLE scanning

        ↓

V2  
Automatic optical image processing  
\+  
multi-angle LEDs  
\+  
false-positive reduction

        ↓

V3  
General SDR/RF analysis  
\+  
direction finding

        ↓

V4  
Offline camera EM side-channel detection  
\+  
active visual stimulation

        ↓

V5  
Thermal detection

        ↓

V6  
Sensor fusion  
\+  
camera classification  
\+  
confidence scoring

        ↓

Future Research  
NLJD / powered-off electronics detection  
---

# **45\. What belongs in our current Technical Specification**

Our **current mandatory prototype** should contain:

Optical detection  
Wi-Fi/Bluetooth detection  
basic UI  
data display/logging  
portable embedded hardware

Our ТЗ should NOT currently require:

EM offline-camera detection  
thermal imaging  
NLJD  
advanced AI  
perfect lens classification

Those should be presented as:

> **Перспективы дальнейшего развития**

or:

> **Additional functionality if time permits**

This protects the semester project from unnecessary risk.

---

# **46\. What makes the project technically valuable even in V1**

Although V1 is deliberately easier, it is still an Embedded Systems project because we have:

microcontroller/SBC  
camera  
LED control  
wireless scanning  
hardware interfaces  
display  
embedded software  
power supply  
physical enclosure  
signal measurements  
real-time user interaction

Then future versions progressively add:

computer vision  
DSP  
SDR  
RF engineering  
EM side channels  
thermal sensing  
sensor fusion  
TinyML

So there is a clear path from a manageable student prototype to a much more sophisticated engineering system.

---

# **47\. Final one-sentence project description**

For general presentation:

> **CamGuard 360 is a portable embedded hidden-camera detection system that combines multiple sensing technologies to identify both conventional wireless cameras and, in future versions, difficult-to-detect offline recording cameras.**

For the current semester:

> **The first prototype of CamGuard 360 will implement optical camera-lens detection and Wi-Fi/Bluetooth scanning, while more advanced detection methods will be developed as future extensions.**

---

# **48\. The idea we should preserve**

The most important thing not to lose while simplifying the semester prototype is this:

**CamGuard 360 is not supposed to remain a Wi-Fi scanner with LEDs.**

That is merely **V1**.

The complete idea is:

Optical  
\+  
Wi-Fi/Bluetooth  
\+  
RF  
\+  
EM side-channel  
\+  
active visual challenge  
\+  
direction finding  
\+  
thermal  
\+  
sensor fusion  
\+  
intelligent classification

with the difficult **offline/local-storage camera problem** remaining one of the main long-term research targets.

So our strategy is:

> **Easy features first → working prototype → improve it → attempt the research feature → eventually combine everything into CamGuard 360\.**

This gives us both a project we can realistically finish this semester and a much larger idea we can continue afterward.
