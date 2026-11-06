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
real=Path(sys.argv[4]).resolve() if len(sys.argv)>4 else None
config=Path(sys.argv[5]).resolve() if len(sys.argv)>5 else None
cases=[('configured',None)] if real else [('windows',None),('sf2',fixture/'한글 음색.sf2'),('dls',fixture/'own.dls')]
for label,bank in cases:
    folder=out/label;folder.mkdir(exist_ok=True);settings=folder/'settings.ini';settings.write_text('autoplay=0\nrepeat=0\npitch=100\nspeed=100\nreverb=0\ncontrolsRevision=1415\nmidiDeviceOutput=1\nmidiOutputRevision=1513\n')
    if label=='sf2':
        with settings.open('a') as ini:
            ini.write('channel.'+('MIDI\nMIDI 01').encode().hex()+'.volume=40\n')
            ini.write('channel.'+('MIDI\nChannel 1').encode().hex()+'.volume=100\n')
    if bank:(folder/'midi-soundfont.txt').write_text(str(bank),encoding='utf-8')
    env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='midi159-'+label,NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_SNAPSHOT_MS='20000',NGMV_TEST_NO_SNAPSHOT_PLAY='1')
    if config:env['NGMV_TEST_VMS_CONFIG']=str(config)
    p=subprocess.Popen([str(exe),'--snapshot',str(real or fixture/'simultaneous.mid'),str(folder/'midi.bmp'),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No MIDI window')[0];state=lambda a:send(main,0x8000+74,a)
        wait_for(lambda:state(1) and state(2)>(1500 if real else 300),'MIDI open did not play',seconds=15);assert not state(13);assert state(23)==0,'Old default device route was not migrated to internal synthesis'
        if label=='sf2':assert send(main,0x8000+74,19,0)==100,'New channel setting did not override legacy label'
        channel_count=state(17);send(main,0x113,2);assert p.wait(timeout=10)==0
        image=Image.open(folder/'midi.bmp').convert('RGB');image.save(folder/'midi.png')
        lanes=[]
        if real:
            count=channel_count;columns=2 if count>8 else 1;rows=(count+columns-1)//columns;height=(1000-276)//rows;colors=[(247,205,87),(126,222,190),(133,180,255),(222,146,232),(250,155,106),(181,211,119),(126,213,228),(247,163,181)]
            for i in range(count):
                column=i//rows;y=276+(i%rows)*height;x=32+column*(1888//columns);width=(1888//columns)-32 if columns>1 else 1856;left=x+176 if columns>1 else 280
                crop=image.crop((left+8,y+8,x+width-36,y+height-16));ys={k//crop.width for k,c in enumerate(crop.getdata()) if c==colors[i%8]}
                if len(ys)>8:lanes.append(i)
            assert lanes,'No actual nonflat MIDI lane in its own waveform cell'
        else:
            for color in [(247,205,87),(126,222,190)]:
                points=[i//image.width for i,c in enumerate(image.getdata()) if c==color and i%image.width>500 and 276<i//image.width<990]
                assert len(points)>100 and max(points)-min(points)>8,('Missing MIDI waveform',label,color)
                lanes.append(len(points))
        results.append({'bank':label,'auto_start':True,'separate_channel_wave_pixels':lanes})
    finally:
        if p.poll() is None:p.terminate();p.wait(timeout=5)
(out/'result.json').write_text(json.dumps({'pass':True,'cases':results,'audio':'dummy','snapshot_forced_play':False},indent=2));print(json.dumps(results))
