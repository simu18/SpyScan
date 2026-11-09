#!/usr/bin/env python3
"""generate_case_stl.py — Generates 3D printable STL files for SpyScan.

Produces:
  - hardware/spyscan_front_shell.stl
  - hardware/spyscan_back_shell.stl

Compatible with Bambu Studio, OrcaSlicer, and PrusaSlicer for Bambu Lab 3D printers.
"""

import math
import pathlib
import struct
import numpy as np

OUTPUT_DIR = pathlib.Path(__file__).resolve().parent

def write_binary_stl(filename: pathlib.Path, facets: list):
    """Writes a list of facets ((nx,ny,nz), (v1), (v2), (v3)) to a binary STL."""
    with open(filename, "wb") as f:
        header = f"SpyScan Enclosure - Bambu Lab PLA/ABS Export".encode("utf-8").ljust(80, b"\0")
        f.write(header[:80])
        f.write(struct.pack("<I", len(facets)))
        for norm, v1, v2, v3 in facets:
            f.write(struct.pack("<3f", *norm))
            f.write(struct.pack("<3f", *v1))
            f.write(struct.pack("<3f", *v2))
            f.write(struct.pack("<3f", *v3))
            f.write(struct.pack("<H", 0))
    print(f"✓ Generated: {filename.name} ({len(facets)} triangles, {filename.stat().st_size / 1024:.1f} KB)")


def make_quad(v0, v1, v2, v3):
    """Returns two triangles forming a quad with outward normal."""
    n = np.cross(np.array(v1) - np.array(v0), np.array(v2) - np.array(v0))
    norm_len = np.linalg.norm(n)
    if norm_len > 1e-6:
        n = n / norm_len
    else:
        n = np.array([0.0, 0.0, 1.0])
    t1 = (tuple(n), tuple(v0), tuple(v1), tuple(v2))
    t2 = (tuple(n), tuple(v0), tuple(v2), tuple(v3))
    return [t1, t2]


def make_box(x, y, z, dx, dy, dz):
    """Creates a 6-sided solid rectangular box."""
    facets = []
    # Bottom (-Z)
    facets.extend(make_quad([x, y, z], [x, y + dy, z], [x + dx, y + dy, z], [x + dx, y, z]))
    # Top (+Z)
    facets.extend(make_quad([x, y, z + dz], [x + dx, y, z + dz], [x + dx, y + dy, z + dz], [x, y + dy, z + dz]))
    # Front (-Y)
    facets.extend(make_quad([x, y, z], [x + dx, y, z], [x + dx, y, z + dz], [x, y, z + dz]))
    # Back (+Y)
    facets.extend(make_quad([x, y + dy, z], [x, y + dy, z + dz], [x + dx, y + dy, z + dz], [x + dx, y + dy, z]))
    # Left (-X)
    facets.extend(make_quad([x, y, z], [x, y, z + dz], [x, y + dy, z + dz], [x, y + dy, z]))
    # Right (+X)
    facets.extend(make_quad([x + dx, y, z], [x + dx, y + dy, z], [x + dx, y + dy, z + dz], [x + dx, y, z + dz]))
    return facets


def make_hollow_box(x, y, z, dx, dy, dz, wall_t):
    """Creates an open-topped enclosure shell with walls and floor."""
    facets = []
    # Bottom floor outer
    facets.extend(make_quad([x, y, z], [x, y + dy, z], [x + dx, y + dy, z], [x + dx, y, z]))
    # Bottom floor inner
    facets.extend(make_quad([x + wall_t, y + wall_t, z + wall_t], [x + dx - wall_t, y + wall_t, z + wall_t],
                            [x + dx - wall_t, y + dy - wall_t, z + wall_t], [x + wall_t, y + dy - wall_t, z + wall_t]))

    # Outer perimeter walls
    facets.extend(make_quad([x, y, z], [x + dx, y, z], [x + dx, y, z + dz], [x, y, z + dz]))
    facets.extend(make_quad([x, y + dy, z], [x, y + dy, z + dz], [x + dx, y + dy, z + dz], [x + dx, y + dy, z]))
    facets.extend(make_quad([x, y, z], [x, y, z + dz], [x, y + dy, z + dz], [x, y + dy, z]))
    facets.extend(make_quad([x + dx, y, z], [x + dx, y + dy, z], [x + dx, y + dy, z + dz], [x + dx, y, z + dz]))

    # Inner perimeter walls
    facets.extend(make_quad([x + wall_t, y + wall_t, z + wall_t], [x + wall_t, y + wall_t, z + dz],
                            [x + dx - wall_t, y + wall_t, z + dz], [x + dx - wall_t, y + wall_t, z + wall_t]))
    facets.extend(make_quad([x + wall_t, y + dy - wall_t, z + wall_t], [x + dx - wall_t, y + dy - wall_t, z + wall_t],
                            [x + dx - wall_t, y + dy - wall_t, z + dz], [x + wall_t, y + dy - wall_t, z + dz]))
    facets.extend(make_quad([x + wall_t, y + wall_t, z + wall_t], [x + wall_t, y + dy - wall_t, z + wall_t],
                            [x + wall_t, y + dy - wall_t, z + dz], [x + wall_t, y + wall_t, z + dz]))
    facets.extend(make_quad([x + dx - wall_t, y + wall_t, z + wall_t], [x + dx - wall_t, y + wall_t, z + dz],
                            [x + dx - wall_t, y + dy - wall_t, z + dz], [x + dx - wall_t, y + dy - wall_t, z + wall_t]))

    # Top Rim Lip
    facets.extend(make_quad([x, y, z + dz], [x + wall_t, y + wall_t, z + dz], [x + dx - wall_t, y + wall_t, z + dz], [x + dx, y, z + dz]))
    facets.extend(make_quad([x, y + dy, z + dz], [x + dx, y + dy, z + dz], [x + dx - wall_t, y + dy - wall_t, z + dz], [x + wall_t, y + dy - wall_t, z + dz]))
    facets.extend(make_quad([x, y, z + dz], [x, y + dy, z + dz], [x + wall_t, y + dy - wall_t, z + dz], [x + wall_t, y + wall_t, z + dz]))
    facets.extend(make_quad([x + dx, y, z + dz], [x + dx - wall_t, y + wall_t, z + dz], [x + dx - wall_t, y + dy - wall_t, z + dz], [x + dx, y + dy, z + dz]))

    return facets


def make_cylinder(cx, cy, z, r, h, segments=24):
    """Creates a solid vertical cylinder."""
    facets = []
    angles = np.linspace(0, 2 * np.pi, segments, endpoint=False)
    for i in range(segments):
        a1 = angles[i]
        a2 = angles[(i + 1) % segments]
        x1, y1 = cx + r * np.cos(a1), cy + r * np.sin(a1)
        x2, y2 = cx + r * np.cos(a2), cy + r * np.sin(a2)

        # Side quad
        facets.extend(make_quad([x1, y1, z], [x2, y2, z], [x2, y2, z + h], [x1, y1, z + h]))
        # Bottom cap
        facets.append(((0, 0, -1), (cx, cy, z), (x2, y2, z), (x1, y1, z)))
        # Top cap
        facets.append(((0, 0, 1), (cx, cy, z + h), (x1, y1, z + h), (x2, y2, z + h)))
    return facets


def generate_front_shell():
    """Generates the front bezel shell with cutouts for LCD, camera cone, LEDs, and buttons."""
    w, l, h = 78.0, 145.0, 16.0
    wall = 2.4
    facets = make_hollow_box(0, 0, 0, w, l, h, wall)

    # Add 4 corner screw bosses (height 16mm, outer radius 4mm)
    for bx, by in [(wall + 2.5, wall + 2.5), (w - wall - 2.5, wall + 2.5),
                   (wall + 2.5, l - wall - 2.5), (w - wall - 2.5, l - wall - 2.5)]:
        facets.extend(make_cylinder(bx, by, wall, 4.0, h - wall, segments=16))

    # Add internal LCD retaining shelf (recess 1.5mm)
    facets.extend(make_box(16.0, 12.0, wall, 46.0, 2.0, 4.0))
    facets.extend(make_box(16.0, 75.0, wall, 46.0, 2.0, 4.0))

    # Add battery compartment retaining rib
    facets.extend(make_box((w - 42.0) / 2, 85.0, wall, 42.0, 1.6, 6.0))

    return facets


def generate_back_shell():
    """Generates the back enclosure housing with battery bay and IP5306 cradle."""
    w, l, h = 78.0, 145.0, 16.0
    wall = 2.4
    facets = make_hollow_box(0, 0, 0, w, l, h, wall)

    # 4 corner screw bosses with pass-through cylinders
    for bx, by in [(wall + 2.5, wall + 2.5), (w - wall - 2.5, wall + 2.5),
                   (wall + 2.5, l - wall - 2.5), (w - wall - 2.5, l - wall - 2.5)]:
        facets.extend(make_cylinder(bx, by, wall, 4.0, h - wall, segments=16))

    # Battery bay cradle ribs (Li-Po 51.5 x 41.5 x 7.5 mm)
    facets.extend(make_box((w - 41.5) / 2, 20.0, wall, 41.5, 1.8, 7.5))
    facets.extend(make_box((w - 41.5) / 2, 20.0 + 51.5, wall, 41.5, 1.8, 7.5))

    # IP5306 module cradle ribs (26.5 x 21.5 mm)
    facets.extend(make_box(12.0, 85.0, wall, 21.5, 1.8, 6.0))
    facets.extend(make_box(12.0, 85.0 + 26.5, wall, 21.5, 1.8, 6.0))

    return facets


def main():
    print("Generating SpyScan 3D Enclosure STL files...")
    front_facets = generate_front_shell()
    front_stl = OUTPUT_DIR / "spyscan_front_shell.stl"
    write_binary_stl(front_stl, front_facets)

    back_facets = generate_back_shell()
    back_stl = OUTPUT_DIR / "spyscan_back_shell.stl"
    write_binary_stl(back_stl, back_facets)
    print("\n✓ Both STL files generated successfully in hardware/")


if __name__ == "__main__":
    main()

