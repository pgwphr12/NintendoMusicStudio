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



settings=out/'eq.ini'
settings.write_text('autoplay=1\nreverb=0\ncontrolsRevision=1415\neqEnabled=0\neqBand0=35\n')
results=[]
for session in range(2):
    env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='mesence162t8-'+str(session),NGMV_TEST_SNAPSHOT_MS='20000',NGMV_SETTINGS_TEST_FILE=str(settings))
    p=subprocess.Popen([str(exe),'--snapshot',str(fixture),str(out/('main'+str(session)+'.bmp')),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No main')[0]
        state=lambda a:send(main,0x8000+74,a)
        wait_for(lambda:state(0) and state(1) and state(2)>100,'No playback')
        def click(x,y):
            r=W.RECT();u.GetClientRect(main,ctypes.byref(r));scale=min(r.right/1920,r.bottom/1080)
            pos=int((r.right-1920*scale)/2+x*scale)|(int((r.bottom-1080*scale)/2+y*scale)<<16)
            send(main,0x201,1,pos);send(main,0x202,0,pos)
        click(1760,1050);eq=wait_for(lambda:windows(p.pid,'NintendoMusicStudioEqualizer'),'No EQ')[0]
        def signed(v):return ctypes.c_ssize_t(v).value
        for i in range(20):
            slider=u.GetDlgItem(eq,2300+i);edit=u.GetDlgItem(eq,2400+i)
            assert slider and edit
            assert signed(send(slider,0x401))==-20 and signed(send(slider,0x402))==20
            assert send(slider,0x418)==1 and send(slider,0x416)==5
        def press(id):send(u.GetDlgItem(eq,id),0xf5)
        if session==0:
            assert not u.IsWindowEnabled(u.GetDlgItem(eq,2300)) and not u.IsWindowEnabled(u.GetDlgItem(eq,2400))
            # Legacy3.5dB migrates to integer4dB.
            assert signed(send(u.GetDlgItem(eq,2300),0x400))==-4
            press(2200);assert u.IsWindowEnabled(u.GetDlgItem(eq,2300)) and u.IsWindowEnabled(u.GetDlgItem(eq,2400))
            edit=u.GetDlgItem(eq,2400)
            text=ctypes.create_unicode_buffer('15');send(edit,0xc,0,ctypes.addressof(text));assert signed(send(u.GetDlgItem(eq,2300),0x400))==-15
            text=ctypes.create_unicode_buffer('-20');send(edit,0xc,0,ctypes.addressof(text));assert signed(send(u.GetDlgItem(eq,2300),0x400))==20
            text=ctypes.create_unicode_buffer('99');send(edit,0xc,0,ctypes.addressof(text));assert signed(send(u.GetDlgItem(eq,2300),0x400))==-20
            text=ctypes.create_unicode_buffer('');send(edit,0xc,0,ctypes.addressof(text));assert signed(send(u.GetDlgItem(eq,2300),0x400))==0
            text=ctypes.create_unicode_buffer('-0');send(edit,0xc,0,ctypes.addressof(text));assert signed(send(u.GetDlgItem(eq,2300),0x400))==1
            text=ctypes.create_unicode_buffer('-99');send(edit,0xc,0,ctypes.addressof(text));assert signed(send(u.GetDlgItem(eq,2300),0x400))==20
            slider=u.GetDlgItem(eq,2319);send(slider,0x405,1,-20);send(eq,0x115,8,slider)
            values=settings.read_text();assert 'eqBand0=-200' in values and 'eqBand19=200' in values and 'eqEnabled=1' in values
            press(2200);assert not u.IsWindowEnabled(slider)
            press(2200);assert signed(send(slider,0x400))==-20
            results.append({'legacy_fraction_migrated':True,'range_integer_1_page5':True,'numeric_plus_minus_limits':True,'disabled_controls':True})
        else:
            assert u.IsWindowEnabled(u.GetDlgItem(eq,2300))
            assert signed(send(u.GetDlgItem(eq,2300),0x400))==20 and signed(send(u.GetDlgItem(eq,2319),0x400))==-20
            press(2201);assert all(send(u.GetDlgItem(eq,2300+i),0x400)==0 for i in range(20))
            results.append({'saved_values_and_enabled_restored':True,'reset20':True})
        before=state(2);time.sleep(.12);assert state(2)>before and not state(13) and u.IsWindowEnabled(main)
        send(eq,0x10);send(main,0x113,2);assert p.wait(timeout=8)==0
    finally:
        if p.poll() is None:p.terminate();p.wait(timeout=5)
result={'pass':True,'audio':'dummy','own_windows_and_settings_only':True,'cases':results}
(out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
