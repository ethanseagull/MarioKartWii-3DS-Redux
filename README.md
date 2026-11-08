# Mario Kart Wii 3DS Redux

A native Nintendo 3DS port of Mario Kart Wii. This fork builds on
[Epic0522's Redux](https://github.com/Epic0522/MarioKartWii-3DS-Redux) and adds
Old 3DS support, ghosts, graphics settings and general optimization.

No game files are included. Everything is converted from your own disc.

## Features

- All 32 courses in Grand Prix, Time Trials and VS
- Grand Prix at 50cc, 100cc, 150cc and Mirror
- VS solo or in teams, Balloon Battle and Coin Runners
- All characters and vehicles, unlocks, award ceremonies and credits
- Mii drivers
- Four licenses with their own settings and records
- Staff ghosts, Expert Staff ghosts and your own saved ghosts
- Dual-screen menus, results and maps using the game's original art, effects and audio
- Runs on New 3DS and Old 3DS, optimized for both
- Settings for texture quality, dynamic shadows, bloom and opponent detail
- Speedometer and notification LED effects

## What you need

- Your own **Mario Kart Wii USA** disc (RMCE01) as an ISO or WBFS
- A PC with an internet connection and about 30 GB free
- A 3DS with Luma3DS and FBI, and about 3 GB free on the SD card
- DSP firmware dumped with DSP1, if your console has never had homebrew sound

## Building

On Windows:

1. Download this repository and unzip it, or clone it.
2. Double-click `Build.bat` and select your disc.

Anything missing, including Python, Git and devkitPro, is downloaded for you.
When it's done it deletes the converted data, leaving about 3 GB.

Or run the build script yourself. Install Python 3.12, Git, devkitPro with
`3ds-dev`, and a C++ compiler (on Windows, `pacman -S gcc` in devkitPro's MSYS2
shell). Then, from a clone:

```sh
py -3.12 tools/build.py --disc "C:/Games/RMCE01.wbfs"
```

On Linux or macOS use `python3.12`. Only Windows is tested. Add
`--delete-converted` to delete the converted data afterwards.

The first build takes about an hour. If it stops, run it again and it picks up
where it left off. The results are in `build/standard/output`.

## Installing

1. Copy the `sd/3ds` folder to the root of your SD card.
2. Install the `.cia` file with FBI, or run `mkw-3ds.3dsx` from the Homebrew Launcher.

## Updating

Run `Build.bat` again, or from your clone:

```sh
git pull
py -3.12 tools/build.py --disc "C:/Games/RMCE01.wbfs"
```

Only what changed is rebuilt, which usually takes a few minutes. An update that
changes game data does a full build. Then repeat the install steps. Your saves
are kept.

If you're coming from a version before 1.0.0, first delete everything in
`sd:/3ds/mkw-3ds/` except `rksys.dat`, `settings.dat`, `last-license.txt`, the
`license-options-*.dat` and `mii-*.3dsmii` files, and the `ghosts` folder.

See [CHANGELOG.md](CHANGELOG.md) for what's new.

## Controls

| Button | Action |
| --- | --- |
| Circle Pad | Steer, tilt in the air, aim items |
| A | Accelerate |
| B | Brake and reverse |
| R | Hop and drift, trick off a ramp |
| L | Use item |
| Y | Trick in the Circle Pad's direction, wheelie on bikes |
| D-pad | Trick in that direction, Up for a wheelie |
| ZR (New 3DS) | Same as D-pad Up |
| X, or ZL on New 3DS | Look behind |
| START | Pause |

## Credits

- dudewhat: the original port
- MiniBandit: the continuation this project builds on
- Epic: Redux
- DarthMDev: Old 3DS performance, race sound and menu fixes
- Codiak540 and janedoemk3ds: Old 3DS testing and research
- smol_bug: console photos for the pause screen
- vabold: Kinoko, the physics engine
- noclip.website: BRRES model parsing
- LagoLunatic: gclib
- devkitPro, libctru and Citro3D
- CFL contributors: Mii rendering
- ogws and the mkw decompilation projects
- Project CTR (makerom) and bannertool

## License

See [LICENSE](LICENSE), [NOTICE](NOTICE) and [licenses/](licenses/). The MIT
text doesn't cover every file; NOTICE explains which parts have no declared
license yet. Mario Kart Wii is copyright Nintendo. This is an unofficial fan
project and isn't affiliated with Nintendo.
