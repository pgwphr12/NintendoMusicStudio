"""Own hidden ordinary-audio playback/waveforms and EOF clock test; dummy audio."""
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
results=[]
for name,count in [('es-many.vgm',33),('es-last.vgm',40),('ay-own.vgm',3),('es-direct-bus.vgm',33)]:
    folder=out/Path(name).stem;folder.mkdir(exist_ok=True);settings=folder/'settings.ini';settings.write_text('repeat=0\nautoplay=0\nreverb=0\n')
    if name=='es-direct-bus.vgm':
        old='channel.'+b'\nES5506 MIX'.hex();one='channel.'+b'\nES5506 1'.hex()
        with settings.open('a') as f:f.write(old+'.volume=125\n'+old+'.mute=1\n'+one+'.volume=170\n'+one+'.mute=0\n')
    env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='chips1510-'+name,NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_SNAPSHOT_MS='20000')
    p=subprocess.Popen([str(exe),'--snapshot',str(fixture/name),str(folder/'channels.bmp'),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No chip player')[0];state=lambda a,b=0:send(main,0x8000+74,a,b)
        wait_for(lambda:state(1) and state(2)>300,'No chip playback');assert state(17)==count and not state(13)
        if name=='es-direct-bus.vgm':
            assert state(19,0)==170 and state(18,0)==0,'Individual ES setting lost to migration'
            assert state(19,31)==125 and state(18,31)==1 and state(18,32)==1,'Legacy MIX setting did not migrate'
        if count==40:
            visible=[state(21,i) for i in range(state(20))];slot=visible.index(38)
            count_visible=len(visible);columns=4 if count_visible>48 else 3 if count_visible>32 else 2 if count_visible>8 else 1
            rows=(count_visible+columns-1)//columns;step=1888//columns;lane=(1000-276)//rows
            left=32+(slot//rows)*step;top=276+(slot%rows)*lane
            x=left+24;y=top+7
            for _ in range(2):
                send(main,0x201,1,x|(y<<16));send(main,0x202,0,x|(y<<16))
                assert state(18,38)==(1 if _==0 else 0),'High-index mute click failed'
            y=top+lane-(24 if lane<70 else 40)+5
            send(main,0x201,1,x|(y<<16));send(main,0x202,0,x|(y<<16));assert state(19,38)==95,'High-index volume click failed'
        if count==33:send(main,0x100,0x78) # recording view,3 columns
        send(main,0x113,2);assert p.wait(timeout=10)==0
        im=Image.open(folder/'channels.bmp').convert('RGB');im.save(folder/'channels.png')
        colored=sum(1 for c in im.crop((200,280,1888,980)).getdata() if c in [(247,205,87),(126,222,190),(137,180,249),(222,146,232),(250,155,106),(181,211,119),(126,213,228),(247,163,181)])
        assert colored>200 or name=='es-direct-bus.vgm','Missing chip waveform'
        results.append({'file':name,'channels':count,'wave_pixels':colored,'high_index_controls':count==40,'recording':count==33})
    finally:
        if p.poll() is None:p.terminate();p.wait(timeout=5)
(out/'result.json').write_text(json.dumps({'pass':True,'cases':results,'audio':'dummy'},indent=2));print(json.dumps(results))

