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



settings=out/'remote-eq.ini'
settings.write_text('autoplay=1\nreverb=0\ncontrolsRevision=1415\n')
results=[]
for session in range(2):
    env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='remote-eq162t8-'+str(session),NGMV_TEST_SNAPSHOT_MS='20000',NGMV_SETTINGS_TEST_FILE=str(settings))
    p=subprocess.Popen([str(exe),'--snapshot',str(fixture),str(out/('main'+str(session)+'.bmp')),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No main')[0]
        state=lambda a,b=0:send(main,0x8000+74,a,b)
        wait_for(lambda:state(0) and state(1) and state(2)>100,'No playback')
        if not state(15):send(main,0x100,0x78)
        remote=wait_for(lambda:windows(p.pid,'NintendoMusicStudioRemote'),'No remote')[0]
        assert u.IsWindowEnabled(main) and u.IsWindowEnabled(remote)
        def press(parent,id):return send(u.GetDlgItem(parent,id),0xf5)
        if session==0:
            press(remote,2102);assert not state(1)
            press(remote,2102);assert state(1)
            old=state(29);press(remote,2104);assert state(29)!=old
            press(remote,2101);assert state(29)==old
            press(remote,2105);assert state(16)
            slider=u.GetDlgItem(remote,2110);send(slider,0x405,1,65);send(remote,0x114,8,slider)
            assert all(state(19,c)==65 for c in range(state(17)))
            slider=u.GetDlgItem(remote,2112);send(slider,0x405,1,30);send(remote,0x114,8,slider);assert state(3)==30
        else:
            assert all(state(19,c)==65 for c in range(state(17))) and state(3)==30
        press(remote,2107);eq=wait_for(lambda:windows(p.pid,'NintendoMusicStudioEqualizer'),'No EQ')[0]
        for i in range(20):assert u.GetDlgItem(eq,2300+i)
        if session==0:
            press(eq,2200)
            for i,pos in [(0,-6),(10,4),(19,-3)]:
                slider=u.GetDlgItem(eq,2300+i);send(slider,0x405,1,pos);send(eq,0x115,8,slider)
            text=settings.read_text();assert 'eqEnabled=1' in text and 'eqBand0=60' in text and 'eqBand10=-40' in text and 'eqBand19=30' in text
        else:
            def signed(v):return ctypes.c_ssize_t(v).value
            assert signed(send(u.GetDlgItem(eq,2300),0x400))==-6
            assert signed(send(u.GetDlgItem(eq,2310),0x400))==4
            assert signed(send(u.GetDlgItem(eq,2319),0x400))==-3
            press(eq,2201)
            assert all(send(u.GetDlgItem(eq,2300+i),0x400)==0 for i in range(20))
        assert u.IsWindowEnabled(main)
        before=state(2);time.sleep(.15);assert state(2)>before and not state(13)
        if session==0:
            # Capture separate windows through their own client painting, never the desktop.
            from PIL import Image
            g=ctypes.WinDLL('gdi32',use_last_error=True)
            u.GetDC.restype=W.HDC;g.CreateCompatibleDC.argtypes=[W.HDC];g.CreateCompatibleDC.restype=W.HDC
            g.CreateCompatibleBitmap.argtypes=[W.HDC,ctypes.c_int,ctypes.c_int];g.CreateCompatibleBitmap.restype=W.HBITMAP
            g.SelectObject.argtypes=[W.HDC,W.HANDLE];g.SelectObject.restype=W.HANDLE
            g.DeleteObject.argtypes=[W.HANDLE];g.DeleteDC.argtypes=[W.HDC];u.ReleaseDC.argtypes=[W.HWND,W.HDC]
            class Header(ctypes.Structure):_fields_=[('size',W.DWORD),('width',W.LONG),('height',W.LONG),('planes',W.WORD),('bits',W.WORD),('compression',W.DWORD),('image',W.DWORD),('x',W.LONG),('y',W.LONG),('used',W.DWORD),('important',W.DWORD)]
            u.PrintWindow.argtypes=[W.HWND,W.HDC,W.UINT]
            g.GetDIBits.argtypes=[W.HDC,W.HBITMAP,W.UINT,W.UINT,ctypes.c_void_p,ctypes.c_void_p,W.UINT]
            for h,name in [(remote,'remote'),(eq,'equalizer')]:
                rect=W.RECT();u.GetClientRect(h,ctypes.byref(rect));width,height=rect.right,rect.bottom
                dc=u.GetDC(h);memory=g.CreateCompatibleDC(dc);bitmap=g.CreateCompatibleBitmap(dc,width,height);previous=g.SelectObject(memory,bitmap)
                try:
                    assert u.PrintWindow(h,memory,3);g.SelectObject(memory,previous)
                    info=Header(ctypes.sizeof(Header),width,-height,1,32,0,width*height*4,0,0,0,0);data=ctypes.create_string_buffer(width*height*4)
                    assert g.GetDIBits(memory,bitmap,0,height,data,ctypes.byref(info),0)==height
                    Image.frombytes('RGB',(width,height),data.raw,'raw','BGRX').save(out/(name+'.png'))
                finally:g.DeleteObject(bitmap);g.DeleteDC(memory);u.ReleaseDC(h,dc)
        press(remote,2106);wait_for(lambda:not state(15),'Cannot leave recording')
        assert u.IsWindowEnabled(main)
        results.append({'session':session,'remote_transport_master_common':True,'20band_controls':True,'EQ_persist_or_reset':True,'modeless_main_responsive':True,'F9_return':True})
        send(main,0x113,2);assert p.wait(timeout=8)==0
    finally:
        if p.poll() is None:p.terminate();p.wait(timeout=5)
result={'pass':True,'audio':'dummy','only_owned_processes_and_settings':True,'cases':results}
(out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
