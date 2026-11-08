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




settings=out/'settings.ini'
settings.write_text('autoplay=1\nrepeat=1\nreverb=0\ncontrolsRevision=1415\n')
env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='rom162t5',NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_SNAPSHOT_MS='20000',NGMV_TEST_NO_SNAPSHOT_PLAY='1')
p=subprocess.Popen([str(exe),'--snapshot',str(rom),str(out/'stable.bmp'),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
try:
    main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No owned window',seconds=12)[0]
    state=lambda a,b=0:send(main,0x8000+74,a,b)
    wait_for(lambda:state(0) and state(2)>200,'No playback',seconds=12)
    assert state(17)==11 and state(28) in (319,321)
    # Arrow navigation across the previously sparse Song199/200 boundary.
    for i in range(30):
        send(main,0x100,0x27)
        assert p.poll() is None and state(17)==11 and not state(13),(i,p.poll())
        assert state(19,10)==100
    for i in range(4):
        send(main,0x100,0x25)
        assert p.poll() is None and state(17)==11 and not state(13)
    send(main,0x100,0x78)
    assert state(15)
    send(main,0x113,2)
    assert p.wait(timeout=8)==0
    result={'pass':True,'next_transitions':30,'previous_transitions':4,'fixed_PCM_layout':11,'recording_view':True,'only_owned_process_and_settings':True,'audio':'dummy'}
    (out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
finally:
    if p.poll() is None:p.terminate();p.wait(timeout=5)
