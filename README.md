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

- Your own **Mario Kart Wii USA** disc image (game ID **RMCE01**) as an ISO or WBFS. Convert RVZ or NKit images first.
- Your 3DS's **CFL_Res.dat**, the console's Mii rendering resource. It isn't on the Wii disc; step 6 below shows how to copy it.
- A Windows, Linux or macOS PC with about 40 GB free and an internet connection. The build has been tested on Windows.

Patched Kinoko, Citro3D and CFL source is included. The repository includes
eight MK7 marker images inherited from the original project and the supplied
HOME icon and logo; other game assets, native Mii system resources, generated
game headers, saves and compiled packages are not supplied.

## Building

`tools/build.py` builds the standard version from your disc in one command:
it converts everything, compiles the game and makes the CIA and the SD card
folder. The steps are for Windows; on Linux or macOS install the same tools
with your package manager and use `python3` instead of `py -3`.

### Set up (once)

1. Install [Python](https://www.python.org/downloads/) 3.11 or newer.
2. Install [Git](https://git-scm.com/download/win).
3. Install [devkitPro](https://devkitpro.org/wiki/Getting_Started) and select
   the 3DS packages (devkitARM, libctru and 3ds-tools).
4. Open the devkitPro MSYS2 shell from the Start menu and run `pacman -S gcc`.
   The build compiles a few small helper programs for your PC with it. On
   Linux or macOS any recent `g++` or `clang++` works.
5. Get the code. Use `git clone`; a ZIP download won't build.

   ```sh
   git clone https://github.com/ethanseagull/MarioKartWii-3DS-Redux.git
   ```

Node.js, ffmpeg, makerom and bannertool are used from `PATH` if you have
them. Otherwise the build downloads the versions pinned in
[tools/release/toolchain.lock.json](tools/release/toolchain.lock.json) and
checks their SHA-256. Node and ffmpeg are only downloaded on Windows.

### Copy CFL_Res.dat from your 3DS

6. On the 3DS, in GodMode9:
   1. Open `[1:] SYSNAND CTRNAND` → `title` → `0004009b` → `00010202` → `content`.
   2. Press A on the `.app` file, choose *NCCH image options...* → *Mount image
      to drive*, and answer yes to open it.
   3. Open `romfs`, press A on `CFL_Res.dat` and choose *Copy to 0:/gm9/out*.
   4. Put the SD card in your PC and copy `gm9/out/CFL_Res.dat` somewhere
      handy.

### Build

7. Open a terminal (PowerShell is fine) in the folder you cloned and run:

   ```sh
   py -3 tools/build.py --disc "C:/Games/RMCE01.wbfs" --cfl "C:/Games/CFL_Res.dat"
   ```

   The first run takes about an hour on a fast PC and needs nothing from you
   while it runs. It sets up `.venv`, fetches the converters pinned in
   [dependencies.json](dependencies.json), extracts the disc, converts every
   course, kart, menu, movie and sound, compiles Citro3D, CFL and the game,
   and builds the HOME banner. If it stops with an error, fix what it names
   and run the same command again: it continues where it stopped. Logs are in
   `build/standard/logs/`.

The results are in `build/standard/output/`:

- `MarioKartWii-3DS-Redux-v<version>.cia`, the game;
- `sd/3ds/`, the game data for the SD card;
- `mkw-3ds.3dsx`, the same game for the Homebrew Launcher.

### Install

8. Copy the `sd/3ds` folder to the root of your SD card. When updating, merge
   it into the existing one; your saves are separate files and stay.
9. Copy the CIA to the SD card and install it with FBI.

To update later, run `git pull`, then the same build command. Only what
changed is rebuilt, usually in a few minutes. `--work DIR` puts the build
somewhere else, `--jobs N` sets how many converters run at once (default 4),
and `--from STAGE` runs a stage and everything after it again
(`--list-stages` lists them).

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
resource family is a separate operation; `tools/build.py` shows the order it
runs them in.

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

The runtime files go in `sd:/3ds/mkw-3ds/`: `menu.pak`, `race.pak` and the loose
files required by the resource map. A build from `tools/build.py` has them all
in `build/standard/output/sd/3ds/mkw-3ds/`.

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
