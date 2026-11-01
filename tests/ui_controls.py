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



from PIL import Image
settings=out/'isolated-settings.ini'
settings.write_text('speedStep=10\nreverbAmount=45\nreverbDecay=12\n',encoding='utf-8')
results=[]
for index in range(2):
    target=out/('normal.bmp' if index==0 else 'recording.bmp')
    env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='controls1415-'+str(index),
             NGMV_TEST_SNAPSHOT_MS='20000',NGMV_SETTINGS_TEST_FILE=str(settings))
    p=subprocess.Popen([str(exe),'--snapshot',str(fixture),str(target),'--size=1920x1080'],
                       env=env,creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No main window')[0]
        state=lambda a:send(main,0x8000+74,a)
        wait_for(lambda:state(0) and state(1) and state(2)>50,'No playback')
        rect=W.RECT();u.GetWindowRect(main,ctypes.byref(rect));assert rect.left < -10000
        def click(x,y,drag_to=None):
            r=W.RECT();u.GetClientRect(main,ctypes.byref(r));scale=min(r.right/1920,r.bottom/1080)
            def pos(x,y):return int((r.right-1920*scale)/2+x*scale)|(int((r.bottom-1080*scale)/2+y*scale)<<16)
            send(main,0x201,1,pos(x,y))
            if drag_to:send(main,0x200,1,pos(*drag_to))
            send(main,0x202,0,pos(*(drag_to or (x,y))))
        if index==0:
            assert [state(a) for a in [5,6,7,8,9]]==[100,100,76,30,1],'New defaults/migration failed'
            click(325,230);assert state(6)==101 and state(5)==100,'Pitch not independent/0.01'
            click(755,230);assert state(5)==101 and state(6)==101,'Speed not independent/0.01'
            click(412,230);assert state(6)==100 and state(5)==101,'Pitch reset affected speed'
            click(840,230);assert state(5)==100 and state(6)==100,'Speed reset affected pitch'
            click(1000,230);assert state(9)==10
            click(755,230);assert state(5)==110 and state(6)==100
            click(840,230);click(1000,230);assert state(9)==1
            click(100,284);assert state(4)==1
            click(238,1054);assert state(10)==95 and state(11)==1 and state(4)==1 and state(3)==100,'Group gain changed mutes/master'
            click(320,1054,(417,1054));assert state(10)==150 and state(11)==1 and state(4)==1
            click(1288,230);assert state(7)==71
            click(1715,230);assert state(8)==29
            click(325,230);click(755,230);click(755,230)
            assert state(6)==101 and state(5)==102
            before=state(2);time.sleep(.25);assert state(2)>before+100 and state(12)==0 and state(13)==0
            results.append({'defaults_migrated':True,'pitch_speed_independent_buttons':True,'step001_default':True,'group_gain_preserved_mute_master':True,'live_audio_no_underruns':True})
        else:
            assert [state(a) for a in [5,6,7,8,9,10,11,4]]==[102,101,71,29,1,150,1,1],'Saved/reopened controls lost'
            send(main,0x100,0x78) # F9
            before=[state(a) for a in [3,10,5,6]]
            click(1565,178);click(417,1054);click(325,230)
            assert [state(a) for a in [3,10,5,6]]==before,'Hidden recording controls still react'
            results.append({'saved_reopened':True,'recording_hidden_controls_inert':True})
        send(main,0x113,2);assert p.wait(timeout=10)==0
        assert target.stat().st_size>100000
    finally:
        if p.poll() is None:p.terminate();p.wait(timeout=5)
image=Image.open(out/'recording.bmp').convert('RGB')
clock=sum(1 for pixel in image.crop((40,125,1300,197)).getdata() if pixel==(232,206,122))
volume=sum(1 for pixel in image.crop((1540,125,1919,199)).getdata() if pixel==(80,203,180))
assert clock>1200,('Clock is not large',clock)
assert volume==0,('Master volume drawn in recording view',volume)
result={'pass':True,'audio':'dummy','offscreen':True,'user_app_operated':False,'large_clock_pixels':clock,'recording_volume_pixels':volume,'cases':results}
(out/'result.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps(result))
