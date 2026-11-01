# Diagnostic build

A build of the game from this tree with the QA layer and automatic per-race
profiling (`MKW_AUTO_PROFILE`, no `MKW_RELEASE`). It is packaged as its own
title, `000400000F4B5D00` "MKW3DSDIAG", so it installs beside the release CIA
and shares its SD data (`sd:/3ds/mkw-3ds/`).

## Inputs

The manual build (`docs/MANUAL-BUILD.md`) produces the generated headers and
libraries from your own disc. None of them are in this repository.

| Variable | Contents |
| --- | --- |
| `MKW_GENERATED` | generated headers, `libCFL.a` and the unpatched `libcitro3d.a` |
| `MKW_GENERATED_INCLUDE` | directory holding the generated `mkw/*.hpp` tables (default `MKW_GENERATED`) |
| `DEVKITPRO` | devkitPro root (falls back to `C:/devkitPro`, `/opt/devkitpro`) |
| `MKW_BANNER_WORK` | banner work directory (`docs/HOME-BANNER.md`) |
| `MKW_EXTRACTED` | extracted disc files (default `data/extracted`) |
| `MAKEROM`, `BANNERTOOL` | tool paths when they are not on `PATH`; `ffmpeg` must be on `PATH` |

## Build

```sh
python tools/diag/compile.py       # build/diag/mkw-3ds-diag.elf
python tools/diag/make_banner.py   # build/banner/banner-startgrid.bnr
python tools/diag/package_cia.py   # build/diag/...-DIAG.cia
```

`compile.py` recompiles only changed translation units. It rebuilds
`libcitro3d.a` from the manual build's archive with this tree's patched
members (`framebuffer.c`, `renderqueue.c`, `texunit.c`: pipelined frames and
texture unit 2 on coordinate 1). `package_cia.py` forces title version 142,
verifies every TMD hash, and writes `build-info.json` with the commit and
hashes.

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
- The benchmark forces VS rules to Courses: In Order, Races: 32, which can be
  saved to your license. Delete both files afterwards, or the DIAG title
  autoplays every launch.

## A/B switches

Create an empty file in `sd:/3ds/mkw-3ds/` to turn one change off:
`no-pipeline.txt`, `no-bloom-merge.txt`, `no-bottom-cover.txt`,
`no-shadow-rect.txt`, `no-player-vram.txt`, `no-draw-distance.txt`,
`no-spatial-course.txt`, `no-hud-shrink.txt`, `no-sky-last.txt`,
`no-load-cache.txt`. Each race's `perf-races.csv` row records which were on.
`stereo-slider.txt` (a number from 0 to 1) pins the 3D slider for comparable
runs.
