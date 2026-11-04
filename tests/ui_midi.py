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
results=[]
for label,bank in [('windows',None),('sf2',fixture/'한글 음색.sf2'),('dls',fixture/'own.dls')]:
    folder=out/label;folder.mkdir(exist_ok=True);settings=folder/'settings.ini';settings.write_text('autoplay=0\nrepeat=0\npitch=100\nspeed=100\nreverb=0\ncontrolsRevision=1415\n')
    if bank:(folder/'midi-soundfont.txt').write_text(str(bank),encoding='utf-8')
    env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='midi159-'+label,NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_SNAPSHOT_MS='20000',NGMV_TEST_NO_SNAPSHOT_PLAY='1')
    p=subprocess.Popen([str(exe),'--snapshot',str(fixture/'simultaneous.mid'),str(folder/'midi.bmp'),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No MIDI window')[0];state=lambda a:send(main,0x8000+74,a)
        wait_for(lambda:state(1) and state(2)>300,'MIDI open did not play');assert not state(13)
        send(main,0x113,2);assert p.wait(timeout=10)==0
        image=Image.open(folder/'midi.bmp').convert('RGB');image.save(folder/'midi.png')
        lanes=[]
        for color in [(247,205,87),(126,222,190)]:
            points=[i//image.width for i,c in enumerate(image.getdata()) if c==color and i%image.width>500 and 276<i//image.width<990]
            assert len(points)>100 and max(points)-min(points)>8,('Missing MIDI waveform',label,color)
            lanes.append(len(points))
        results.append({'bank':label,'auto_start':True,'separate_channel_wave_pixels':lanes})
    finally:
        if p.poll() is None:p.terminate();p.wait(timeout=5)
(out/'result.json').write_text(json.dumps({'pass':True,'cases':results,'audio':'dummy','snapshot_forced_play':False},indent=2));print(json.dumps(results))
