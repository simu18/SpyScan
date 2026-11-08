/* spyscan_case.scad — Parametric Handheld Enclosure for SpyScan
 *
 * Designed for 3D printing on Bambu Lab (PLA or ABS).
 * Houses:
 *   - ESP32-S3 Camera Board (57 x 28 x 1.6 mm) + OV3660 camera
 *   - ILI9341 2.4" Display Module (76 x 43 x 5 mm, glass 60 x 40 mm)
 *   - IP5306 Power Management Module (25 x 20 x 12 mm)
 *   - 1S Li-Po Battery (50 x 40 x 6 mm)
 *   - 6-LED On-Axis Ring + 2-LED Off-Axis Pair + 5V IR Module
 *   - 3× Tactile Navigation Buttons (UP, SEL, BACK)
 *   - Top SMA Antenna Port (CC1101 Sub-GHz) + Slide Switch
 *   - Bottom USB-C Ports (Charging + Programming)
 */

$fn = 40;

// Overall Enclosure Dimensions (mm)
case_w = 78.0;      // Width
case_l = 145.0;     // Length
case_h = 32.0;      // Total Height (16mm Front + 16mm Back)
wall_t = 2.4;       // Solid Wall Thickness
corner_r = 5.0;     // Corner Fillet Radius

// Component Dimensions + Clearances (mm)
lcd_pcb_l = 77.5;   // 76 + 1.5 clearance
lcd_pcb_w = 44.5;   // 43 + 1.5 clearance
lcd_pcb_h = 6.5;
lcd_glass_l = 60.5; // Visible window
lcd_glass_w = 40.5;

cam_pcb_l = 58.5;   // 57 + 1.5 clearance
cam_pcb_w = 29.5;   // 28 + 1.5 clearance

lipo_l = 51.5;      // 50 + 1.5 clearance
lipo_w = 41.5;      // 40 + 1.5 clearance
lipo_h = 7.5;       // 6 + 1.5 clearance

ip5306_l = 26.5;    // 25 + 1.5 clearance
ip5306_w = 21.5;    // 20 + 1.5 clearance
ip5306_h = 13.5;

// Optical Geometry
lens_hole_d = 12.0;       // Base lens aperture
lens_cone_d = 20.0;       // Front cone opening (clears 65° FOV without clipping)
cone_depth = 8.0;         // Recess depth
led_hole_d = 5.3;         // 5mm LED clearance hole
led_ring_d = 27.5;        // LED ring circle (prevents bite-in into cone)

btn_hole_d = 6.5;         // 6mm button cap clearance
sma_hole_d = 6.5;         // SMA connector
screw_d = 3.2;            // M3 screw pass-through
insert_d = 4.2;           // M3 heat-set insert hole

// Helper: Rounded Box
module rounded_box(w, l, h, r) {
    hull() {
        translate([r, r, 0]) cylinder(h=h, r=r);
        translate([w - r, r, 0]) cylinder(h=h, r=r);
        translate([r, l - r, 0]) cylinder(h=h, r=r);
        translate([w - r, l - r, 0]) cylinder(h=h, r=r);
    }
}

// ------------------------------------------------------------- FRONT SHELL
module front_shell() {
    difference() {
        // Outer Shell
        rounded_box(case_w, case_l, case_h / 2, corner_r);

        // Internal Cavity
        translate([wall_t, wall_t, wall_t])
            rounded_box(case_w - 2 * wall_t, case_l - 2 * wall_t, (case_h / 2) + 1, corner_r - 1.5);

        // 1. LCD Visible Window (Glass 60.5 x 40.5)
        translate([(case_w - lcd_glass_w) / 2, 14.0, -1])
            cube([lcd_glass_w, lcd_glass_l, wall_t + 2]);

        // LCD Internal Step / Shelf for PCB
        translate([(case_w - lcd_pcb_w) / 2, 14.0 - ((lcd_pcb_l - lcd_glass_l) / 2), wall_t - 1.2])
            cube([lcd_pcb_w, lcd_pcb_l, 3.0]);

        // 2. Camera Aperture & 65° Recess Cone
        // Camera Center at X = 46.0 mm, Y = 94.0 mm
        translate([46.0, 94.0, -1])
            cylinder(h = wall_t + 2, d = lens_hole_d);
        
        // Chamfered / Conical Optical FOV Relief
        translate([46.0, 94.0, -0.1])
            cylinder(h = cone_depth, d1 = lens_cone_d, d2 = lens_hole_d);

        // 3. On-Axis 6-LED Ring (Ø5.3mm on Ø27.5mm circle)
        for (a = [0:60:300]) {
            translate([46.0 + (led_ring_d / 2) * cos(a), 94.0 + (led_ring_d / 2) * sin(a), -1])
                cylinder(h = wall_t + 2, d = led_hole_d);
        }

        // 4. Off-Axis 2-LED Pair (38 mm to the left)
        translate([14.0, 89.0, -1]) cylinder(h = wall_t + 2, d = led_hole_d);
        translate([14.0, 99.0, -1]) cylinder(h = wall_t + 2, d = led_hole_d);

        // 5. 5V Hardware IR Window
        translate([68.0, 94.0, -1]) cylinder(h = wall_t + 2, d = 6.5);

        // 6. 3× Navigation Buttons (UP, SEL, BACK)
        translate([20.0, 126.0, -1]) cylinder(h = wall_t + 2, d = btn_hole_d);
        translate([39.0, 126.0, -1]) cylinder(h = wall_t + 2, d = btn_hole_d);
        translate([58.0, 126.0, -1]) cylinder(h = wall_t + 2, d = btn_hole_d);

        // 7. Top Cutouts: SMA Antenna & Power Switch
        translate([60.0, case_l - 1, (case_h / 4)])
            rotate([-90, 0, 0]) cylinder(h = wall_t + 2, d = sma_hole_d);
        translate([16.0, case_l - 1, (case_h / 4) - 2])
            cube([9.0, wall_t + 2, 4.0]);

        // 8. Bottom Cutouts: USB-C Ports
        translate([20.0, -1, (case_h / 4) - 2])
            cube([9.5, wall_t + 2, 3.5]);
        translate([case_w - 29.5, -1, (case_h / 4) - 2])
            cube([9.5, wall_t + 2, 3.5]);
    }

    // 4× M3 Heat-Set Insert Corner Bosses
    boss_r = 4.0;
    boss_h = (case_h / 2);
    translate([wall_t + 2, wall_t + 2, 0])
        difference() {
            cylinder(h = boss_h, r = boss_r);
            translate([0, 0, boss_h - 5.0]) cylinder(h = 5.5, d = insert_d);
        }
    translate([case_w - wall_t - 2, wall_t + 2, 0])
        difference() {
            cylinder(h = boss_h, r = boss_r);
            translate([0, 0, boss_h - 5.0]) cylinder(h = 5.5, d = insert_d);
        }
    translate([wall_t + 2, case_l - wall_t - 2, 0])
        difference() {
            cylinder(h = boss_h, r = boss_r);
            translate([0, 0, boss_h - 5.0]) cylinder(h = 5.5, d = insert_d);
        }
    translate([case_w - wall_t - 2, case_l - wall_t - 2, 0])
        difference() {
            cylinder(h = boss_h, r = boss_r);
            translate([0, 0, boss_h - 5.0]) cylinder(h = 5.5, d = insert_d);
        }
}

// ------------------------------------------------------------- BACK SHELL
module back_shell() {
    difference() {
        // Outer Shell
        rounded_box(case_w, case_l, case_h / 2, corner_r);

        // Internal Cavity
        translate([wall_t, wall_t, wall_t])
            rounded_box(case_w - 2 * wall_t, case_l - 2 * wall_t, (case_h / 2) + 1, corner_r - 1.5);

        // 4× M3 Screw Pass-Through Holes & Counterbores
        translate([wall_t + 2, wall_t + 2, -1]) {
            cylinder(h = wall_t + 2, d = screw_d);
            cylinder(h = 2.0, d = 6.2); // M3 screw head counterbore
        }
        translate([case_w - wall_t - 2, wall_t + 2, -1]) {
            cylinder(h = wall_t + 2, d = screw_d);
            cylinder(h = 2.0, d = 6.2);
        }
        translate([wall_t + 2, case_l - wall_t - 2, -1]) {
            cylinder(h = wall_t + 2, d = screw_d);
            cylinder(h = 2.0, d = 6.2);
        }
        translate([case_w - wall_t - 2, case_l - wall_t - 2, -1]) {
            cylinder(h = wall_t + 2, d = screw_d);
            cylinder(h = 2.0, d = 6.2);
        }
    }

    // Battery Bay Retaining Ribs
    translate([(case_w - lipo_w) / 2, 20.0, wall_t]) {
        cube([lipo_w, 1.6, lipo_h]);
        translate([0, lipo_l, 0]) cube([lipo_w, 1.6, lipo_h]);
    }

    // IP5306 Module Retaining Ribs
    translate([12.0, 80.0, wall_t]) {
        cube([ip5306_w, 1.6, 6.0]);
        translate([0, ip5306_l, 0]) cube([ip5306_w, 1.6, 6.0]);
    }
}

// Render Front Shell (or translate for side-by-side export)
front_shell();
translate([case_w + 15, 0, 0]) back_shell();
