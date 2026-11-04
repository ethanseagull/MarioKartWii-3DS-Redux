# Diagnostic build

A build of the game from this tree with the QA layer and automatic per-race
profiling (`MKW_AUTO_PROFILE`, no `MKW_RELEASE`). It is packaged as its own
title, `000400000F4B5D00` "MKW3DSDIAG", so it installs beside the release CIA
and shares its SD data (`sd:/3ds/mkw-3ds/`).

For developers measuring performance. To play the game, build the standard
version with `tools/build.py` (see the main README).

## Inputs

Run `tools/build.py` once first. These scripts reuse its work directory
(`MKW_WORK`, default `build/standard`): the libraries, shaders and generated
headers in `compile/`, the HOME banner in `home-banner/production/` and the
makerom it found or downloaded. Nothing else outside this repository is read.

| Variable | Contents |
| --- | --- |
| `MKW_WORK` | the standard build's work directory (default `build/standard`) |
| `DEVKITPRO` | devkitPro root (falls back to `C:/devkitPro`, `/opt/devkitpro`) |
| `MKW_BANNER` | another banner instead of the standard build's |
| `MAKEROM` | makerom, when it is neither on `PATH` nor in the standard build |

## Build

```sh
python tools/diag/compile.py       # build/diag/mkw-3ds-diag.elf
python tools/diag/package_cia.py   # build/diag/...-DIAG.cia
```

`compile.py` recompiles only changed translation units and links against the
standard build's `libcitro3d.a` and `libCFL.a`. `package_cia.py` forces title
version 142, verifies every TMD hash, and writes `build-info.json` with the
commit and hashes.

## Unattended benchmark (32 courses)

Copy both files from `tools/diag/benchmark/` into `sd:/3ds/mkw-3ds/`, start
the DIAG title and leave the console on charge. It goes through the menus,
races all 32 courses in order in VS with the CPU driving your kart, then exits
to the HOME Menu (about 100 minutes).

- `menu-autoplay-result.json` says whether the script finished.
- `perf-races.csv` has one row per race; each race also writes
  `perf-race-NNN-*.csv` (per second, spikes, passes, draws, skinning).
- After every race the game writes `benchmark-progress.txt` with the next
  course (0–31, cup order). A stopped run resumes there; delete the file to
  start again from Luigi Circuit.
- A race with no forward progress for 60 s is finished on the spot and
  listed under `"stuck"` in the result file; leave those rows out of
  comparisons.
- `benchmark-course.txt` (a course number, 0–31) races every race of the
  set on that course instead, for a short single-course A/B. With it,
  `unit1-alternate.txt` turns `no-unit1-vram.txt`'s change on for the first
  race load and off for the next, and so on (`runtime.log`: `unit1 vram
  on/off`); every load then places the course textures again. The set still
  counts 32 races: end the script with `quit` after the last race you want.
- The benchmark forces VS rules to Courses: In Order, Races: 32, which can be
  saved to your license. Delete both files afterwards, or the DIAG title
  autoplays every launch.

## A/B switches

Create an empty file in `sd:/3ds/mkw-3ds/` to turn one change off:
`no-pipeline.txt`, `no-bloom-merge.txt`, `no-bottom-cover.txt`,
`no-shadow-rect.txt`, `no-player-vram.txt`, `no-draw-distance.txt`,
`no-spatial-course.txt`, `no-hud-shrink.txt`, `no-sky-last.txt` (Rainbow
Road always draws its sky first), `no-load-cache.txt`, `no-plain-uv.txt`,
`no-fast-vertex.txt` (model draws without environment maps, projected
textures, light textures or shadow passes go back to the vertex shader's
general path; the `fast_vertex` column), `no-vram-priority.txt` (course and
sky textures take VRAM in file order instead of sky first, then by covered
surface per byte; the `vram_priority` column). Each race's `perf-races.csv` row
records which were on. `no-sector-reads.txt` reads pack entries exactly as
stored instead of as whole 512-byte sectors (load times only).
`no-read-ahead.txt` reads only the requested entry instead of the whole entries
that follow it within 12 KB (128 KB among entries a repacked pack lays out in
traced order). `no-race-cache.txt` turns off the New 3DS RAM cache that keeps
the racers, items, effects, Lakitu and pause menu textures read by one race
load for the next.
`no-pause-keep.txt` releases the racers' graphics on every pause, as before.
`no-prefetch.txt` turns off the background thread that reads the next 128 KB
of a repacked pack's traced entries while the game parses the last read.
`pack-trace.txt` writes every pack read to `pack-trace.csv` (archive, offset,
size, ms, read-ahead hit, matches a direct read) for `tools/pak_order.py`.

## Repacked packs

`tools/pakpack.c` (build: `gcc -O2 -o pakpack tools/pakpack.c`) rewrites an
MPK1 pack as MPK2: entries in the order `tools/pak_order.py` gives from pack
traces, LZ4-compressed where that saves space (lossless; every block is
checked against its source), loose files optionally added
(`textures/firebar-indirect.ftan=...`). The game reads MPK1 and MPK2.
`tools/build.py` does this for both packs with the read order saved in
`tools/pak-order/` (the traced part of `pak_order.py`'s output: its first
3,803 race.pak and 276 menu.pak lines). After new traces, regenerate the
order and save its traced lines there.

Two files turn an Old 3DS reduction back on for a New 3DS comparison:
`racer-lod.txt` (opponents switch to low-detail models with distance; the
`racer_lod` column) and `animation-budget.txt` (crowds and animated items drop
to 30/15 Hz when frames run slow). Without them every opponent keeps full
detail and animations run every tick, as in the Wii's single-player races.

The per-second and spike files also log `hook_ms` (the deferred writes run
between two GPU frames) and `gpu_gap_ms` (how long the GPU sat idle before
each frame's commands were queued). The per-second file adds `gpu_gap_min_ms`,
the second's smallest gap: what a frame that was ready in time still waits.
`no-unit1-vram.txt` keeps 4-bit textures that texture unit 1 samples (the
baked shadow maps) in LINEAR; without it their ETC1 goes to VRAM as ETC1A4.
`runtime.log` reports after each race load what stays in LINEAR (`course
textures left in LINEAR track+sky=… unit1=… objects=…`, count/KB).
`stereo-slider.txt` (a number from 0 to 1) pins the 3D slider for comparable
runs.
`no-menu-pipeline.txt` draws menus immediately on the main thread (CPU work,
then the GPU, one after the other) instead of through the render thread.
The Settings page's options are in `settings.dat`, not in the license:
Dynamic Shadows for benchmarks is set there (or on the Settings page).

## Ghost sync check

`ghost-test.txt` with a script whose Time Trial drives `race:N:items` (A held,
L pressed at frames 360, 660 and 960): the first 20 s of the race are saved as
the course's ghost. Race that ghost (Your Ghost on the ghost page) with the
same script: `runtime.log` lists `ghost test: frame=… distance=…` every 60
frames. A distance that stays at 0 means the recorded inputs replay exactly,
mushrooms included.
