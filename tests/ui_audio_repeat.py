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



settings=out/'settings.ini';settings.write_text('repeat=0\nautoplay=0\npitch=100\nspeed=100\nreverb=0\ncontrolsRevision=1415\n')
target=out/'repeat.bmp';env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='repeat155',NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_SNAPSHOT_MS='40000')
p=subprocess.Popen([str(exe),'--snapshot',str(fixture),str(target),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
try:
    main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No main')[0]
    state=lambda a:send(main,0x8000+74,a)
    wait_for(lambda:state(0) and state(1),'No playback')
    # Click the visible Repeat control rather than pre-enabling it in an INI.
    rect=W.RECT();u.GetClientRect(main,ctypes.byref(rect));scale=min(rect.right/1920,rect.bottom/1080)
    def click_repeat():
        x=int((rect.right-1920*scale)/2+994*scale);y=int((rect.bottom-1080*scale)/2+165*scale)
        send(main,0x201,1,x|(y<<16));send(main,0x202,0,x|(y<<16))
    def click_play():
        x=int((rect.right-1920*scale)/2+587*scale);y=int((rect.bottom-1080*scale)/2+165*scale)
        send(main,0x201,1,x|(y<<16));send(main,0x202,0,x|(y<<16))
    click_repeat()
    wanted=int(os.environ.get("NGMV_REPEAT_TEST_WRAPS","3"));wraps=0;previous=state(2);maximum=previous;deadline=time.monotonic()+25
    while time.monotonic()<deadline and wraps<wanted:
        current=state(2);maximum=max(maximum,current)
        assert state(13)==0,'Player error during repeat'
        assert state(1),'Playback stopped during repeat'
        if current+80<previous:wraps+=1
        previous=current;time.sleep(.012)
    result={'file':fixture.name,'wraps':wraps,'maximum_ms':maximum,'playing':bool(state(1)),'error':state(13),'pass':wraps>=wanted}
    print(json.dumps(result));(out/'result.json').write_text(json.dumps(result,indent=2))
    assert wraps>=wanted,'Audible-time repeat boundary missing'
    click_repeat() # Turning repeat off must preserve ordinary post-EOF clock behavior.
    if fixture.stem=='overstated':
        start=state(2);time.sleep(2.3);assert state(2)>start+1800,'Repeat-off still wraps'
        result['repeat_off_clock_continues']=True
        (out/'result.json').write_text(json.dumps(result,indent=2))
    if os.environ.get('NGMV_TEST_PLAY_AFTER_END')=='1':
        previous=state(2);stable=0;deadline=time.monotonic()+10
        while time.monotonic()<deadline and stable<8:
            current=state(2)
            assert state(1),'Transport stopped at EOF with Repeat off'
            stable=stable+1 if abs(current-previous)<=20 else 0
            previous=current;time.sleep(.12)
        assert stable>=8,'Source clock did not settle at EOF'
        result['repeat_off_silent_transport_stays_playing']=True
        assert state(13)==0,'Decoder error after Repeat was turned off'
        click_play();wait_for(lambda:not state(1),'Pause control did not pause the EOF transport')
        click_play()
        wait_for(lambda:state(1),'Play control did not restart the ended track')
        assert state(2)<500,'Play after end did not restart at the beginning'
        result['repeat_off_play_after_end_restart']=True
        (out/'result.json').write_text(json.dumps(result,indent=2))
    send(main,0x113,2);assert p.wait(timeout=10)==0
    assert 'repeat=0' in settings.read_text(), 'Repeat button did not toggle off' 
finally:
    if p.poll() is None:p.terminate();p.wait(timeout=5)
