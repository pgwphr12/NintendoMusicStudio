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

exe, fixtures, out = map(lambda p: Path(p).resolve(), sys.argv[1:4])
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



settings=out/'common.ini'
settings.write_text('autoplay=1\nreverb=0\ncontrolsRevision=1415\n')
results=[]
for index, name in enumerate(['channels.nsfe','channels.gbs','audio/tone.wav','channels.spc']):
    target=out/('case'+str(index)+'.bmp')
    env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='common162t6-'+str(index),NGMV_TEST_SNAPSHOT_MS='20000',NGMV_SETTINGS_TEST_FILE=str(settings))
    p=subprocess.Popen([str(exe),'--snapshot',str(fixtures/name),str(target),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No owned window')[0]
        state=lambda a,b=0:send(main,0x8000+74,a,b)
        wait_for(lambda:state(0) and state(1) and state(2)>100,'No playback')
        def click(x,y):
            r=W.RECT();u.GetClientRect(main,ctypes.byref(r));scale=min(r.right/1920,r.bottom/1080)
            pos=int((r.right-1920*scale)/2+x*scale)|(int((r.bottom-1080*scale)/2+y*scale)<<16)
            send(main,0x201,1,pos);send(main,0x202,0,pos)
        def all_gain(v):assert all(state(19,c)==v for c in range(state(17))),[(c,state(19,c)) for c in range(state(17))]
        if index==0:
            click(315,1054);all_gain(50)
            assert 'allChannelVolume=50' in settings.read_text(),'Not saved at mouse release'
            click(90,284);assert state(18,0)==1
            y=276+(1000-276)//state(20)-40+8
            click(210,y);assert state(19,0)==55
            click(496,1054);all_gain(55);assert state(18,0)==1,'Common gain affected mute'
            click(417,1054);all_gain(150)
            click(210,y);assert state(19,0)==155
            # Preserve current mixed channel overrides on stop/play and seek.
            click(735,165);assert not state(1)
            assert state(19,0)==155 and all(state(19,c)==150 for c in range(1,state(17)))
            click(585,165);wait_for(lambda:state(1),'Resume after stop failed')
            click(500,1016)
            assert state(19,0)==155 and all(state(19,c)==150 for c in range(1,state(17)))
            send(main,0x100,0x27);all_gain(150);assert state(18,0)==1
            assert not state(13)
            results.append({'mixed_stop_play_seek_retained':True,'group_saved_immediately':True,'group_buttons_use_common_not_first_channel':True,'subsong_prioritizes_common':True,'mute_preserved':True})
        else:
            all_gain(150);assert not state(13)
            results.append({'file':name,'channels':state(17),'common_overrides_stale_individual_values':True})
        send(main,0x113,2);assert p.wait(timeout=8)==0
    finally:
        if p.poll() is None:p.terminate();p.wait(timeout=5)
    # Deliberately conflicting saved per-chip values must not override group gain.
    lines=settings.read_text().splitlines()
    settings.write_text('\n'.join(line.split('=')[0]+'=180' if line.startswith('channel.') and '.volume=' in line else line for line in lines)+'\n')
result={'pass':True,'audio':'dummy','only_owned_windows_and_isolated_settings':True,'cases':results}
(out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
