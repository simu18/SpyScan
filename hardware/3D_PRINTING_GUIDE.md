# SpyScan 3D Printing & Enclosure Guide (Bambu Lab)

**Printer Target:** Bambu Lab (X1-Carbon, P1S, P1P, A1 / Bambu Studio / OrcaSlicer)  
**Materials Supported:** PLA or ABS  
**Generated Files:**  
- [`hardware/spyscan_front_shell.stl`](spyscan_front_shell.stl) (Front bezel shell)  
- [`hardware/spyscan_back_shell.stl`](spyscan_back_shell.stl) (Back housing with battery bay)  
- [`hardware/spyscan_case.scad`](spyscan_case.scad) (Parametric source for OpenSCAD / FreeCAD)  

---

## 1. Component Verification & Dimension Check

Here is the dimension analysis comparing your specified components against standard manufacturing tolerances:

| Part | Spec (L × W × H, mm) | Clearance Added | Internal Pocket in CAD | Online / Reality Check |
|---|---|---|---|---|
| **ESP32-S3 Camera Board** | 57 × 28 × 1.6 (+5 back) | +1.5 mm | **58.5 × 29.5 × 8.0 mm** | Matches Freenove / AI-Thinker S3-CAM boards. USB-C port aligns with bottom edge. |
| **OV3660 Camera Sensor** | 8.5 × 8.5 × ~8 mm | Direct press-fit | **9.0 × 9.0 mm cradle** | Centered inside the front cone aperture. |
| **ILI9341 2.4" LCD** | 76 × 43 × ~5 (+7 header) | +1.5 mm | **77.5 × 44.5 × 12.0 mm** | Visible active glass is 49 × 37 mm; outer glass frame is **60.5 × 40.5 mm**. |
| **IP5306 Module** | 25 × 20 × ~12 mm | +1.5 mm | **26.5 × 21.5 × 13.5 mm** | Fits USB-C charging board; USB-C port sits flush with enclosure perimeter. |
| **Li-Po Battery** | 50 × 40 × 6 mm | +1.5 mm | **51.5 × 41.5 × 7.5 mm** | Fits standard 1000–1200 mAh 1S flat pouch cells (e.g., 103450 or 604050). |

---

## 2. Critical Geometric Finding: Lens Cone vs. LED Ring

> [!WARNING]
> **GEOMETRIC OVERLAP PREVENTED:**
> In your original draft, you specified:
> - LED ring circle: **Ø26.0 mm** ($R = 13.0\text{ mm}$), holes **Ø5.3 mm** ($r = 2.65\text{ mm}$).
>   $\implies$ Inner edge of LED holes is at $13.0 - 2.65 = \mathbf{10.35\text{ mm}}$ radius.
> - Outer cone opening: **Ø22.0 mm** ($R = \mathbf{11.0\text{ mm}}$).
> 
> Notice that **$11.0\text{ mm} > 10.35\text{ mm}$**! A Ø22 mm cone would cut directly into the 6 LED holes by **0.65 mm**, creating light bleed holes that flood the camera lens with strobe light and break optical retroreflection.
>
> **The Engineering Fix in our Model:**
> 1. We set the outer cone diameter to **Ø20.0 mm** ($R = 10.0\text{ mm}$). Over an 8 mm depth with a 65° FOV ($\tan(32.5^\circ) \approx 0.637$), an expansion of $5.1\text{ mm}$ is required ($R_{\text{needed}} = 4.25 + 5.1 = 9.35\text{ mm} \implies \varnothing 18.7\text{ mm}$).
> 2. We widened the LED ring circle slightly to **Ø27.5 mm** ($R = 13.75\text{ mm}$).
> 3. This guarantees a solid **$1.1\text{ mm}$ opaque light-shield wall** between the cone and the LEDs, preventing blinding lens flare.

---

## 3. Bambu Lab Slicing & Printing Profile

Import the `.stl` files directly into **Bambu Studio** or **OrcaSlicer**:

| Setting | Recommended Value (PLA) | Recommended Value (ABS) | Reason |
|---|---|---|---|
| **Layer Height** | **0.20 mm Standard** | **0.20 mm Standard** | Clean vertical walls and precise hole diameters. |
| **Wall Loops / Perimeters** | **4 loops (1.6 mm)** | **4 loops (1.6 mm)** | Ensures screw bosses and corners are 100% solid plastic. |
| **Top / Bottom Shells** | 5 top, 4 bottom | 5 top, 4 bottom | Prevents pillowing over hollow internal areas. |
| **Infill Density** | **25%** | **25%** | High rigidity under thumb pressure on buttons. |
| **Infill Pattern** | **Gyroid** | **Gyroid** | Isotropic strength in all directions. |
| **Nozzle Temperature** | 220 °C | 255 °C | Standard Bambu hotend temperatures. |
| **Bed Temperature** | 55–65 °C (Textured PEI) | 90–100 °C (Engineering Plate) | Strong first layer adhesion. |
| **Chamber / Door** | Open (or top lid vented) | Fully closed enclosure | Prevents ABS warping / corner curling. |
| **Supports** | **Tree (Auto)** | **Tree (Auto)** | Only needed for the USB-C slots and top SMA port overhang. |
| **Print Orientation** | **Flat face down on bed** | **Flat face down on bed** | Texture from PEI plate gives an authentic matte tactical finish on the front face. |

---

## 4. Hardware Assembly Bill of Materials (BOM)

| Item | Quantity | Purpose |
|---|---|---|
| **M3 × 4.0 mm Brass Heat-Set Inserts** | 4 pcs | Melted into the front shell corner bosses with a soldering iron (220 °C). |
| **M3 × 12 mm Socket Head Cap Screws** | 4 pcs | Inserted from the back shell to clamp the enclosure shut. |
| **6×6 mm Tactile Button Caps** | 3 pcs | Extension caps for UP, SELECT, BACK buttons. |
| **EVA Foam Sheet (1.0 mm)** | 1 strip | Placed beneath Li-Po battery to prevent rattling inside the bay. |
| **433 MHz Rubber Ducky Antenna** | 1 pc | Screws onto the top SMA connector for the Sub-1 GHz CC1101 transceiver. |

---

## 5. Assembly Order

1. **Install Heat-Set Inserts:** Using a soldering iron set to 220 °C, gently press the 4× M3 brass inserts flush into the front shell corner bosses.
2. **Mount LEDs & Camera:**
   - Slide the 6 On-Axis LEDs into the Ø5.3 mm holes around the cone.
   - Position the 2 Off-Axis LEDs and the 5V IR sensor module.
   - Mount the OV3660 camera module into its rear alignment cradle.
3. **Mount LCD:** Seat the ILI9341 display PCB into the internal perimeter shelf. Secure corners with a dab of silicone or 2 mm mounting screws.
4. **Wire Power & Battery:** Place the Li-Po battery and IP5306 module in their respective back shell retaining bays. Connect the main slide switch to the top slot.
5. **Close Enclosure:** Carefully route internal wires away from the center, align front and back shells, and tighten the 4× M3 screws.
