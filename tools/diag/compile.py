"""Compile the diagnostic ELF from this source tree -> build/diag/mkw-3ds-diag.elf:
  - MKW_RELEASE omitted: QA layer (runtime.log, menu profiler, load-stage and
    slow-job timings, sound stats, full-quality.txt toggle, subsystem logs).
  - MKW_AUTO_PROFILE: every race is profiled automatically from the end of
    the intro to results; per-second, spike and summary CSVs are written to
    sdmc:/3ds/MKWiiDS/ on pause, race end and exit.

Inputs: the standard build's work directory (tools/build.py; MKW_WORK,
default build/standard): compile/libCFL.a and compile/libcitro3d.a (built
from vendor/, with this tree's citro3d patches), compile/shader/shader_data.hpp
and compile/generated/ (the tables cooked from your disc). Run tools/build.py
once first; afterwards only this script is needed after a code change.
  DEVKITPRO       devkitPro root (falls back to C:/devkitPro, /opt/devkitpro)

    python tools/diag/compile.py
"""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import hashlib, json, os, subprocess, sys

ROOT = Path(__file__).resolve().parents[2]
WORK = Path(os.environ.get('MKW_WORK', ROOT / 'build/standard')).resolve() / 'compile'
if not (WORK / 'generated').is_dir():
    raise SystemExit(f'{WORK / "generated"} missing: run tools/build.py first (or set MKW_WORK).')
GEN = WORK / 'generated'
# MKW_DIAG_VARIANT=o3dstest: the Old 3DS test build for a New 3DS (include/mkw/console_model.hpp),
# in build/diag-o3dstest/ (its own objects, so the two builds don't rebuild each other).
VARIANT = os.environ.get('MKW_DIAG_VARIANT', '')
OUT = ROOT / ('build/diag-' + VARIANT if VARIANT else 'build/diag')
NAME = 'mkw-3ds-diag'
# MKW_KINOKO_PGO: profile-guided optimization of Kinoko (vendor/kinoko, built with LTO).
#   generate  a training build in <out>-pgogen whose Kinoko writes its profile counters
#             to sdmc:/3ds/MKWiiDS/pgo/<source path>.gcda at each race end (merged over
#             races); copy that folder's contents to tools/pgo/kinoko/.
#   off       ignore tools/pgo/kinoko/.
#   (unset)   Kinoko sources with a profile in tools/pgo/kinoko/ are built with it
#             (tools/build.py does the same); the rest as before.
PGO = os.environ.get('MKW_KINOKO_PGO', '')
if PGO == 'generate':
    OUT = OUT.with_name(OUT.name + '-pgogen')
KINOKO_SOURCE = ROOT / 'vendor/kinoko/source'
PGO_DIR = ROOT / 'tools/pgo/kinoko'
# Path components of the training build's object folder for the Kinoko sources, which
# libgcov cuts from each .gcda path (GCOV_PREFIX_STRIP) before adding the SD folder.
PGO_STRIP = len((OUT / 'objects' / KINOKO_SOURCE.relative_to(ROOT)).resolve().as_posix().split('/'))
# Profile use: no unrolling or peeling (the Old 3DS is instruction-cache bound); code
# the training never reached is optimized as without a profile.
PGO_USE_FLAGS = ['-fprofile-use', '-fprofile-partial-training', '-Wno-missing-profile', '-Wno-coverage-mismatch',
                 '-fno-unroll-loops', '-fno-peel-loops']
SHADER = WORK / 'shader'
# devkitPro's shells set DEVKITPRO=/opt/devkitpro, which Windows Python cannot open.
SDK = next(p for p in [Path(os.environ.get('DEVKITPRO', '')), Path('C:/devkitPro'), Path('/opt/devkitpro')]
           if str(p) not in ('', '.') and p.is_dir())
CXX = SDK / 'devkitARM/bin/arm-none-eabi-g++'

env = dict(os.environ)
env['PATH'] = os.pathsep.join([str(SDK / 'devkitARM/bin'), str(SDK / 'tools/bin'), env['PATH']])

ARCH = ['-march=armv6k', '-mtune=mpcore', '-mfloat-abi=hard', '-mtp=soft']
FLAGS = ARCH + ['-O2', '-g', '-mword-relocations', '-ffunction-sections', '-fdata-sections', '-ffp-contract=off',
                '-std=gnu++23', '-D__3DS__', '-DREVOLUTION', '-DMKW_AUTO_PROFILE', '-DMKW_WRAP_ROMFS_MOUNT', '-DKINOKO_QUIET',
                '-fno-exceptions', '-fno-rtti', '-fcheck-new', '-fno-strict-aliasing',
                '-ffile-prefix-map=' + ROOT.as_posix() + '/=']
FLAGS += {'': [], 'o3dstest': ['-DMKW_OLD3DS_TEST']}[VARIANT]
FLAGS += ['-I' + p.as_posix() for p in [SHADER, ROOT / 'vendor/kinoko/include', ROOT / 'vendor/kinoko/source', ROOT / 'include',
                                        ROOT / 'vendor/cfl-mii/include', ROOT / 'vendor/citro3d/include', SDK / 'libctru/include', GEN]]

sources = [ROOT / s.strip() for s in (ROOT / 'docs/MANUAL-PROGRAM-SOURCES.txt').read_text().splitlines()
           if s.strip() and not s.startswith('#')]


def compile_one(src, signature):
    obj = OUT / 'objects' / src.relative_to(ROOT).with_suffix('.o')
    obj.parent.mkdir(parents=True, exist_ok=True)
    flags = FLAGS + (['-flto=auto'] if src.suffix == '.cc' else [])
    profile = b''
    if src.suffix == '.cc' and src.is_relative_to(KINOKO_SOURCE):
        if PGO == 'generate':
            flags = flags + ['-fprofile-generate', '-fprofile-update=single', f'-DMKW_KINOKO_PGO_GENERATE={PGO_STRIP}']
        elif PGO != 'off' and (PGO_DIR / src.relative_to(KINOKO_SOURCE)).with_suffix('.gcda').is_file():
            # GCC reads the profile beside the object (its -o path with .gcda).
            profile = (PGO_DIR / src.relative_to(KINOKO_SOURCE)).with_suffix('.gcda').read_bytes()
            obj.with_suffix('.gcda').write_bytes(profile)
            flags = flags + PGO_USE_FLAGS
    digest = hashlib.sha256(src.read_bytes() + signature + ' '.join(flags).encode() + profile).hexdigest()
    name = str(obj.relative_to(OUT / 'objects').with_suffix(src.suffix))
    stamp = obj.with_suffix('.sha256')
    if obj.exists() and stamp.exists() and stamp.read_text() == digest:
        return name, 0, 'cached'
    # A forward-slash object path: the training build's .gcda paths are cut at '/'.
    run = subprocess.run([str(CXX), *flags, '-c', str(src), '-o', obj.as_posix()], env=env, capture_output=True, text=True)
    obj.with_suffix('.log').write_text(run.stdout + run.stderr)
    if run.returncode == 0:
        stamp.write_text(digest)
    return name, run.returncode, (run.stdout + run.stderr)[-3000:]


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    headers = sorted(list((ROOT / 'include').rglob('*.hpp')) + list((ROOT / 'include').rglob('*.inc'))
                     + list((ROOT / 'vendor/kinoko').rglob('*.hh')) + list((ROOT / 'vendor/citro3d/include').rglob('*.h'))
                     + [SHADER / 'shader_data.hpp'] + list(GEN.rglob('*.hpp')) + list(GEN.rglob('*.inc')))
    signature = hashlib.sha256(b''.join(p.read_bytes() for p in headers)).digest()
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        results = list(pool.map(lambda s: compile_one(s, signature), sources))
    failed = [r for r in results if r[1]]
    (OUT / 'compile-results.json').write_text(json.dumps(results, indent=2))
    print(f'{len(results) - len(failed)}/{len(results)} translation units compiled', flush=True)
    for name, code, log in failed:
        print(name + '\n' + log, flush=True)
    if failed:
        sys.exit(1)
    objects = [OUT / 'objects' / s.relative_to(ROOT).with_suffix('.o') for s in sources]
    rsp = OUT / 'objects.rsp'
    rsp.write_text('\n'.join('"' + p.as_posix() + '"' for p in objects))
    wrappers = ['linearAlloc', 'linearMemAlign', 'linearRealloc', 'linearGetSize', 'linearFree', 'linearSpaceFree', 'fopen',
                'romfsMountFromTitle']  # DIAG: logs the Mii resource fallback (cfl-fallback-test.txt)
    subprocess.run([str(CXX), *ARCH, '-flto=auto', *(['-fprofile-generate'] if PGO == 'generate' else []), '-specs=3dsx.specs', '-Wl,--gc-sections', '-Wl,-Map=' + str(OUT / (NAME + '.map')),
                    *['-Wl,--wrap=' + w for w in wrappers], '@' + str(rsp), str(WORK / 'libCFL.a'), str(WORK / 'libcitro3d.a'),
                    '-L' + str(SDK / 'libctru/lib'), '-lctru', '-lm', '-o', str(OUT / (NAME + '.elf'))], env=env, check=True)
    subprocess.run([str(SDK / 'devkitARM/bin/arm-none-eabi-size'), str(OUT / (NAME + '.elf'))], env=env, check=True)


if __name__ == '__main__':
    main()
