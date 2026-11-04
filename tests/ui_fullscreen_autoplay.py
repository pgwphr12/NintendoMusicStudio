"""Repeat a real decoded file in own hidden player; dummy audio/isolated settings."""
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



settings=out/'settings.ini';settings.write_text('repeat=0\nautoplay=1\npitch=100\nspeed=100\nreverb=0\ncontrolsRevision=1415\n')
env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='fullscreen156-autoplay',NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_SNAPSHOT_MS='20000',NGMV_TEST_DISABLE_PLAYBACK_TIMER='1')
p=subprocess.Popen([str(exe),'--snapshot',str(fixture),str(out/'autoplay.bmp'),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
try:
    main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No main')[0];state=lambda a:send(main,0x8000+74,a)
    wait_for(lambda:state(0) and state(1),'No DS playback');send(main,0x100,0x7a);assert state(14)
    u.SetWindowPos.argtypes=[W.HWND,W.HWND,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,W.UINT]
    assert u.SetWindowPos(main,None,-20000,-20000,3840,2160,0x0034)
    def title():
        value=ctypes.create_unicode_buffer(1024);u.GetWindowTextW(main,value,1024);return value.value
    seen=[];deadline=time.monotonic()+9
    while time.monotonic()<deadline:
        name=title();assert not state(13),'Decoder error during DS fullscreen autoplay'
        for tag in ['02 Track','10 Track']:
            if tag in name and tag not in seen:seen.append(tag)
        if seen==['02 Track','10 Track']:break
        time.sleep(.012)
    assert seen==['02 Track','10 Track'],seen
    assert state(14) and state(1),'Fullscreen/playback changed during advance'
    (out/'result.json').write_text(json.dumps({'pass':True,'auto_advance':seen,'fullscreen':True,'timer1_delivery':'disabled','audio':'dummy'},indent=2));print(json.dumps(seen))
    send(main,0x100,0x7a);send(main,0x113,2);assert p.wait(timeout=10)==0
finally:
    if p.poll() is None:p.terminate();p.wait(timeout=5)
