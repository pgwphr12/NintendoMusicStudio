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
settings=out/'isolated-settings.ini';settings.write_text('autoplay=0\nrepeat=0\npitch=100\nspeed=100\nreverb=0\ncontrolsRevision=1415\n',encoding='utf-8')
results=[]
for ext in ['wav','mp3','ogg','flac']:
    target=out/(ext+'.bmp')
    env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='ordinary154-'+ext,NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_SNAPSHOT_MS='20000')
    process=subprocess.Popen([str(exe),'--snapshot',str(fixture/('tone.'+ext)),str(target),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        main=wait_for(lambda:windows(process.pid,'NintendoMusicStudioWindow'),'No player')[0]
        state=lambda a:send(main,0x8000+74,a)
        wait_for(lambda:state(0) and state(1) and state(2)>300,'No ordinary audio playback')
        title=ctypes.create_unicode_buffer(1024);u.GetWindowTextW(main,title,1024);assert 'tone.'+ext in title.value
        send(main,0x113,2);assert process.wait(timeout=10)==0
        image=Image.open(target).convert('RGB');image.save(target.with_suffix('.png'))
        bands=[]
        for color in [(247,205,87),(126,222,190)]:
            points=[(i%image.width,i//image.width) for i,c in enumerate(image.getdata()) if c==color and i%image.width>500 and 270<i//image.width<980]
            assert len(points)>100 and max(y for x,y in points)-min(y for x,y in points)>8,(ext,color,'Flat or missing waveform')
            bands.append(len(points))
        results.append({'format':ext,'playing':True,'nonflat_stereo_waveforms':True,'wave_pixels':bands})
    finally:
        if process.poll() is None:process.terminate();process.wait(timeout=5)
target=out/'after-end.bmp'
process=subprocess.Popen([str(exe),'--snapshot',str(fixture/'tone.wav'),str(target)],env=dict(env,NGMV_TEST_SESSION='ordinary154-end'),creationflags=subprocess.CREATE_NO_WINDOW)
try:
    main=wait_for(lambda:windows(process.pid,'NintendoMusicStudioWindow'),'No EOF player')[0]
    state=lambda a:send(main,0x8000+74,a)
    wait_for(lambda:state(2)>2300,'Waveform clock froze at audio end',seconds=6)
    before=state(2);time.sleep(.15);assert state(2)>before+80 and state(13)==0
    send(main,0x113,2);assert process.wait(timeout=10)==0
finally:
    if process.poll() is None:process.terminate();process.wait(timeout=5)
result={'pass':True,'audio':'dummy','offscreen':True,'user_apps_operated':False,'cases':results,'clock_after_actual_EOF':'PASS'}
(out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
