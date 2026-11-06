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
config=out/'VirtualMIDISynth.conf';a=out/'a.sf2';b=out/'b.sf2';shutil.copy2(fixture/'own.sf2',a);shutil.copy2(fixture/'alternate.sf2',b)
def choose(font):
 temp=config.with_suffix('.tmp');temp.write_text('[SoundFonts]\nsf1='+str(font)+'\nsf1.enabled=1\n',encoding='utf-8');os.replace(temp,config)
choose(a)
key='channel.'+('MIDI\nMIDI 01').encode().hex()
settings=out/'settings.ini';settings.write_text('master=73\nrepeat=0\nautoplay=0\nfollowVirtualMidi=1\n'+key+'.mute=1\n'+key+'.volume=55\n')
(out/'midi-soundfont.txt').write_text(str(a),encoding='utf-8')
env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='live-bank-owned',NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_VMS_CONFIG=str(config),NGMV_TEST_SNAPSHOT_MS='25000',NGMV_TEST_NO_SNAPSHOT_PLAY='1')
p=subprocess.Popen([str(exe),'--snapshot',str(fixture/'live.mid'),str(out/'live.bmp'),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
try:
 main=wait_for(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No live MIDI window')[0];state=lambda a,b=0:send(main,0x8000+74,a,b)
 wait_for(lambda:state(1) and state(2)>800,'No playback',seconds=10);assert state(3)==73 and state(18,0)==1 and state(19,0)==55
 before=state(2);choose(b);wait_for(lambda:state(25)>=1,'Bank change not applied',seconds=8);after=state(2)
 assert state(1) and after>=before-50 and after<before+3500,('Playback position reset',before,after)
 assert state(3)==73 and state(18,0)==1 and state(19,0)==55,'Controls changed during bank swap'
 broken=out/'broken.sf2';broken.write_bytes(b'RIFF'+bytes(20));choose(broken);wait_for(lambda:state(26),'Broken bank not reported',seconds=8)
 assert state(25)==1 and state(1) and state(2)>=after and not state(13),'Broken bank lost old decoder or playing state'
 choose(b);wait_for(lambda:not state(26),'Old valid bank did not recover',seconds=5)
 send(main,0x100,0x20);assert not state(1);paused=state(2);choose(a);wait_for(lambda:state(25)>=2,'Paused bank change ignored',seconds=8)
 assert not state(1) and abs(state(2)-paused)<50,'Paused position/state changed'
 choose(out/'missing.sf2');wait_for(lambda:state(26),'Missing bank not reported',seconds=5);assert state(25)==2 and not state(13),'Failed bank replaced decoder'
 choose(b);wait_for(lambda:state(25)>=3 and not state(26),'Recovery bank failed',seconds=8);assert not state(1) and abs(state(2)-paused)<50
 shutil.copyfile(fixture/'own.sf2',b);wait_for(lambda:state(25)>=4,'Same-path bank edit not applied',seconds=8)
 assert not state(1) and abs(state(2)-paused)<50 and state(3)==73 and state(18,0)==1 and state(19,0)==55
 assert (out/'midi-soundfont.txt').read_text()==str(b),'New bank selection was not saved'
 send(main,0x100,0x20);wait_for(lambda:state(1) and state(2)>paused+100,'Resume failed')
 result={'pass':True,'playing_position_before_ms':before,'playing_position_after_ms':after,'paused_position_ms':paused,'reloads':state(25),'pause_mute_gain_preserved':True,'missing_bank_kept_old_decoder':True,'corrupt_bank_kept_playing':True,'recovery_and_same_path_change':True,'audio':'dummy','only_owned_config_modified':True}
 send(main,0x113,2);assert p.wait(timeout=10)==0
 (out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
finally:
 if p.poll() is None:p.terminate();p.wait(timeout=5)
