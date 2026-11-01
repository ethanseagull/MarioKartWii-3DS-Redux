"""HOME Menu banner for the DIAG CIA: Mario in Standard Kart M
(tools/release/banner, see docs/HOME-BANNER.md) with Mario Kart Wii's own
Starting Grid jingle (Battle, Online, Time Trials), sound/strm/o_Start2_32_fan.brstm,
as its sound.

The stream is 3.824 s of 32 kHz PCM16; a banner sound is at most 3 s. It is
resampled to 44.1 kHz, cut at 3.000 s with a 0.2 s fade so the cut doesn't
click, encoded as PCM16 CWAV by bannertool, and spliced in place of the CWAV
bannertool wrote next to the CGFX.

Inputs:
  MKW_BANNER_WORK  banner work directory; uses production/banner.cgfx
  MKW_EXTRACTED    extracted disc files (default data/extracted)
  BANNERTOOL       bannertool executable (default: on PATH); ffmpeg on PATH

    python tools/diag/make_banner.py   -> build/banner/banner-startgrid.bnr
"""
import os, shutil, struct, subprocess, sys, tempfile, wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if not os.environ.get('MKW_BANNER_WORK'):
    raise SystemExit('Set MKW_BANNER_WORK to the banner work directory (docs/HOME-BANNER.md).')
CGFX = Path(os.environ['MKW_BANNER_WORK']) / 'production/banner.cgfx'
STREAM = Path(os.environ.get('MKW_EXTRACTED', ROOT / 'data/extracted')) / 'sound/strm/o_Start2_32_fan.brstm'
BANNERTOOL = os.environ.get('BANNERTOOL') or shutil.which('bannertool') or 'bannertool'
OUT = ROOT / 'build/banner/banner-startgrid.bnr'
sys.path.insert(0, str(ROOT / 'tools'))
import brstm_to_wav  # noqa: E402


def main():
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        channels, rate, total, pcm = brstm_to_wav.decode(STREAM.read_bytes())
        source = tmp / 'start.wav'
        with wave.open(str(source), 'wb') as w:
            w.setnchannels(channels); w.setsampwidth(2); w.setframerate(rate); w.writeframes(pcm)
        trimmed = tmp / 'banner.wav'
        subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', str(source), '-af',
                        'aresample=44100:resampler=soxr,atrim=end_sample=132300,afade=t=out:st=2.8:d=0.2',
                        '-c:a', 'pcm_s16le', '-ar', '44100', str(trimmed)], check=True)
        cwav = tmp / 'banner.bcwav'
        subprocess.run([BANNERTOOL, 'makecwav', '-i', str(trimmed), '-o', str(cwav)], check=True,
                       stdout=subprocess.DEVNULL)
        sound = cwav.read_bytes()
        graphics = tmp / 'graphics.bnr'
        subprocess.run([BANNERTOOL, 'makebanner', '-ci', str(CGFX), '-a', str(trimmed), '-o', str(graphics)],
                       check=True, stdout=subprocess.DEVNULL)
        release = graphics.read_bytes()
    at = struct.unpack_from('<I', release, 0x84)[0]
    if release[at:at + 4] != b'CWAV' or sound[:4] != b'CWAV':
        raise SystemExit('unexpected banner layout')
    info = sound.find(b'INFO')
    encoding, loop = sound[info + 8], sound[info + 9]
    rate, _, end = struct.unpack_from('<III', sound, info + 12)
    if encoding != 1 or loop or rate != 44100 or end > 132300:
        raise SystemExit(f'unexpected CWAV: encoding {encoding} loop {loop} rate {rate} samples {end}')
    OUT.write_bytes(release[:at] + sound)
    print(f'{OUT}: CWAV PCM16 {rate} Hz, {end} samples ({end / rate:.3f} s), {len(release[:at]) + len(sound)} bytes')


if __name__ == '__main__':
    main()
