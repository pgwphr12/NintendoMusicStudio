"""Own hidden ordinary-audio playback/waveforms and EOF clock test; dummy audio."""
import ctypes
from ctypes import wintypes as W
import json
import os
from pathlib import Path
import subprocess
import sys
import time

exe, fixture, out = map(lambda p: Path(p).resolve(), sys.argv[1:4])
out.mkdir(parents=True, exist_ok=True)
u = ctypes.WinDLL('user32', use_last_error=True)
CB = ctypes.WINFUNCTYPE(W.BOOL, W.HWND, W.LPARAM)
u.EnumWindows.argtypes = [CB, W.LPARAM]
u.GetWindowThreadProcessId.argtypes = [W.HWND, ctypes.POINTER(W.DWORD)]
u.GetClassNameW.argtypes = [W.HWND, W.LPWSTR, ctypes.c_int]
u.GetWindowTextW.argtypes = [W.HWND, W.LPWSTR, ctypes.c_int]
u.IsWindowEnabled.argtypes = [W.HWND]
u.IsWindow.argtypes = [W.HWND]
u.GetDlgItem.argtypes = [W.HWND, ctypes.c_int]
u.GetDlgItem.restype = W.HWND
u.GetClientRect.argtypes = [W.HWND, ctypes.POINTER(W.RECT)]
u.GetWindowRect.argtypes = [W.HWND, ctypes.POINTER(W.RECT)]
u.PostMessageW.argtypes = [W.HWND, W.UINT, W.WPARAM, W.LPARAM]
u.SendMessageTimeoutW.argtypes = [W.HWND, W.UINT, W.WPARAM, W.LPARAM,
                                W.UINT, W.UINT, ctypes.POINTER(ctypes.c_size_t)]


def windows(pid, cls):
    found = []
    @CB
    def visit(h, _):
        p = W.DWORD()
        u.GetWindowThreadProcessId(h, ctypes.byref(p))
        if p.value == pid:
            c = ctypes.create_unicode_buffer(256)
            u.GetClassNameW(h, c, len(c))
            if c.value == cls:
                found.append(h)
        return True
    u.EnumWindows(visit, 0)
    return found


def wait_for(fn, text, seconds=4):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        value = fn()
        if value:
            return value
        time.sleep(.015)
    raise AssertionError(text)


def send(h, msg, a=0, b=0):
    value = ctypes.c_size_t()
    ok = u.SendMessageTimeoutW(h, msg, a, b, 2, 1500, ctypes.byref(value))
    assert ok, ('Unresponsive owned window', msg, ctypes.get_last_error())
    return value.value



from PIL import Image
settings=out/'settings.ini';settings.write_text('repeat=0\nautoplay=0\npitch=100\nspeed=100\nreverb=0\ncontrolsRevision=1415\n')
results=[]
for recording in [False,True]:
    target=out/('recording.bmp' if recording else 'normal.bmp')
    args=[str(exe),'--snapshot',str(fixture),str(target),'--size=1920x1080']
    if recording:args.append('--recording')
    env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='recording155-'+str(recording),NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_SNAPSHOT_MS='20000')
    process=subprocess.Popen(args,env=env,creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        main=wait_for(lambda:windows(process.pid,'NintendoMusicStudioWindow'),'No window')[0]
        state=lambda a:send(main,0x8000+74,a);wait_for(lambda:state(0) and state(2)>250,'No audio')
        send(main,0x113,2);assert process.wait(timeout=10)==0
        image=Image.open(target).convert('RGB');image.save(target.with_suffix('.png'))
        region=image.crop((38,320,275,580));points=[(i%region.width,i//region.width) for i,c in enumerate(region.getdata()) if c==(247,205,87)]
        if recording:assert len(points)>100 and max(y for x,y in points)-min(y for x,y in points)>20,'Recording waveform did not enter name area'
        right=image.crop((1856,300,1888,600));edge=[(i%right.width,i//right.width) for i,c in enumerate(right.getdata()) if c==(247,205,87)]
        if recording:assert len(edge)>25 and max(x for x,y in edge)>=30,'Recording wave did not reach channel cell edge'
        else:assert not edge,'Normal-mode right margin changed'
        outside=image.crop((1888,300,1920,600));assert not any(c==(247,205,87) for c in outside.getdata()),'Wave escaped channel cell'
        if recording:
            label=image.crop((54,205,180,250));label_ys=[i//label.width for i,c in enumerate(label.getdata()) if c==(247,205,87)]
            assert label_ys and min(label_ys)<12,'Recording label is too low'
        results.append({'recording':recording,'name_area_wave_pixels':len(points),'right_edge_wave_pixels':len(edge)})
    finally:
        if process.poll() is None:process.terminate();process.wait(timeout=5)
assert results[0]['name_area_wave_pixels']==0,'Normal mode wave moved into names'
(out/'result.json').write_text(json.dumps({'pass':True,'cases':results,'audio':'dummy'},indent=2));print(json.dumps(results))
