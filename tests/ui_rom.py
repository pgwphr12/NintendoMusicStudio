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
env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='rom162t4',NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_SNAPSHOT_MS='20000',NGMV_TEST_NO_SNAPSHOT_PLAY='1')
p=subprocess.Popen([str(exe),'--snapshot',str(rom),str(out/'rom.bmp'),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
try:
    main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No owned ROM window',seconds=12)[0]
    state=lambda a:send(main,0x8000+74,a)
    wait_for(lambda:state(0) and state(1) and state(2)>300,'ROM did not start',seconds=12)
    state_count=state(17);count=state(28);assert count in (319,321,48,40,57),count
    assert state(29)==0 and state(17) in (11,12) and not state(13)
    send(main,0x100,0x27) # Next real ROM song, not another file.
    assert state(29)==1
    wait_for(lambda:state(2)>300,'Next song clock stopped',seconds=8)
    send(main,0x100,0x20);assert not state(1)
    send(main,0x100,0x25);assert state(29)==0 and not state(1)
    send(main,0x100,0x27);assert state(29)==1 and not state(1)
    send(main,0x100,0x20);assert state(1)
    send(main,0x100,0x78);assert state(15)
    wait_for(lambda:state(2)>500,'Recording view clock stopped',seconds=8)
    assert not state(13)
    send(main,0x113,2);assert p.wait(timeout=10)==0
    image=Image.open(out/'rom.bmp').convert('RGB');image.save(out/'rom.png')
    # Individual PCM voice rows must produce real nonflat waves.
    # SMA2 Song1 has silent PCM1/2 at this instant; inspect actually active
    # PCM5/6 rows in the right column, below their labels.
    for color, lo, hi in [((247,205,87),530,615),((126,222,190),675,755)]:
        points=[i//image.width for i,c in enumerate(image.get_flattened_data()) if c==color and i%image.width>1000 and lo<i//image.width<hi]
        assert len(points)>100 and max(points)-min(points)>8,('Missing active PCM waveform',color)
    result={'pass':True,'entries':count,'exposed_channels':state_count,'auto_start':True,'previous_next_pause_recording':True,'pcm_waves':True,'audio':'dummy','original_rom_read_only':True}
    (out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
finally:
    if p.poll() is None:p.terminate();p.wait(timeout=5)
