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





import shutil

# Owned local banks; the supplied fixture library remains read-only.
import struct
k=ctypes.WinDLL('kernel32',use_last_error=True)
k.GlobalAlloc.argtypes=[W.UINT,ctypes.c_size_t];k.GlobalAlloc.restype=W.HANDLE
k.GlobalLock.argtypes=[W.HANDLE];k.GlobalLock.restype=ctypes.c_void_p
k.GlobalUnlock.argtypes=[W.HANDLE]
def drop(h,path):
 data=struct.pack('<IiiII',20,0,0,0,1)+str(path).encode('utf-16le')+bytes(4)
 memory=k.GlobalAlloc(0x42,len(data));assert memory
 ptr=k.GlobalLock(memory);assert ptr;ctypes.memmove(ptr,data,len(data));k.GlobalUnlock(memory)
 assert u.PostMessageW(h,0x233,memory,0) # Receiver owns/frees successfully queued HDROP.
paired=out/'paired.mid';shutil.copy2(fixture/'live.mid',paired);shutil.copy2(fixture/'own2.dls',paired.with_suffix('.dls'))
settings=out/'settings.ini';settings.write_text('master=500\nallChannelVolume=500\nrepeat=1\nreverb=0\nmidiLocalBankSearch=0\nfollowVirtualMidi=0\ncontrolsRevision=1415\n')
(out/'midi-soundfont.txt').write_text(str(fixture/'own.sf2'),encoding='utf-8')
u.GetMenuState.argtypes=[W.HMENU,W.UINT,W.UINT];u.GetMenuState.restype=W.UINT
u.GetMenuItemRect.argtypes=[W.HWND,W.HMENU,W.UINT,ctypes.POINTER(W.RECT)]
u.GetMenuItemCount.argtypes=[W.HMENU]
u.GetMenuItemID.argtypes=[W.HMENU,ctypes.c_int]
results=[]
for session in range(2):
 env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='midi-options-owned',NGMV_TEST_NO_VMS='1',NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_SNAPSHOT_MS='25000')
 p=subprocess.Popen([str(exe),'--snapshot',str(paired),str(out/('session%d.bmp'%session)),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
 try:
  main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No MIDI window',10)[0];state=lambda x,y=0:send(main,0x8000+74,x,y)
  wait_for(lambda:state(1)and state(2)>250,'No manual MIDI playback',10)
  assert not state(27)and not state(54)and not state(41)and state(52)==500 and state(53)==500
  assert state(3)==500 and all(state(19,c)==500 for c in range(state(17)))
  u.PostMessageW(main,0x100,0x7b,0);remote=wait_for(lambda:windows(p.pid,'NintendoMusicStudioRemote'),'No remote')[0]
  wait_for(lambda:send(u.GetDlgItem(remote,2112),0x402)==500,'Master slider maximum not updated')
  assert send(u.GetDlgItem(remote,2110),0x402)==500 and send(u.GetDlgItem(remote,2110),0x400)==500
  u.PostMessageW(main,0x100,0x77,0)
  popup=wait_for(lambda:windows(p.pid,'#32768'),'Bank menu unavailable without VMS')[0];menu=send(popup,0x1e1)
  assert menu
  assert u.GetMenuState(menu,2,0)&3==0 and u.GetMenuState(menu,5,0)&3 and u.GetMenuState(menu,6,0)&3
  assert not u.GetMenuState(menu,7,0)&8
  ids=[u.GetMenuItemID(menu,i) for i in range(u.GetMenuItemCount(menu))];idx=ids.index(7)
  # Navigate only this owned popup modal loop, without moving the desktop cursor.
  rect=W.RECT();assert u.GetMenuItemRect(None,menu,idx,ctypes.byref(rect));origin=W.RECT();u.GetWindowRect(popup,ctypes.byref(origin))
  pt=((rect.left+rect.right)//2-origin.left)|(((rect.top+rect.bottom)//2-origin.top)<<16)
  u.PostMessageW(popup,0x200,0,pt);u.PostMessageW(popup,0x201,1,pt);u.PostMessageW(popup,0x202,0,pt)
  wait_for(lambda:state(54)and state(41),'Local bank option did not apply',8)
  assert 'midiLocalBankSearch=1' in settings.read_text()
  # Toggle back through the same menu; keep the manual bank for restart.
  u.PostMessageW(main,0x100,0x77,0);popup=wait_for(lambda:windows(p.pid,'#32768'),'Second bank menu')[0];menu=send(popup,0x1e1)
  rect=W.RECT();assert u.GetMenuItemRect(None,menu,idx,ctypes.byref(rect));origin=W.RECT();u.GetWindowRect(popup,ctypes.byref(origin))
  pt=((rect.left+rect.right)//2-origin.left)|(((rect.top+rect.bottom)//2-origin.top)<<16)
  u.PostMessageW(popup,0x200,0,pt);u.PostMessageW(popup,0x201,1,pt);u.PostMessageW(popup,0x202,0,pt)
  wait_for(lambda:not state(54)and not state(41),'Manual bank not restored',8)
  results.append({'session':session,'pass':True,'no_VMS_manual_bank_playback':True,'menu_VMS_only_disabled':True,'local_option_live_and_persistent':True,'master_and_channels_500':True})
  send(main,0x113,2);assert p.wait(timeout=10)==0
 finally:
  if p.poll()is None:p.terminate();p.wait(timeout=5)
print(json.dumps({'pass':True,'audio':'dummy','sessions':results}))
(out/'result.json').write_text(json.dumps({'pass':True,'audio':'dummy','sessions':results},indent=2))
