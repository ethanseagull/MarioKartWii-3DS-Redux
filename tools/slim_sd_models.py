#!/usr/bin/env python3
"""Move loose models/ files that menu.pak or race.pak already holds out of an SD package.

The runtime reads .gmdl, .dskm and .dobj through pack::read first (Model::load,
DriverAnimation::load, the object bank), so a loose copy of a packed entry is
never opened. Leaving thousands of them in models/ still costs time: the 3DS
looks a file up by scanning its folder entry by entry, so every remaining loose
open there (.list, .djnt) gets slower with each extra file.

Run it after both packs are final (pack_racer_lod.py and pack_mii_racers.py
included). Without --move-to it only reports. Moved files keep their names, so
moving them back restores the package exactly.

    python3 tools/slim_sd_models.py D:/3ds/mkw-3ds
    python3 tools/slim_sd_models.py D:/3ds/mkw-3ds --move-to C:/backup/models-duplicates
"""
import argparse, shutil, struct
from pathlib import Path

TYPES = ('.gmdl', '.dskm', '.dobj')


def pack_names(path):
    with path.open('rb') as f:
        header = f.read(12)
        if header[:4] not in (b'MPK1', b'MPK2'):
            raise SystemExit(f'{path}: not an MPK1 or MPK2 archive')
        count, names_bytes = struct.unpack('<II', header[4:])
        f.seek(12 + (16 if header[:4] == b'MPK2' else 12) * count)
        blob = f.read(names_bytes)
    return {n.decode() for n in blob.split(b'\0') if n}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('package', type=Path, help='the SD folder holding menu.pak, race.pak and models/')
    parser.add_argument('--move-to', type=Path, help='move the duplicates into this folder (outside the package)')
    args = parser.parse_args()
    packed = set()
    for archive in ('menu.pak', 'race.pak'):
        path = args.package / archive
        if not path.is_file():
            raise SystemExit(f'{path} is missing: both packs must be in place first')
        packed |= pack_names(path)
    models = args.package / 'models'
    loose = sorted(p for p in models.iterdir() if p.is_file() and p.suffix in TYPES)
    duplicates = [p for p in loose if 'models/' + p.name in packed]
    kept = sum(1 for p in models.iterdir() if p.is_file()) - len(duplicates)
    size = sum(p.stat().st_size for p in duplicates)
    print(f'{len(duplicates)} packed duplicates ({size / 2**20:.1f} MiB); {kept} files stay in models/')
    if not args.move_to:
        print('report only; pass --move-to to move them')
        return
    target = args.move_to.resolve()
    if args.package.resolve() in target.parents or target == args.package.resolve():
        raise SystemExit('--move-to must be outside the package')
    target.mkdir(parents=True, exist_ok=True)
    for p in duplicates:
        if (target / p.name).exists():
            raise SystemExit(f'{target / p.name} already exists')
    for p in duplicates:
        shutil.move(str(p), str(target / p.name))
    print(f'moved to {target}')


if __name__ == '__main__':
    main()
