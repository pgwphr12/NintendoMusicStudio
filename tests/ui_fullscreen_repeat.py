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



from PIL import Image
settings=out/'settings.ini';settings.write_text('repeat=0\nautoplay=0\npitch=100\nspeed=100\nreverb=0\ncontrolsRevision=1415\n')
target=out/'fullscreen.bmp';env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='fullscreen156',NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_SNAPSHOT_MS='20000',NGMV_TEST_DISABLE_PLAYBACK_TIMER='1')
p=subprocess.Popen([str(exe),'--snapshot',str(fixture),str(target),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
u.SetWindowPos.argtypes=[W.HWND,W.HWND,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,W.UINT]
results=[]
try:
    main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No main')[0]
    state=lambda a:send(main,0x8000+74,a);wait_for(lambda:state(0) and state(1),'No playback')
    rect=W.RECT();u.GetClientRect(main,ctypes.byref(rect));scale=min(rect.right/1920,rect.bottom/1080)
    x=int((rect.right-1920*scale)/2+994*scale);y=int((rect.bottom-1080*scale)/2+165*scale)
    send(main,0x201,1,x|(y<<16));send(main,0x202,0,x|(y<<16));assert state(16),'Repeat click failed'
    # Actual F11 handler; isolated snapshots keep the fullscreen-sized window offscreen.
    send(main,0x100,0x7a);assert state(14),'F11 did not enter fullscreen'
    assert u.SetWindowPos(main,None,-20000,-20000,3840,2160,0x0034)
    r=W.RECT();u.GetWindowRect(main,ctypes.byref(r));assert r.left < -10000,'Test window unexpectedly visible'
    phases=[('fullscreen',None),('fullscreen-recording',0x78),('fullscreen-normal-again',0x78),('windowed',0x7a),('fullscreen-again',0x7a)]
    for label,key in phases:
        if key:send(main,0x100,key)
        previous=state(2);maximum=previous;wrap=False;deadline=time.monotonic()+5
        while time.monotonic()<deadline:
            current=state(2);maximum=max(maximum,current)
            assert state(13)==0 and state(1),'Playback error/stopped during fullscreen'
            if current+80<previous:wrap=True;break
            previous=current;time.sleep(.012)
        result={'phase':label,'wrap':wrap,'maximum_ms':maximum,'fullscreen':bool(state(14)),'recording':bool(state(15))}
        results.append(result);(out/'result.json').write_text(json.dumps({'pass':all(x['wrap'] for x in results),'cases':results,'timer1_delivery':'disabled in own snapshot loop','audio':'dummy'},indent=2));print(json.dumps(result),flush=True)
        assert wrap,'No repeat without idle timer: '+label
    send(main,0x100,0x7a) # restore our own window before capture/close
    send(main,0x113,2);assert p.wait(timeout=10)==0
finally:
    if p.poll() is None:p.terminate();p.wait(timeout=5)
