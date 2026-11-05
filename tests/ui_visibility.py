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




results=[]
for name,wanted in [('channels.nsfe',4),('dmc-late.nsfe',5),('chips/es-late.vgm',8),('chips/es-many.vgm',32),('chips/ay-own.vgm',3),('channels.minigsf',6),('channels.mini2sf',16)]:
 folder=out/Path(name).stem;folder.mkdir(exist_ok=True);settings=folder/'settings.ini';settings.write_text('repeat=0\nautoplay=0\n')
 env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='visibility1511-'+Path(name).stem,NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_SNAPSHOT_MS='20000')
 p=subprocess.Popen([str(exe),'--snapshot',str(fixture/name),str(folder/'view.bmp'),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
 try:
  main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No window')[0];state=lambda a,b=0:send(main,0x8000+74,a,b)
  wait_for(lambda:state(0) and state(1),'No playback');first=[state(21,i) for i in range(state(20))];assert len(first)==wanted,(name,first)
  if name=='dmc-late.nsfe':assert 4 in first and state(2)<800,'DMC only appeared at first note'
  if name=='chips/es-late.vgm':assert 38 in first and state(2)<800,'ES only appeared at first note'
  time.sleep(1.1 if 'late' in name else .08);assert [state(21,i) for i in range(state(20))]==first,'Channel list changed during playback'
  send(main,0x100,0x20);assert [state(21,i) for i in range(state(20))]==first,'Pause changed list'
  send(main,0x100,0x78);assert [state(21,i) for i in range(state(20))]==first,'Recording changed list'
  assert not state(13);send(main,0x113,2);assert p.wait(timeout=10)==0
  results.append({'file':name,'visible':first,'fixed_before_first_note':True})
 finally:
  if p.poll() is None:p.terminate();p.wait(timeout=5)
(out/'result.json').write_text(json.dumps({'pass':True,'cases':results,'audio':'dummy'},indent=2));print(json.dumps(results))
