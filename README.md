# Mario Kart Wii 3DS Redux (New 3DS fork)

A native Nintendo 3DS port of Mario Kart Wii. This is
[ethanseagull](https://github.com/ethanseagull)'s fork of
[Epic0522's MarioKartWii-3DS-Redux](https://github.com/Epic0522/MarioKartWii-3DS-Redux),
which builds on dudewhat's original port and MiniBandit's continuation.

## Features

- Grand Prix in 50cc, 100cc, 150cc and Mirror, with cup progression, unlocks and award ceremonies.
- Time Trials with saved records, solo and team VS, Balloon Battle and Coin Runners.
- All characters and vehicles, including unlockables and native 3DS Mii drivers.
- Four license slots with their own settings, rules and records.
- Dual-screen menus, race results and maps, with the original materials, animations, particles and audio.
- Optional dynamic shadows and notification LED effects (License Settings).

## What you need

- Your own **Mario Kart Wii USA** disc image (game ID **RMCE01**) as an ISO or WBFS. Convert RVZ or NKit images first.
- Your 3DS's **CFL_Res.dat**, the console's Mii rendering resource. It isn't on the Wii disc; step 6 below shows how to copy it.
- A PC with about 40 GB free and an internet connection. The build is tested on Windows.
- A 3DS with custom firmware (Luma3DS), FBI and GodMode9.

No game files are included in this repository. Everything the game needs is
converted from your own disc.

## Building

The steps are for Windows. On Linux or macOS install the same tools with your
package manager and use `python3` instead of `py -3`.

### Set up (once)

1. Install [Python](https://www.python.org/downloads/) 3.11 or newer.
2. Install [Git](https://git-scm.com/download/win).
3. Install [devkitPro](https://devkitpro.org/wiki/Getting_Started) and select
   the 3DS packages (devkitARM, libctru and 3ds-tools).
4. Open the devkitPro MSYS2 shell from the Start menu and run `pacman -S gcc`.
   The build compiles a few small helper programs for your PC with it. On
   Linux or macOS any recent `g++` or `clang++` works.
5. Get the code with `git clone`.

   ```sh
   git clone https://github.com/ethanseagull/MarioKartWii-3DS-Redux.git
   ```

Node.js, ffmpeg, makerom and bannertool are used from `PATH` if you have
them. Otherwise the build downloads the versions pinned in
[tools/release/toolchain.lock.json](tools/release/toolchain.lock.json) and
checks their SHA-256. Node and ffmpeg are only downloaded on Windows. ffmpeg
needs the soxr resampler, which most builds have.

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

## Installing

8. Copy the `sd/3ds` folder to the root of your SD card. The game data ends
   up in `sd:/3ds/mkw-3ds/`.
9. Copy the CIA to the SD card and install it with FBI. Or skip the CIA and
   start `mkw-3ds.3dsx` from the Homebrew Launcher.

If your console has never run a homebrew game with sound, dump its DSP
firmware once with DSP1.

To update, run `git pull`, then the same build command; only what changed is
rebuilt, usually in a few minutes. The game code is recompiled where it
changed, and a cook stage runs again when its tool or a file it reads changed,
together with the stages after it (an early stage can take as long as a first
build's cooking). Copy the new `sd/3ds` over the old one and install the new
CIA. Your saves are separate files (`rksys.dat` and its
backup, `license-options-*.dat`, `last-license.txt`, `mii-*.3dsmii`), so they
stay.

Build options: `--work DIR` builds somewhere other than `build/standard`,
`--jobs N` sets how many converters run at once (default 4), and
`--from STAGE` runs one stage and everything after it again
(`--list-stages` lists them).

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
| D-pad / ZR | Boost panel trick |
| START | Pause |
| SELECT | Toggle the speedometer |
| Touch screen | Switch the bottom-screen map view |

## Development

- [docs/MANUAL-BUILD.md](docs/MANUAL-BUILD.md): what each build stage does and
  how to run one by hand.
- [docs/MANUAL-RESOURCE-STAGES.md](docs/MANUAL-RESOURCE-STAGES.md): every
  converter with its inputs and outputs.
- [docs/HOME-BANNER.md](docs/HOME-BANNER.md): how the HOME banner is made.
- [tools/diag](tools/diag/README.md): the diagnostic build and the benchmark.
- [docs/NOTIFICATION_LED.md](docs/NOTIFICATION_LED.md): the LED effects.

When reporting a problem, say which console (or emulator) you used, the mode,
the course and how to make it happen. Keep game files, generated files, saves
and system resources out of pull requests.

## Credits

- **dudewhat**: the original Mario Kart Wii 3DS port.
- **MiniBandit**: the continuation this project builds on.
- **Epic**: Redux development and integration.
- **ethanseagull**: this fork.
- **DarthMDev**: Old 3DS drawing performance fix, race sound fixes and menu cadence.
- **Codiak540**: Old 3DS performance testing and research (course-object detail distance, particle limit, performance log idea).
- **janedoemk3ds**: Old 3DS performance research with Codiak540 (lighter models, visibility culling, faster course drawing; PR #40 in mkw-3ds).
- **vabold / Kinoko**: Mario Kart Wii physics.
- **noclip.website**: BRRES model parsing.
- **LagoLunatic / gclib**: disc and texture utilities.
- **devkitPro, libctru and Citro3D**: 3DS development and graphics libraries.
- **CFL contributors**: native Mii rendering.
- **ogws and mkw decompilation projects**: references for original behavior.
- **Project CTR (makerom) and bannertool contributors**: packaging tools.

## License

See [LICENSE](LICENSE), [NOTICE](NOTICE) and [licenses/](licenses/). The root
MIT text does not cover every file; NOTICE explains which parts have no
declared license yet. Mario Kart Wii is copyright Nintendo. This is an
unofficial fan project, not affiliated with Nintendo.
