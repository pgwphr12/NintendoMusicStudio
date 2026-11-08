"""Optional read-only supplied-ROM native integration; dummy audio, own process only."""
import ctypes
from ctypes import wintypes as W
import json
import os
from pathlib import Path
import subprocess
import sys
import time

exe, rom, out = map(lambda p: Path(p).resolve(), sys.argv[1:4])
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
settings=out/'settings.ini'
settings.write_text('autoplay=1\nrepeat=1\npitch=100\nspeed=100\nreverb=0\ncontrolsRevision=1415\n')
env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='nesrom162t3',NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_SNAPSHOT_MS='20000',NGMV_TEST_NO_SNAPSHOT_PLAY='1')
p=subprocess.Popen([str(exe),'--snapshot',str(rom),str(out/'nes.bmp'),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
try:
    main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No owned NES ROM window',seconds=12)[0]
    state=lambda a:send(main,0x8000+74,a)
    wait_for(lambda:state(0) and state(1) and state(2)>300,'NES ROM auto-start failed',seconds=12)
    assert state(28)==14 and state(17)==5 and state(20)==5 and state(29)==0 and not state(13)
    send(main,0x100,0x27);assert state(29)==1 and state(20)==5
    send(main,0x100,0x27);assert state(29)==2 and state(20)==4
    send(main,0x100,0x20);assert not state(1)
    send(main,0x100,0x25);assert state(29)==1 and state(20)==5 and not state(1)
    send(main,0x100,0x27);assert state(29)==2 and state(20)==4 and not state(1)
    send(main,0x100,0x20);assert state(1)
    send(main,0x100,0x78);assert state(15)
    wait_for(lambda:state(2)>800,'NES recording-view clock stopped',seconds=8)
    assert not state(13) and state(20)==4
    send(main,0x113,2);assert p.wait(timeout=10)==0
    image=Image.open(out/'nes.bmp').convert('RGB');image.save(out/'nes.png')
    nonflat=[]
    for color in [(247,205,87),(126,222,190),(133,180,255),(222,146,232)]:
        points=[i//image.width for i,c in enumerate(image.getdata()) if c==color and i%image.width>500 and 210<i//image.width<1040]
        if len(points)>100 and max(points)-min(points)>8:nonflat.append(color)
    assert nonflat,'No original NES APU waveform'
    result={'pass':True,'entries':14,'source_apu_channels':5,'fixed_visible_by_track':[5,5,4],'auto_start':True,'next_previous_pause_recording':True,'nonflat_base_apu_lanes':len(nonflat),'audio':'dummy','only_owned_window_and_settings':True}
    (out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
finally:
    if p.poll() is None:p.terminate();p.wait(timeout=5)
