"""Modeless About regression; only exact Popen children, offscreen, dummy audio.

Read-only observations verify that native mouse hit testing actually changes
playback/mute/volume while About is open, rather than merely delivering messages.
"""
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



settings=out/'settings.ini'
settings.write_text('autoplay=1\nreverb=0\nrecording=0\ncontrolsRevision=1415\n')
env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='footer162t7',NGMV_TEST_SNAPSHOT_MS='20000',NGMV_SETTINGS_TEST_FILE=str(settings))
p=subprocess.Popen([str(exe),'--snapshot',str(fixture),str(out/'normal.bmp'),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
try:
    main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No main')[0]
    state=lambda a:send(main,0x8000+74,a)
    wait_for(lambda:state(0) and state(1) and state(2)>100,'No playback')
    def click(x,y):
        r=W.RECT();u.GetClientRect(main,ctypes.byref(r));scale=min(r.right/1920,r.bottom/1080)
        pos=int((r.right-1920*scale)/2+x*scale)|(int((r.bottom-1080*scale)/2+y*scale)<<16)
        send(main,0x201,1,pos);send(main,0x202,0,pos)
    click(1285,1050)
    assert not windows(p.pid,'NintendoMusicStudioEqualizer'),'Reverb click opened EQ'
    click(1760,1050)
    eq=wait_for(lambda:windows(p.pid,'NintendoMusicStudioEqualizer'),'EQ new hitbox failed')[0]
    assert u.IsWindowEnabled(main)
    send(eq,0x10)
    send(main,0x113,2);assert p.wait(timeout=8)==0
    assert 'reverb=1' in settings.read_text(),'EQ click changed reverb or reverb hitbox failed'
    from PIL import Image
    image=Image.open(out/'normal.bmp').convert('RGB');image.save(out/'normal.png');image.crop((1200,1020,1890,1080)).save(out/'footer.png')
    result={'pass':True,'reverb_button_unchanged':True,'EQ_separate_right_hitbox':True,'main_responsive':True,'audio':'dummy','only_owned_process_settings':True}
    (out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
finally:
    if p.poll() is None:p.terminate();p.wait(timeout=5)
