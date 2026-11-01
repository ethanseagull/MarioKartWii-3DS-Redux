# Mario Kart Wii 3DS Redux

A native Nintendo 3DS port of Mario Kart Wii, based on dudewhat's original
port and MiniBandit's continuation.

## Features

- Grand Prix with 50cc, 100cc, 150cc and Mirror classes, cup progression, unlocks and award ceremonies.
- Time Trials with saved records, solo and team VS, and Balloon Battle and Coin Runners.
- Character and vehicle selection, including unlockable vehicles and native 3DS Mii drivers.
- Four license slots with saved identities, rules, records and presentation settings.
- Dual-screen menus, race results and maps, with restored model materials, animations, particles and audio.
- Optional dynamic shadows.
- Notification LED effects for items, race events and drifting, with mode and brightness settings.
- Distance-based opponent detail and adaptive animation updates for demanding scenes.

See [CHANGELOG.md](CHANGELOG.md) for the full cumulative change list.
Online play is not included. Performance varies by hardware and course.
Target 60; running in the 40–60 range.

## What you need

- Your own **Mario Kart Wii USA** disc image, game ID **RMCE01**, in ISO or WBFS format; convert RVZ or NKit images to a supported format first.
- Your own native 3DS Mii resource file, **CFL_Res.dat**, obtained separately.
- devkitARM and libctru, plus picasso, 3dsxtool and tex3ds.
- Python 3, Node.js, npm, Git, ffmpeg and a host C/C++ compiler for offline utilities.
- The converter dependencies and revisions listed in [dependencies.json](dependencies.json) and [vendor/sources.lock.json](vendor/sources.lock.json).
- makerom and bannertool for CIA work, plus a banner model generated from your disc; the icon, logo PNGs and banner sound are included.
- A separate working directory for extracted files, converted resources and compiler outputs, with room for both the disc and its intermediates.

Patched Kinoko, Citro3D and CFL source is included. Acquire the remaining
converter dependencies yourself and apply the relevant files in `patches/`.
The repository includes eight MK7 marker images inherited from the original
project and the supplied HOME icon and logo; other game assets, native Mii system resources, generated game
headers, saves and compiled packages are not supplied.

## Building

One command turns your disc into the standard CIA and the SD folder.

### Prerequisites

- [devkitPro](https://devkitpro.org/wiki/Getting_Started) with the 3DS
  packages (devkitARM, libctru, 3ds-tools: picasso, tex3ds, 3dsxtool).
- A host C++ compiler. On Windows, open devkitPro's MSYS2 shell once and run
  `pacman -S gcc`; elsewhere any C++23 `g++` or `clang++` works (or set `CXX`).
- Python 3.11 or newer (on Windows from python.org; start it with `py -3`).
- Git.
- Node.js, ffmpeg, makerom and bannertool are taken from `PATH`. If they
  aren't there, the builder downloads the versions pinned in
  [tools/release/toolchain.lock.json](tools/release/toolchain.lock.json) and
  checks their SHA-256. Node and ffmpeg downloads are Windows-only; elsewhere
  install them with your package manager.
- Your **Mario Kart Wii USA (RMCE01)** ISO or WBFS.
- Your 3DS's **CFL_Res.dat**, the console's Mii rendering resource. Copy it off the console with GodMode9:
  1. Open `[1:] SYSNAND CTRNAND` → `title` → `0004009b` → `00010202` → `content`.
  2. Press A on the `.app` file, choose *NCCH image options...* → *Mount image
     to drive*, and answer yes to open it.
  3. Open `romfs`, press A on `CFL_Res.dat` and choose *Copy to 0:/gm9/out*.
  4. On your PC the file is at `gm9/out/CFL_Res.dat` on the SD card; pass that
     path (or a copy of the file) to `--cfl`.
- About 40 GB free disk space and internet access for the first run.

### Steps

```sh
git clone <this repository>
cd MarioKartWii-3DS-Redux
py -3 tools/build.py --disc "D:/Games/RMCE01.wbfs" --cfl "D:/CFL_Res.dat"
```

Use `python3` instead of `py -3` outside Windows. The first run takes an hour
or more: it converts every course, kart, menu, movie and sound on the disc. It
creates `.venv`, fetches the pinned converters from
[dependencies.json](dependencies.json), extracts the disc, runs every
conversion stage, compiles Citro3D, CFL and the game, and builds the HOME
banner. Results land in `build/standard/output/`:

- `MarioKartWii-3DS-Redux-v<version>.cia`: install it with FBI or another title installer;
- `sd/3ds/mkw-3ds/`: copy the `sd/3ds` folder to the root of your SD card;
- `mkw-3ds.3dsx`: the same game for the Homebrew Launcher.

A stopped or failed run continues where it stopped when you run the same
command again. After a code change only compiling and packaging run again.
`--work DIR` puts everything somewhere else, `--jobs N` sets parallel
converters, `--from STAGE` redoes a stage and all later ones
(`--list-stages` shows them). Logs are in `build/standard/logs/`.

## Building by hand

The steps below are what `tools/build.py` automates, for developers who want
to run a stage themselves. The linked references provide the conversion
stages, source list and compiler settings.

### 1. Prepare the tools

Install the SDK and utilities listed above, and obtain the external
converter checkouts at their pinned revisions. Populate `vendor/gclib`
before installing the Python requirements, since it is an editable local
dependency. Set up a Python environment and install the listed packages:

```sh
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install -r requirements.txt
npm ci --no-audit --no-fund
```

Use the Windows activation command if working in a Windows shell. Paths in
the examples below are placeholders and must be replaced with your own.

### 2. Extract your disc and convert the resources

Inspect the disc inventory, then extract the files needed by each stage:

```sh
python3 tools/extract_disc.py /external/RMCE01.iso /external/extracted --list-only
python3 tools/extract_disc.py /external/RMCE01.iso /external/extracted --prefix Scene/
```

Follow the [resource-stage reference](docs/MANUAL-RESOURCE-STAGES.md) and
[manual resource instructions](docs/MANUAL-BUILD.md#2-extract-and-convert-resources-one-stage-at-a-time)
for courses, characters, vehicles, menus, movies, audio and effects. Each
resource family is a separate operation; there is no full-course or
full-roster scheduler.

For example, bundle one exporter, export one BRRES model, then convert it:

```sh
node tools/bundle_cooker.mjs tools/export_brres.ts
node build/export_brres.mjs /external/source.brres /external/exported ModelName
python3 tools/cook_model.py /external/exported /external/cooked --prefix model_name
```

Some converters retain the original `data/`, `assets/`, `build/` and
`sd-card/` path conventions; inspect their inputs before running them and
keep generated content out of the published source tree. A Wii disc does
not supply the native 3DS `CFL_Res.dat` file.

### 3. Prepare archives and generated headers

Create the runtime file map, review its paths and pack the resources explicitly:

```sh
python3 tools/resource_manifest.py > /external/runtime-files.json
python3 tools/pack_race_assets.py /external/runtime-files.json /external/race.pak --exclude-menu /external/menu.pak
```

Prepare `menu.pak` first with `pack_menu_assets.py`; apply the LOD and Mii
archive overlays separately. Keep metadata read outside the archives at its
required loose path. See [archive preparation](docs/MANUAL-BUILD.md#3-organize-existing-runtime-files-and-create-archives-explicitly).

Produce the 66 external headers declared in
[config/generated-inputs.json](config/generated-inputs.json), then provide
their directory as a compiler include path. The approved Bowser's Castle
visibility policy is already included in `include/mkw/castle_regions.hpp`;
a normal build does not need a browser bake.

### 4. Compile the libraries and game

Prepare the shader headers with picasso, compile the bundled Citro3D and
CFL sources, and archive those objects. Then compile the game translation
units listed in [MANUAL-PROGRAM-SOURCES.txt](docs/MANUAL-PROGRAM-SOURCES.txt).

Use the exact architecture flags, include paths and linker wrappers in
[the compilation instructions](docs/MANUAL-BUILD.md#5-compile-libraries-then-compile-each-program-source-yourself)
and [library flags](docs/MANUAL-LIBRARY-FLAGS.json). Application sources use
GNU C++23; Kinoko sources additionally use LTO. Do not enable fast-math or
`MKW_FRAME_CAPTURE` for a normal release.

After linking the ELF, create the homebrew executable:

```sh
3dsxtool /external/mkw-3ds.elf /external/mkw-3ds.3dsx
```

### 5. Package a CIA, if needed

Follow [HOME banner preparation](docs/HOME-BANNER.md) for the included artwork
and the explicit extraction, scene and CGFX conversion stages. Prepare the
SMDH and banner with bannertool; `tools/build.py` then replaces the banner's
sound with the game's Starting Grid jingle (`sound/strm/o_Start2_32_fan.brstm`). Copy
[the RSF template](tools/release/mkw-3ds.rsf) to your working directory and
remove its RomFs block for an external-SD build. Then invoke makerom with
your compiled ELF and prepared HOME assets:

```sh
makerom -f cia -target t -exefslogo -elf /external/mkw-3ds.elf -rsf /external/external-sd.rsf -icon /external/icon.icn -banner /external/banner.bnr -ver 142 -DAPP_REMASTER=0 -o /external/MKW-3DS-v0.8.14.cia
```

Version 0.8.14 uses title version 142. Check the resulting Ticket/TMD version
and contents; some makerom builds have a version-macro collision described
in [the packaging notes](docs/MANUAL-BUILD.md#6-create-home-assets-and-cia-manually).

## Installing and running

Place the converted runtime files under `sd:/3ds/mkw-3ds/`, including
`menu.pak`, `race.pak` and the loose files required by the resource map.

- **Homebrew:** place `mkw-3ds.3dsx` in that directory and launch it with the Homebrew Launcher.
- **CIA:** install the CIA with your console's title installer and retain the runtime resources on the SD card.
- **Emulator:** launch the 3DSX in Azahar with its emulated SD directory pointing to the staged resource tree and the required system data configured.

On hardware, dump your DSP firmware with DSP1 if it is not already available.
Do not copy an emulator-only DSP stub to a console.
When updating, preserve `rksys.dat`, its backup, `license-options-*.dat`,
`last-license.txt` and `mii-*.3dsmii`. Game resources and personal system data
are not part of the source release.

## Controls

| Button | Action |
| --- | --- |
| Circle Pad | Steer and choose the direction of an item throw |
| A | Accelerate |
| B | Brake / reverse |
| R | Hop and drift; trick off a ramp |
| Y / D-pad Up | Trick / bike wheelie |
| X / ZL | Look behind |
| L | Use item |
| D-pad / ZR| Boost panel trick |
| START | Pause |
| SELECT | Toggle the speedometer |
| Touch screen | Switch the bottom-screen map view |

Dynamic shadows and LED options are in License Settings. 

## Contributing

Include the hardware or emulator used, the game mode, course and steps to
reproduce a problem. Distinguish build checks from gameplay testing. Keep
game assets, generated files, saves and native system resources out of
source changes, and preserve third-party license notices.

## Credits

- **dudewhat** - original Mario Kart Wii 3DS port.
- **MiniBandit** - the continuation this project builds on.
- **Epic** - Redux development and integration.
- **DarthMDev**: Old 3DS drawing performance fix, race sound fixes and menu cadence
- **Codiak540**: Old 3DS performance testing and optimisation research (course-object detail distance, particle limit, performance log idea)
- **janedoemk3ds**: Old 3DS performance research with Codiak540 (lighter models, visibility culling, faster course drawing; PR #40 in repo mkw-3ds)
- **vabold / Kinoko** - Mario Kart Wii physics.
- **noclip.website** - BRRES model parsing.
- **LagoLunatic / gclib** - disc and texture utilities.
- **devkitPro, libctru and Citro3D** - Nintendo 3DS development and graphics libraries.
- **CFL contributors** - native Mii rendering support.
- **ogws and mkw decompilation projects** - original-behavior references.
- **Project CTR / makerom and bannertool contributors** - packaging tools.

See [LICENSE](LICENSE), [NOTICE](NOTICE) and [licenses/](licenses/) for
attribution and licensing, including unresolved original-port and CFL license
scopes; the root MIT text does not cover the whole snapshot. Mario Kart Wii is copyright Nintendo; this is
an unofficial fan project and is not affiliated with Nintendo.
