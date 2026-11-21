from pathlib import Path
import ctypes as C
from ctypes import wintypes as W
import os,subprocess,sys,time,wave,struct,math
exe=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve();out.mkdir(parents=True,exist_ok=True)
audio=out/'fixture.vgm';data=bytearray(0x100);data[:4]=b'Vgm '
struct.pack_into('<I',data,8,0x150);struct.pack_into('<I',data,0x0c,3579545);struct.pack_into('<I',data,0x34,0xcc)
data.extend([0x50,0x80,0x50,0x01,0x50,0x90,0x62]);loop=len(data)
data.extend([0x50,0x9f,0x62,0x50,0x90,0x62,0x66]);struct.pack_into('<I',data,4,len(data)-4)
struct.pack_into('<I',data,0x18,2205);struct.pack_into('<I',data,0x1c,loop-0x1c);struct.pack_into('<I',data,0x20,1470);audio.write_bytes(data)
second=bytearray(data[:-1]);second.extend([0x61,0x44,0xac,0x66]);struct.pack_into('<I',second,4,len(second)-4);struct.pack_into('<I',second,0x18,46305);struct.pack_into('<I',second,0x20,45570);(out/'fixture2.vgm').write_bytes(second)
settings=out/'settings.ini';settings.write_text('controlsRevision=1415\nrecording=0\nautoplay=0\n')
u=C.WinDLL('user32',use_last_error=True);CB=C.WINFUNCTYPE(W.BOOL,W.HWND,W.LPARAM)
u.GetWindowThreadProcessId.argtypes=[W.HWND,C.POINTER(W.DWORD)];u.GetClassNameW.argtypes=[W.HWND,W.LPWSTR,C.c_int]
u.GetDlgItem.argtypes=[W.HWND,C.c_int];u.GetDlgItem.restype=W.HWND;u.GetClientRect.argtypes=[W.HWND,C.POINTER(W.RECT)]
u.SendMessageTimeoutW.argtypes=[W.HWND,W.UINT,W.WPARAM,W.LPARAM,W.UINT,W.UINT,C.POINTER(C.c_size_t)]
def send(h,m,a=0,b=0):
 r=C.c_size_t();assert u.SendMessageTimeoutW(h,m,a,b,2,3000,C.byref(r));return r.value
def find(pid,name):
 matches=[]
 @CB
 def cb(h,_):
  p=W.DWORD();u.GetWindowThreadProcessId(h,C.byref(p));s=C.create_unicode_buffer(128);u.GetClassNameW(h,s,128)
  if p.value==pid and s.value==name:matches.append(h)
  return True
 u.EnumWindows(cb,0);return matches[0] if matches else None
def wait(fn):
 end=time.monotonic()+10
 while time.monotonic()<end:
  x=fn()
  if x:return x
  time.sleep(.03)
 raise AssertionError('Owned window timeout')
target=out/'exports';target.mkdir(exist_ok=True)
env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='exportcheck',NGMV_TEST_SNAPSHOT_MS='20000',NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_EXPORT_DIR=str(target),NGMV_TEST_EXPORT_REPEATS='1')
p=subprocess.Popen([str(exe),'--snapshot',str(audio),str(out/'snapshot.bmp'),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
try:
 main=wait(lambda:find(p.pid,'NintendoMusicStudioWindow'));state=lambda n:send(main,0x8000+74,n)
 wait(lambda:state(0) and state(1))
 r=W.RECT();u.GetClientRect(main,C.byref(r));scale=min(r.right/1920,r.bottom/1080);x=int((r.right-1920*scale)/2+980*scale);y=int((r.bottom-1080*scale)/2+45*scale);pos=x|(y<<16)
 send(main,0x201,1,pos);send(main,0x202,0,pos)
 u.IsWindowVisible.argtypes=[W.HWND];dialog=wait(lambda:next((h for h in [find(p.pid,'NintendoMusicStudioExport')] if h and u.IsWindowVisible(h)),None));combo=u.GetDlgItem(dialog,2701)
 assert combo and send(combo,0x146)==2 and send(combo,0x147)==0,'Export choices missing'
 edit=u.GetDlgItem(dialog,2705);u.IsWindowEnabled.argtypes=[W.HWND];assert u.IsWindowEnabled(edit),'Repeat edit disabled'
 u.GetDlgItemTextW.argtypes=[W.HWND,C.c_int,W.LPWSTR,C.c_int]
 assert not list(target.iterdir()),'Export started before pressing start'
 send(u.GetDlgItem(dialog,2707),0xf5)
 result=wait(lambda:next((p for p in target.glob('fixture*') if p.is_dir()),None));files=list(result.glob('*.wav'));assert files,'No WAV files published';assert (result/'master.wav').is_file(),'Master mix missing'
 for wav in files:
  with wave.open(str(wav),'rb') as f:assert f.getframerate()==48000 and f.getnchannels()==2 and f.getnframes()==4000,'Wrong VGM recorded-loop WAV length'
 assert not list(target.glob('.nms-export-*')),'Temporary export folder was retained'
 assert state(48)==1,'Initial export length incorrect';send(main,0x100,0x27);wait(lambda:state(48)==2)
 send(main,0x100,0x7b);remote=wait(lambda:find(p.pid,'NintendoMusicStudioRemote'));checkbox=u.GetDlgItem(remote,2122)
 assert checkbox and send(checkbox,0xf0)==0;send(checkbox,0xf5);assert send(checkbox,0xf0)==1
 assert 'legacyWaveformMotion=1' in settings.read_text(),'Legacy selection not persisted'
 send(main,0x113,2);assert p.wait(timeout=10)==0
 print('PASS export UI, recorded-loop WAV, track duration refresh and saved legacy waveform option; dummy audio')
finally:
 if p.poll()is None:p.terminate();p.wait(timeout=5)
