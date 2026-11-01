"""Compile the diagnostic ELF from this source tree -> build/diag/mkw-3ds-diag.elf:
  - MKW_RELEASE omitted: QA layer (runtime.log, menu profiler, load-stage and
    slow-job timings, sound stats, full-quality.txt toggle, subsystem logs).
  - MKW_AUTO_PROFILE: every race is profiled automatically from the end of
    the intro to results; per-second, spike and summary CSVs are written to
    sdmc:/3ds/mkw-3ds/ on pause, race end and exit.

Inputs outside the tree, from the manual build (docs/MANUAL-BUILD.md):
  MKW_GENERATED   directory with the manual build's generated headers,
                  libCFL.a and the unpatched libcitro3d.a
  MKW_GENERATED_INCLUDE
                  directory holding the generated mkw/*.hpp tables (cooked
                  from the disc; default MKW_GENERATED). Searched after
                  include/, so this tree's own headers always win.
  DEVKITPRO       devkitPro root (falls back to C:/devkitPro, /opt/devkitpro)

    python tools/diag/compile.py
"""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import hashlib, json, os, subprocess, sys

ROOT = Path(__file__).resolve().parents[2]
if not os.environ.get('MKW_GENERATED'):
    raise SystemExit('Set MKW_GENERATED to the manual build\'s generated-header directory.')
GEN = Path(os.environ['MKW_GENERATED']).resolve()
GEN_INCLUDE = Path(os.environ.get('MKW_GENERATED_INCLUDE', GEN)).resolve()
OUT = ROOT / 'build/diag'
NAME = 'mkw-3ds-diag'
SHADER = OUT / 'shader'       # shader_data.hpp, ahead of GEN on the include path
CITRO3D = ROOT / 'vendor/citro3d/source'
# citro3d members patched here (pipelined frames, texture unit 2 on coordinate 1).
CITRO3D_PATCHED = ['framebuffer.c', 'renderqueue.c', 'texunit.c']
# devkitPro's shells set DEVKITPRO=/opt/devkitpro, which Windows Python cannot open.
SDK = next(p for p in [Path(os.environ.get('DEVKITPRO', '')), Path('C:/devkitPro'), Path('/opt/devkitpro')]
           if str(p) not in ('', '.') and p.is_dir())
CXX = SDK / 'devkitARM/bin/arm-none-eabi-g++'
CC = SDK / 'devkitARM/bin/arm-none-eabi-gcc'
AR = SDK / 'devkitARM/bin/arm-none-eabi-ar'

env = dict(os.environ)
env['PATH'] = os.pathsep.join([str(SDK / 'devkitARM/bin'), str(SDK / 'tools/bin'), env['PATH']])

ARCH = ['-march=armv6k', '-mtune=mpcore', '-mfloat-abi=hard', '-mtp=soft']
FLAGS = ARCH + ['-O2', '-g', '-mword-relocations', '-ffunction-sections', '-fdata-sections', '-ffp-contract=off',
                '-std=gnu++23', '-D__3DS__', '-DREVOLUTION', '-DMKW_AUTO_PROFILE', '-DKINOKO_QUIET',
                '-fno-exceptions', '-fno-rtti', '-fcheck-new', '-fno-strict-aliasing',
                '-ffile-prefix-map=' + ROOT.as_posix() + '/=']
FLAGS += ['-I' + p.as_posix() for p in [SHADER, ROOT / 'vendor/kinoko/include', ROOT / 'vendor/kinoko/source', ROOT / 'include',
                                        ROOT / 'vendor/cfl-mii/include', ROOT / 'vendor/citro3d/include', SDK / 'libctru/include', GEN, GEN_INCLUDE]]

sources = [ROOT / s.strip() for s in (ROOT / 'docs/MANUAL-PROGRAM-SOURCES.txt').read_text().splitlines()
           if s.strip() and not s.startswith('#')]


def shader():
    """shader_data.hpp from src/mkw.v.pica, effect.v.pica and the Mii variant
    (the production order, docs/MANUAL-BUILD.md)."""
    SHADER.mkdir(parents=True, exist_ok=True)
    shbin = SHADER / 'mkw.shbin'
    # The Mii program must keep every scene uniform at the same register
    # (Renderer::init checks), so derive it from the scene shader.
    sys.path.insert(0, str(ROOT / 'tools'))
    import mii_shader
    scene = ROOT / 'src/mkw.v.pica'
    mii = SHADER / 'native_mii.v.pica'
    mii.write_bytes(mii_shader.variant(scene.read_bytes().decode().replace('\r\n', '\n')).replace('\n', '\r\n').encode())
    subprocess.run([str(SDK / 'tools/bin/picasso'), '-o', str(shbin), str(scene),
                    str(ROOT / 'src/effect.v.pica'), str(mii)], env=env, check=True)
    data = shbin.read_bytes()
    rows = [','.join(f'0x{b:02x}' for b in data[i:i + 16]) + ',' for i in range(0, len(data), 16)]
    text = ('#pragma once\nstatic const unsigned char mkw_shader[] __attribute__((aligned(4))) = {\n' + '\n'.join(rows)
            + f'\n}};\nstatic const unsigned int mkw_shader_size = {len(data)};\n')
    header = SHADER / 'shader_data.hpp'
    if not header.exists() or header.read_text() != text:
        header.write_text(text)


def compile_one(src, signature):
    obj = OUT / 'objects' / src.relative_to(ROOT).with_suffix('.o')
    obj.parent.mkdir(parents=True, exist_ok=True)
    flags = FLAGS + (['-flto=auto'] if src.suffix == '.cc' else [])
    digest = hashlib.sha256(src.read_bytes() + signature + ' '.join(flags).encode()).hexdigest()
    name = str(obj.relative_to(OUT / 'objects').with_suffix(src.suffix))
    stamp = obj.with_suffix('.sha256')
    if obj.exists() and stamp.exists() and stamp.read_text() == digest:
        return name, 0, 'cached'
    run = subprocess.run([str(CXX), *flags, '-c', str(src), '-o', str(obj)], env=env, capture_output=True, text=True)
    obj.with_suffix('.log').write_text(run.stdout + run.stderr)
    if run.returncode == 0:
        stamp.write_text(digest)
    return name, run.returncode, (run.stdout + run.stderr)[-3000:]


def citro3d():
    """libcitro3d.a: the manual build's archive with the patched members recompiled."""
    config = json.loads((ROOT / 'docs/MANUAL-LIBRARY-FLAGS.json').read_text())['hardware_sdk']['flags']
    flags = [f.replace('<SOURCE_ROOT>', ROOT.as_posix()).replace('<DEVKITPRO>', SDK.as_posix()) for f in config]
    archive = OUT / 'libcitro3d.a'
    srcs = [CITRO3D / n for n in CITRO3D_PATCHED]
    inputs = [GEN / 'libcitro3d.a', *srcs, *CITRO3D.glob('*.h'), *(ROOT / 'vendor/citro3d/include/c3d').glob('*.h')]
    digest = hashlib.sha256(b''.join(p.read_bytes() for p in inputs) + ' '.join(flags).encode()).hexdigest()
    stamp = OUT / 'libcitro3d.sha256'
    if archive.exists() and stamp.exists() and stamp.read_text() == digest:
        return
    objdir = OUT / 'citro3d'
    objdir.mkdir(parents=True, exist_ok=True)
    objects = []
    for src in srcs:
        obj = objdir / src.with_suffix('.o').name
        subprocess.run([str(CC), *flags, '-c', str(src), '-o', str(obj)], env=env, check=True)
        objects.append(obj)
    archive.write_bytes((GEN / 'libcitro3d.a').read_bytes())
    subprocess.run([str(AR), 'rs', str(archive), *map(str, objects)], env=env, check=True)
    stamp.write_text(digest)
    print(f'libcitro3d.a: {len(objects)} members replaced', flush=True)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    shader()
    headers = sorted(list((ROOT / 'include').rglob('*.hpp')) + list((ROOT / 'include').rglob('*.inc'))
                     + list((ROOT / 'vendor/kinoko').rglob('*.hh')) + list((ROOT / 'vendor/citro3d/include').rglob('*.h'))
                     + [SHADER / 'shader_data.hpp'] + list(GEN.glob('*.hpp')) + list(GEN_INCLUDE.glob('mkw/*.hpp')))
    signature = hashlib.sha256(b''.join(p.read_bytes() for p in headers)).digest()
    citro3d()
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
    wrappers = ['linearAlloc', 'linearMemAlign', 'linearRealloc', 'linearGetSize', 'linearFree', 'linearSpaceFree', 'fopen']
    subprocess.run([str(CXX), *ARCH, '-flto=auto', '-specs=3dsx.specs', '-Wl,--gc-sections', '-Wl,-Map=' + str(OUT / (NAME + '.map')),
                    *['-Wl,--wrap=' + w for w in wrappers], '@' + str(rsp), str(GEN / 'libCFL.a'), str(OUT / 'libcitro3d.a'),
                    '-L' + str(SDK / 'libctru/lib'), '-lctru', '-lm', '-o', str(OUT / (NAME + '.elf'))], env=env, check=True)
    subprocess.run([str(SDK / 'devkitARM/bin/arm-none-eabi-size'), str(OUT / (NAME + '.elf'))], env=env, check=True)


if __name__ == '__main__':
    main()
