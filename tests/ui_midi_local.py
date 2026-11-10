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
paired=out/'paired.mid';shutil.copy2(fixture/'live.mid',paired);shutil.copy2(fixture/'own2.dls',paired.with_suffix('.dls'));shutil.copy2(fixture/'own.sf2',paired.with_suffix('.sf2'))
config=out/'VirtualMIDISynth.conf';a=out/'vms-a.sf2';b=out/'vms-b.sf2';shutil.copy2(fixture/'own.sf2',a);shutil.copy2(fixture/'alternate.sf2',b)
def choose(font):
 config.write_text('[SoundFonts]\nsf1='+str(font)+'\nsf1.enabled=1\n',encoding='utf-8')
choose(a)
settings=out/'settings.ini';settings.write_text('master=73\nrepeat=0\nfollowVirtualMidi=1\ncontrolsRevision=1415\n')
(out/'midi-soundfont.txt').write_text(str(a),encoding='utf-8')
env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='local-midi-native',NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_VMS_CONFIG=str(config),NGMV_TEST_SNAPSHOT_MS='20000')
p=subprocess.Popen([str(exe),'--snapshot',str(paired),str(out/'local.bmp'),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
try:
 main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No local MIDI window',seconds=10)[0];state=lambda x,y=0:send(main,0x8000+74,x,y)
 wait_for(lambda:state(1)and state(2)>400,'Local MIDI did not start',seconds=10);assert state(41)and state(42)==1 and state(3)==73
 choose(b);time.sleep(1.8);assert state(41)and state(25)==0 and not state(26),'VMS overrode local DLS'
 paired.with_suffix('.dls').write_bytes(b'bad owned DLS');drop(main,paired)
 wait_for(lambda:state(1)and state(42)==2 and state(2)>300,'Sibling SF2 fallback not playing');time.sleep(1.2)
 assert state(25)==0 and state(41)and not state(13),'VMS overrode local SF2'
 drop(main,fixture/'live.mid');wait_for(lambda:state(1)and not state(41),'Unpaired MIDI did not resume default bank')
 wait_for(lambda:state(25)>=1,'VMS live follow did not recover',seconds=7)
 choose(a);wait_for(lambda:state(25)>=2,'Second VMS change did not apply',seconds=7)
 assert state(1)and state(3)==73 and not state(13)
 result={'pass':True,'local_DLS_and_SF2_protected':True,'unpaired_VMS_live_follow_restored':True,'master_preserved':True,'audio':'dummy','only_owned_files_changed':True}
 send(main,0x113,2);assert p.wait(timeout=10)==0
 (out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
finally:
 if p.poll()is None:p.terminate();p.wait(timeout=5)
