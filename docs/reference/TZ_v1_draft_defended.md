# **Technical Specification for the CamGuard 360 Project**

## **General Information**

### **1\. Project Goal**

The goal of the project is to develop a portable embedded device called **CamGuard 360**, designed to search for possible hidden cameras in indoor environments using optical lens detection and analysis of available Wi-Fi and Bluetooth wireless devices.

The main objective of the current project version is to create a simple, reliable, and fully functional prototype that can be demonstrated as part of the course project.

The V1 prototype is planned to implement two primary methods:

1. **Optical detection of reflections from a possible hidden-camera lens using red LEDs and an integrated camera.**  
2. **Sequential scanning of available Wi-Fi and Bluetooth devices with signal-strength indication.**

The main goal of V1 is not to automatically classify every detected object as a hidden camera with complete accuracy.

The system shall detect suspicious indicators and provide information that allows the user to perform further inspection.

More advanced methods, such as advanced image processing, SDR-based RF analysis, electromagnetic side-channel detection, thermal detection, and multi-sensor data fusion, are considered future development directions.

### **2\. Development Team**

**Developer:** Rahaman Md Afifur   
**Group:** P3330

The project is developed individually.

The developer is responsible for:

* development of the hardware part of the device;  
* development of software for the ESP32-S3;  
* implementation of optical scanning;  
* implementation of Wi-Fi and Bluetooth scanning;  
* development of the user interface;  
* assembly of the prototype;  
* conducting tests;  
* analysis of results;  
* preparation of project documentation and demonstration.

# **Technical Requirements**

## **1\. Functional Requirements**

### **1.1. General Functions**

The device shall provide two primary operating modes:

* optical scanning;  
* Wi-Fi/Bluetooth wireless scanning.

Switching between operating modes shall be performed using physical buttons.

The main information shall be displayed on a small integrated non-touch TFT or OLED display.

### **1.2. Optical Detection**

The optical module shall use an integrated camera and visible red LEDs.

The red LEDs shall illuminate the inspected area and create visible reflections from optical surfaces.

Infrared LEDs are not included in the mandatory V1 implementation.

The software shall capture camera frames and search for bright regions exceeding a defined brightness threshold.

Detected bright regions shall be treated as possible candidates for further inspection.

In V1, the system is not required to automatically determine whether a detected object is specifically a camera lens.

To reduce random false detections, the system shall verify whether a detected object persists across several consecutive frames.

A bright region shall be considered a persistent candidate if it remains approximately in the same position for several consecutive frames.

The result shall be presented to the user as the number of detected suspicious regions and/or their location in the image.

Example result:

OPTICAL SCAN

Suspicious reflections: 2

Candidate 1  
Candidate 2

Move closer for inspection

### **1.3. Wi-Fi Scanning**

The ESP32-S3 shall perform active scanning of available Wi-Fi networks.

For each detected network, available parameters shall be displayed, such as:

* SSID;  
* BSSID/MAC address, when available;  
* RSSI signal level;  
* wireless channel.

The user shall be able to observe signal-strength changes while moving the device.

This may be used for approximate localization of the signal source.

For example:

Unknown Wi-Fi

RSSI: \-72 dBm  
       ↓  
RSSI: \-58 dBm  
       ↓  
RSSI: \-41 dBm

The system shall not automatically classify every detected Wi-Fi device as a hidden camera.

### **1.4. Wi-Fi Scanning Limitation**

Standard ESP32-S3 Wi-Fi scanning primarily detects devices operating as access points and transmitting beacon frames.

A camera that is already connected as a client to an existing Wi-Fi network and does not create its own access point may not appear directly in a standard Wi-Fi scan.

Therefore, the V1 demonstration is planned to use a test Wi-Fi camera capable of creating its own access point.

Promiscuous-mode Wi-Fi packet capture and analysis of client devices are considered future project extensions.

### **1.5. Bluetooth Scanning**

The ESP32-S3 shall support scanning for Bluetooth Low Energy devices.

For detected devices, available information shall be displayed, including:

* device name, if transmitted;  
* device address, if available;  
* RSSI;  
* available advertising information.

RSSI may be used to approximately estimate whether the user is moving closer to a detected device.

### **1.6. Sequential Wi-Fi and Bluetooth Operation**

Wi-Fi and Bluetooth share the ESP32-S3 2.4 GHz radio subsystem.

Therefore, in V1 Wi-Fi and Bluetooth scanning shall not operate continuously at the same time.

The system shall use a sequential scanning mode:

Wi-Fi scan  
↓  
display results  
↓  
BLE scan  
↓  
display results

Alternatively, the user may manually select the required scan type.

---

### **1.7. User Interface**

The user interface shall be kept as simple as possible.

Two or three physical buttons shall be used for control.

For example:

Button 1 — Mode / Menu  
Button 2 — Start / Select  
Button 3 — Back

The main screen may contain:

CAMGUARD 360

\> Optical Scan  
  Wi-Fi Scan  
  BLE Scan

A touchscreen is not required for V1.

---

### **1.8. Result Storage**

In V1, saving scan results is an optional rather than mandatory function.

If development time allows, the following may be stored:

* scan type;  
* scan time;  
* number of optical candidates;  
* RSSI values of detected wireless devices.

---

## **2\. Reliability Requirements**

The device shall provide stable operation of the main functions without requiring frequent reboots.

The prototype shall operate continuously for at least 30 minutes in demonstration mode.

If the camera, display, or another peripheral device is unavailable, the software shall not enter an endless reboot loop.

Failure of one operating mode shall not completely prevent use of the remaining available functions.

---

## **3\. Operating Conditions**

The device is intended for indoor use.

Expected operating conditions are:

* temperature from \+10°C to \+35°C;  
* no direct exposure to water;  
* normal indoor lighting;  
* absence of strong mechanical shock and vibration.

The effectiveness of the optical method depends on lens visibility and the angle between the light source, target lens, and CamGuard camera.

Strong reflections from glass, metallic surfaces, and other shiny objects may generate additional candidates.

Wireless scanning is intended to identify available radio devices and does not by itself prove the presence of a hidden camera.

---

## **4\. Hardware Composition and Technical Parameters**

V1 shall use **one primary ESP32-S3 microcontroller**.

A separate Raspberry Pi shall not be used in V1.

This reduces:

* the number of hardware components;  
* power consumption;  
* system boot time;  
* the number of software interfaces between different processors;  
* debugging complexity.

### **Main V1 Components**

**1\. ESP32-S3 board with camera support**

Used as the main computing and control module.

It shall perform:

* camera control;  
* basic image processing;  
* LED control;  
* Wi-Fi scanning;  
* BLE scanning;  
* display control;  
* button input processing.

**2\. ESP32-S3-compatible camera**

For example, an OV2640-class module or equivalent.

A specially modified NoIR camera is not required for V1.

**3\. Red LEDs**

Used for optical detection of reflections from possible camera lenses.

The exact number depends on the enclosure design, approximately 4–8 LEDs.

**4\. Transistor/MOSFET or simple LED driver**

Used to control the LEDs from the ESP32-S3.

**5\. Small non-touch TFT or OLED display**

Used to display:

* operating mode;  
* number of optical candidates;  
* list of Wi-Fi/BLE devices;  
* RSSI level;  
* system status.

**6\. Two or three push buttons**

Used to control the user interface.

**7\. Li-Po battery**

Provides portable battery-powered operation.

**8\. USB-C charging module of the TP4056 class or equivalent**

Used for battery charging.

**9\. Enclosure**

The enclosure is planned to be manufactured using 3D printing.

## **5\. Information and Software Compatibility Requirements**

The software shall be developed for the ESP32-S3.

The following development environments may be used:

* ESP-IDF;  
* Arduino Framework for ESP32;  
* PlatformIO.

The primary programming language shall be C/C++.

The following interfaces may be used between the ESP32-S3 and peripheral devices:

* GPIO;  
* SPI;  
* I²C;  
* ESP32 camera interface.

The main device functions shall operate without connection to the Internet or a cloud service.

## **6\. Transportation and Storage Requirements**

The device shall be transported while powered off.

During transportation:

* protect the display and camera from mechanical damage;  
* prevent moisture exposure;  
* prevent battery short circuits;  
* avoid strong mechanical impacts.

The device shall be stored in a dry indoor environment at room temperature.

# **Documentation Requirements**

The following documentation shall be prepared as part of the project:

1. Technical Specification.  
2. Hardware block diagram.  
3. Connection diagram for the ESP32-S3, camera, display, LEDs, and buttons.  
4. Software architecture description.  
5. Project source code.  
6. Bill of materials.  
7. Short user manual.  
8. Testing procedure description.  
9. Test results.  
10. Description of device limitations.  
11. Project README in GitLab.  
12. Final project report.

# **Technical and Economic Indicators**

The primary goal of V1 is to create a low-cost prototype based on a single microcontroller.

The preliminary cost of a single educational prototype is estimated at approximately:

### **RUB 4,000–7,000**

The final amount shall be refined after the specific components are selected and the budget is approved by the instructor.

The main cost items are:

* ESP32-S3 camera board;  
* display;  
* battery and charging module;  
* LEDs and electronic components;  
* push buttons;  
* enclosure materials.

With mass production and the use of a custom PCB, the estimated manufacturing cost could potentially be reduced to approximately **RUB 2,000–4,000**, excluding certification, industrial enclosure development, logistics, and commercial margins.

The main advantage of the development is the combination of two detection approaches in one portable device:

* optical detection of suspicious reflections;  
* detection of available Wi-Fi and Bluetooth devices with signal-strength estimation.

Future versions may extend the device with more advanced detection methods without requiring a complete redesign of the project concept.

# **Development Stages and Schedule**

## **Stage 1\. Technical Specification and Design**

Tasks:

* approval of the Technical Specification;  
* creation of the GitLab repository;  
* selection of the ESP32-S3 board;  
* selection of the camera;  
* selection of the display;  
* preparation of the bill of materials and cost estimate;  
* creation of the initial hardware block diagram.

## **Stage 2\. Basic Hardware Assembly**

Tasks:

* camera connection;  
* display connection;  
* button connection;  
* red LED connection;  
* power-system testing.

**Deliverable:**  
A functioning ESP32-S3-based hardware prototype.

## **Stage 3\. Wi-Fi/BLE Scanning Implementation**

Tasks:

* Wi-Fi scanning;  
* display of SSID and RSSI;  
* BLE scanning;  
* display of BLE devices and RSSI;  
* implementation of sequential switching between Wi-Fi and BLE scans.

## **Stage 4\. Optical Module Implementation**

Tasks:

* red LED control;  
* camera frame acquisition;  
* bright-region detection;  
* candidate extraction;  
* persistence checking across consecutive frames.

## **Stage 5\. User Interface**

Tasks:

* menu development;  
* button control;  
* result display;  
* RSSI visualization;  
* error-message display.

## **Stage 6\. Enclosure and Integration**

Tasks:

* enclosure design;  
* 3D printing;  
* component installation;  
* battery integration.

## **Stage 7\. Testing and Final Preparation**

Tasks:

* optical-mode testing;  
* Wi-Fi/BLE testing;  
* false optical-candidate testing;  
* RSSI testing at different distances;  
* stability testing;  
* demonstration preparation;  
* final report and presentation preparation.

# **Control and Acceptance Procedure**

A functional CamGuard 360 prototype shall be presented for project acceptance.

## **Test 1\. Optical Detection**

A camera with an optically visible lens and several other reflective objects shall be placed in the test area.

The device shall:

* activate red illumination;  
* capture the image;  
* detect bright regions;  
* verify their persistence across consecutive frames;  
* display suspicious candidates.

## **Test 2\. Wi-Fi Scanning**

A Wi-Fi camera that creates its own access point shall be used in the test area.

CamGuard shall:

* detect the camera network;  
* display the SSID;  
* display RSSI;  
* show RSSI changes as the device moves closer to or farther from the source  
*   
    
  {Nb: Rs for sequaltial searching}. Real time op system . free FreeRTOS

## **Test 3\. Bluetooth Scanning**

One or more BLE devices shall be placed in the test environment.

CamGuard shall:

* perform a BLE scan;  
* detect the devices;  
* display available information;  
* display RSSI.

## **Test 4\. Stability**

The prototype shall operate continuously for at least 30 minutes without a critical failure or mandatory manual reboot.

# **Final Demonstration**

The final demonstration is planned to include:

* a Wi-Fi camera;  
* a camera with an optically visible lens;  
* several ordinary Wi-Fi/BLE devices;  
* several reflective objects that are not cameras.

The demonstration shall show:

1. CamGuard startup;  
2. mode selection using physical buttons;  
3. optical detection of suspicious reflections;  
4. Wi-Fi scanning;  
5. Bluetooth scanning;  
6. RSSI changes while moving the device;  
7. known system limitations.

# **Future Project Development**

After completion of V1, the following features may be implemented:

* automatic reflection classification;  
* multi-angle LED illumination;  
* infrared illumination;  
* advanced image processing;  
* Wi-Fi promiscuous mode and client-device analysis;  
* general RF analysis using SDR;  
* directional RF-source localization;  
* camera electromagnetic side-channel detection;  
* active visual camera stimulation and EM-signal correlation analysis;  
* detection of cameras recording locally to an SD card;  
* thermal detection;  
* multi-method sensor fusion;  
* automatic camera-confidence estimation.

# **References**

1. **GOST 19.201-78. Technical Specification. Requirements for Content and Formatting.**  
2. **Espressif Systems. ESP32-S3 Series Datasheet.**  
   ESP32-S3 microcontroller documentation.  
3. **Espressif Systems. ESP-IDF Programming Guide — Wi-Fi.**  
   Documentation for Wi-Fi operation on the ESP32-S3.  
4. **Espressif Systems. ESP-IDF Programming Guide — Bluetooth Low Energy.**  
   Documentation for BLE operation.  
5. **Embedded Systems course materials, project track.**

## **V1 Scope Summary**

The mandatory V1 architecture is:

               CAMGUARD 360 V1

                   ESP32-S3  
                      │  
        ┌─────────────┼─────────────┐  
        │             │             │  
        ▼           ▼          ▼  
     Camera       Wi-Fi/BLE      Display  
        │                         Buttons  
    Red LEDs  
        │  
Bright-reflection  
   detection  
        │  
Persistence check

The mandatory V1 implementation excludes Raspberry Pi, SDR, electromagnetic detection, thermal cameras, touchscreen control, IR LEDs, and complex automatic classification.

The first version implements two primary methods:

> **Optical detection of suspicious lens reflections and sequential Wi-Fi/Bluetooth scanning. The entire device is based on a single ESP32-S3. More advanced methods, including SDR and electromagnetic detection of offline cameras, are reserved for future development.**
