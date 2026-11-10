"""Own offscreen/dummy render benchmark and cached-label reference comparison.

Arguments: executable, local fixture, private output directory. No user settings
are changed, and only the subprocess created here is terminated on a timeout.
Timing is measured CPU/native drawing cost, not physical monitor FPS.
"""
from pathlib import Path
import csv, json, os, subprocess, sys
from PIL import Image, ImageChops, ImageStat

exe, fixture, output = [Path(p).resolve() for p in sys.argv[1:4]]
output.mkdir(exist_ok=True, parents=True)
results = []
for mode, show, reference, paused in [
    ('cached-auto', 0, 0, 0), ('cached-all', 1, 0, 0),
    ('pixel-cached', 1, 0, 1), ('pixel-gdi', 1, 1, 1)
]:
    folder = output / mode
    folder.mkdir(exist_ok=True)
    settings = folder / 'settings.ini'
    settings.write_text(f'showAllChannels={show}\nspectrumView=0\nreverb=0\nrepeat=0\ncontrolsRevision=1415\n')
    env = dict(os.environ, SDL_AUDIODRIVER='dummy', NGMV_TEST_SESSION='smooth-final',
               NGMV_SETTINGS_TEST_FILE=str(settings))
    if not paused:
        env['NGMV_RENDER_BENCHMARK'] = '1'
    else:
        env['NGMV_TEST_NO_SNAPSHOT_PLAY'] = '1'
    if reference:
        env['NGMV_LABEL_GDI_BASELINE'] = '1'
    shot = folder / 'view.bmp'
    child = subprocess.Popen([str(exe), '--snapshot', str(fixture), str(shot),
                              '--recording', '--size=1920x1080'], env=env,
                             creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        assert child.wait(timeout=30) == 0
    finally:
        if child.poll() is None:
            child.terminate()
            child.wait(timeout=5)
    if not paused:
        with Path(str(shot) + '.render.csv').open() as report:
            result = next(csv.DictReader(report))
        assert result['gdi_before'] == result['gdi_after'], result
        results.append(dict(mode=mode, **result))
a = Image.open(output/'pixel-cached/view.bmp').convert('RGB')
b = Image.open(output/'pixel-gdi/view.bmp').convert('RGB')
diff = ImageChops.difference(a, b)
stats = {'max_channel_error': max(v[1] for v in diff.getextrema()),
         'mean_channels': ImageStat.Stat(diff).mean}
assert stats['max_channel_error'] <= 5 and max(stats['mean_channels']) < .1, stats
result = {'pass': True, 'render': results, 'label_compare': stats, 'audio': 'dummy',
          'physical_refresh_test': False}
(output/'result.json').write_text(json.dumps(result, indent=2))
print(json.dumps(result))
