from pathlib import Path
import ctypes as C
from ctypes import wintypes as W
import os,subprocess,sys,time,wave,struct,math,json
exe=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve();out.mkdir(parents=True,exist_ok=True)
audio=out/'fixture.vgm';data=bytearray(0x100);data[:4]=b'Vgm '
struct.pack_into('<I',data,8,0x150);struct.pack_into('<I',data,0x0c,3579545);struct.pack_into('<I',data,0x34,0xcc)
data.extend([0x50,0x80,0x50,0x01,0x50,0x90,0x62]);loop=len(data)
data.extend([0x50,0x9f,0x62,0x50,0x90,0x62,0x66]);struct.pack_into('<I',data,4,len(data)-4)
struct.pack_into('<I',data,0x18,2205);struct.pack_into('<I',data,0x1c,loop-0x1c);struct.pack_into('<I',data,0x20,1470);audio.write_bytes(data)
second=bytearray(data[:-1]);second.extend([0x61,0x44,0xac,0x66]);struct.pack_into('<I',second,4,len(second)-4);struct.pack_into('<I',second,0x18,46305);struct.pack_into('<I',second,0x20,45570);(out/'fixture2.vgm').write_bytes(second)
settings=out/'settings.ini';settings.write_text('controlsRevision=1415\nrecording=0\nautoplay=1\nrepeat=1\nreverb=0\nuiLanguage=0\n')
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

u.GetWindowTextW.argtypes=[W.HWND,W.LPWSTR,C.c_int];u.GetWindowRect.argtypes=[W.HWND,C.POINTER(W.RECT)]
def text(h):
 b=C.create_unicode_buffer(32768);send(h,0xd,len(b),C.addressof(b));return b.value
def rect(h):
 r=W.RECT();u.GetWindowRect(h,C.byref(r));return r
def capture(h,name):
 g=C.WinDLL('gdi32');u.GetDC.argtypes=[W.HWND];u.GetDC.restype=W.HDC;u.ReleaseDC.argtypes=[W.HWND,W.HDC];u.PrintWindow.argtypes=[W.HWND,W.HDC,W.UINT]
 g.CreateCompatibleDC.argtypes=[W.HDC];g.CreateCompatibleDC.restype=W.HDC;g.CreateCompatibleBitmap.argtypes=[W.HDC,C.c_int,C.c_int];g.CreateCompatibleBitmap.restype=W.HBITMAP;g.SelectObject.argtypes=[W.HDC,W.HGDIOBJ];g.SelectObject.restype=W.HGDIOBJ;g.GetDIBits.argtypes=[W.HDC,W.HBITMAP,W.UINT,W.UINT,C.c_void_p,C.c_void_p,W.UINT];g.DeleteObject.argtypes=[W.HGDIOBJ];g.DeleteDC.argtypes=[W.HDC]
 r=W.RECT();u.GetClientRect(h,C.byref(r));w,z=r.right,r.bottom;dc=u.GetDC(h);mem=g.CreateCompatibleDC(dc);bmp=g.CreateCompatibleBitmap(dc,w,z);old=g.SelectObject(mem,bmp)
 try:
  u.PrintWindow(h,mem,1);g.SelectObject(mem,old)
  info=struct.pack('<IiiHHIIiiII',40,w,-z,1,32,0,w*z*4,0,0,0,0);buf=C.create_string_buffer(w*z*4);assert g.GetDIBits(mem,bmp,0,z,buf,C.create_string_buffer(info),0)==z
  (out/(name+'.bmp')).write_bytes(struct.pack('<2sIHHI',b'BM',54+w*z*4,0,0,54)+info+buf.raw)
 finally:g.SelectObject(mem,old);g.DeleteObject(bmp);g.DeleteDC(mem);u.ReleaseDC(h,dc)

class MonitorInfo(C.Structure):
 _fields_=[('size',W.DWORD),('monitor',W.RECT),('work',W.RECT),('flags',W.DWORD)]
u.MonitorFromWindow.argtypes=[W.HWND,W.DWORD];u.MonitorFromWindow.restype=W.HANDLE
u.GetMonitorInfoW.argtypes=[W.HANDLE,C.POINTER(MonitorInfo)]
u.SetWindowPos.argtypes=[W.HWND,W.HWND,C.c_int,C.c_int,C.c_int,C.c_int,W.UINT]
def check_work_area(h):
 m=MonitorInfo();m.size=C.sizeof(m);assert u.GetMonitorInfoW(u.MonitorFromWindow(h,2),C.byref(m));r=rect(h);w=m.work
 assert r.left>=w.left and r.top>=w.top and r.right<=w.right and r.bottom<=w.bottom,('outside work area',r.left,r.top,r.right,r.bottom,w.left,w.top,w.right,w.bottom)
 return w
sessions=[]
for session in range(2):
 env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='corrlanguage'+str(session),NGMV_TEST_SNAPSHOT_MS='60000',NGMV_SETTINGS_TEST_FILE=str(settings))
 p=subprocess.Popen([str(exe),'--snapshot',str(audio),str(out/('main'+str(session)+'.bmp')),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
 try:
  main=wait(lambda:find(p.pid,'NintendoMusicStudioWindow'));state=lambda n:send(main,0x8000+74,n);wait(lambda:state(0) and state(1));send(main,0x100,0x7b);remote=wait(lambda:find(p.pid,'NintendoMusicStudioRemote'))
  press=lambda h,id:send(u.GetDlgItem(h,id),0xf5)
  work=check_work_area(remote)
  u.SetWindowPos(remote,None,work.right-20,work.bottom-20,0,0,0x15)
  send(main,0x100,0x7b);send(main,0x100,0x7b);check_work_area(remote)
  combo=u.GetDlgItem(remote,2125)
  def language(n):
   send(combo,0x14e,n);send(remote,0x111,(1<<16)|2125,combo);assert state(50)==n,(n,state(50),send(combo,0x147))
  assert rect(u.GetDlgItem(remote,2123)).top>=rect(u.GetDlgItem(remote,2122)).bottom
  assert rect(u.GetDlgItem(remote,2125)).top>=rect(u.GetDlgItem(remote,2115)).bottom
  assert rect(u.GetDlgItem(remote,2125)).bottom<=rect(u.GetDlgItem(remote,2116)).top
  if session==0:
   assert state(49)==0 and state(50)==0
   assert text(u.GetDlgItem(remote,2123))=='코르스코프 파형'
   press(remote,2122);assert state(49)==1;press(remote,2123);assert state(49)==2 and send(u.GetDlgItem(remote,2122),0xf0)==0
   wait(lambda:state(51)>=3)
   language(1);capture(remote,'remote-english');assert text(u.GetDlgItem(remote,2123))=='Corrscope waveform'
   assert text(u.GetDlgItem(remote,2103))=='Stop' and text(u.GetDlgItem(remote,2124))=='Language'
   r=W.RECT();u.GetClientRect(main,C.byref(r));scale=min(r.right/1920,r.bottom/1080);pos=int((r.right-1920*scale)/2+980*scale)|(int((r.bottom-1080*scale)/2+45*scale)<<16);send(main,0x201,1,pos);send(main,0x202,0,pos);export=wait(lambda:find(p.pid,'NintendoMusicStudioExport'))
   assert text(u.GetDlgItem(export,2707))=='Select folder and export'
   length=C.create_unicode_buffer('42.5');send(u.GetDlgItem(export,2703),0xc,0,C.addressof(length));assert text(u.GetDlgItem(export,2703))=='42.5',('immediate',text(u.GetDlgItem(export,2703)));language(0);assert text(u.GetDlgItem(export,2703))=='42.5' and text(u.GetDlgItem(export,2707))=='폴더 선택 후 내보내기',(text(u.GetDlgItem(export,2703)),text(u.GetDlgItem(export,2707)))
   language(1);assert text(u.GetDlgItem(export,2703))=='42.5'
   send(main,0x100,0x70);about=wait(lambda:find(p.pid,'NintendoMusicStudioInfo'));assert 'Music playback' in text(u.GetDlgItem(about,1));send(about,0x10)
   press(remote,2107);eq=wait(lambda:find(p.pid,'NintendoMusicStudioEqualizer'));assert text(u.GetDlgItem(eq,2200))=='Enable EQ';language(0);capture(remote,'remote-korean');assert text(u.GetDlgItem(eq,2200))=='EQ 사용';language(1);send(eq,0x10)
   press(remote,2103);assert not state(1);wait(lambda:not state(43));assert state(49)==2
   press(remote,2102);wait(lambda:state(1));assert 'corrscopeWaveform=1' in settings.read_text() and 'uiLanguage=1' in settings.read_text()
  else:
   assert state(49)==2 and state(50)==1,'Mode/language did not persist'
   wait(lambda:state(51)>=1)
   press(remote,2122);assert state(49)==1 and send(u.GetDlgItem(remote,2123),0xf0)==0
   press(remote,2122);assert state(49)==0
  sessions.append({'session':session,'pass':True,'corrscope_frames':state(51)})
  send(main,0x113,2);assert p.wait(timeout=10)==0
 finally:
  if p.poll()is None:p.terminate();p.wait(timeout=5)
(out/'result.json').write_text(json.dumps({'pass':True,'sessions':sessions,'audio':'dummy','live_language_and_inputs_preserved':True,'work_area_initial_and_reopen':True,'placement':'corrscope below legacy; language in middle'},indent=2));print('PASS corrscope processing, three distinct modes, KO/EN live switch, layout, export input preservation restart persistence and taskbar-safe placement')
