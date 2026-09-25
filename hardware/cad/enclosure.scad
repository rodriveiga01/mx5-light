// SPDX-License-Identifier: GPL-3.0-or-later
// Parametric enclosure for the NA wink controller (prototype intent).
// Prototype material (ASA/ABS print) ONLY - NOT automotive-qualified.
// See docs/SAFETY_CASE.md for thermal / moisture / vibration limits.
// Validate with: parser check in scripts/verify.sh (mesh render needs OpenSCAD).
//
// Contents: PCB mounting (4x M3 bosses + inserts), relay clearance dome,
// connector clearance cutouts (J1/J2/J3), cable exits (16 mm gland + boots),
// internal strain-relief posts, chassis mounting ears, tilt drain, service
// lid with fasteners, status-LED window (DEFERRED/NON-FUNCTIONAL in rev 0.3.1:
// no LED in BOM/schematic — do NOT rely on it), fuse-access hatch (service
// disconnect without opening the case).
//
// Placement: cabin/footwell REQUIRED (never engine bay). Connectors DOWN
// (drip). Fuse holder reachable from outside.
// Print: ASA/ABS, 0.2 mm layers, 4 walls, 40% gyroid (prototype only).

// --- Parameters (mm) ---
pcb_l = 100; pcb_w = 70; pcb_t = 1.6;
wall = 2.5; lid_t = 2.0; clearance_xy = 1.5; clearance_z = 8;
boss_d = 6.0; boss_hole_d = 3.2;           // M3 self-tap / insert
gland_d = 16.0;                             // cable gland entry
drain_d = 3.0;                              // tongue-and-groove tilt drain corner
strain_post_d = 8.0;
relay_dome_h = 6.0;                         // extra clearance over K1-K4 bank
conn_cut_w = 30.0; conn_cut_h = 12.0;       // J1/J2/J3 face cutouts
ear_w = 12.0; ear_hole_d = 4.2;             // M4 chassis ears
led_win_d = 4.0;                            // deferred LED window (NON-FUNCTIONAL rev 0.3.1 — geometry kept, no LED fitted)
fuse_hatch_w = 20.0; fuse_hatch_h = 14.0;   // F1 service hatch
lid_screw_d = 3.2; lid_insert_d = 4.0;      // M3 lid screws + inserts

inner_l = pcb_l + 2*clearance_xy;
inner_w = pcb_w + 2*clearance_xy;
outer_l = inner_l + 2*wall;
outer_w = inner_w + 2*wall;
base_h = clearance_z + pcb_t + 6;           // room for relays + buck inductor

module body() {
  difference() {
    // outer body
    cube([outer_l, outer_w, base_h + lid_t], center=false);
    // cavity
    translate([wall, wall, wall])
      cube([inner_l, inner_w, base_h + lid_t], center=false);
    // relay clearance dome (over K1-K4 bank, center-right of PCB)
    translate([outer_l*0.35, outer_w*0.45, base_h - relay_dome_h])
      cube([outer_l*0.5, outer_w*0.4, relay_dome_h + lid_t + 1], center=false);
    // cable entry (gland side, +X face)
    translate([outer_l - 1, outer_w/2, wall + 4])
      rotate([0, 90, 0])
      cylinder(d=gland_d, h=wall + 2, $fn=32);
    // connector face cutouts (J1/J2/J3 edge, -X face)
    translate([-1, outer_w*0.25, wall + 2])
      cube([wall + 2, conn_cut_w, conn_cut_h], center=false);
    // fuse service hatch (top edge near F1 corner)
    translate([wall + 6, -1, base_h - fuse_hatch_h/2])
      cube([fuse_hatch_w, wall + 2, fuse_hatch_h], center=false);
    // deferred status-LED window (lid-adjacent wall; NON-FUNCTIONAL rev 0.3.1)
    translate([wall + inner_l - 10, outer_w - 1, base_h - 4])
      rotate([90, 0, 0])
      cylinder(d=led_win_d, h=wall + 2, $fn=16);
    // tilt drain at -X/-Y corner
    translate([wall + 6, wall + 6, -0.5])
      cylinder(d=drain_d, h=wall + 1, $fn=16);
    // lid screw counterbores x4
    for (x = [wall + 4, wall + inner_l - 4])
      for (y = [wall + 4, wall + inner_w - 4])
        translate([x, y, base_h + lid_t - 1])
          cylinder(d=lid_screw_d + 1.6, h=2, $fn=16);
  }
  // PCB bosses x4 (M3 inserts)
  for (x = [wall + 4, wall + inner_l - 4])
    for (y = [wall + 4, wall + inner_w - 4])
      difference() {
        translate([x, y, wall]) cylinder(d=boss_d, h=6, $fn=24);
        translate([x, y, wall]) cylinder(d=boss_hole_d, h=7, $fn=16);
      }
  // strain-relief posts near gland
  translate([outer_l - wall - 12, outer_w/2 - 10, wall])
    cylinder(d=strain_post_d, h=8, $fn=24);
  translate([outer_l - wall - 12, outer_w/2 + 10, wall])
    cylinder(d=strain_post_d, h=8, $fn=24);
  // chassis mounting ears x2 (-X/+X)
  for (sx = [-ear_w, outer_l])
    difference() {
      translate([sx, outer_w/2 - 10, 0]) cube([ear_w, 20, wall], center=false);
      translate([sx + ear_w/2, outer_w/2, -0.5])
        cylinder(d=ear_hole_d, h=wall + 1, $fn=16);
    }
}

module lid() {
  translate([0, 0, base_h + lid_t]) {
    difference() {
      cube([outer_l, outer_w, lid_t], center=false);
      // tongue-and-groove channel (mating ridge on body implied)
      translate([wall/2, wall/2, -1])
        cube([outer_l - wall, outer_w - wall, 1.5], center=false);
      // lid screw through-holes x4 (match body counterbores)
      for (x = [wall + 4, wall + inner_l - 4])
        for (y = [wall + 4, wall + inner_w - 4])
          translate([x, y, -0.5])
            cylinder(d=lid_screw_d, h=lid_t + 1, $fn=16);
    }
  }
}

body();
lid();

// echo key dims for verify.sh parser check
echo(str("OUTER ", outer_l, " x ", outer_w, " x ", base_h + lid_t));
echo(str("NOT automotive-qualified - prototype ASA/ABS only"));
