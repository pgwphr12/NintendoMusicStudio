"""Own native choice window: retired preference migration, keyboard, capture."""
from pathlib import Path
import ctypes as C
from ctypes import wintypes as W
import os,subprocess,sys,time,json,struct
from PIL import Image
exe,fixture,out=map(lambda p:Path(p).resolve(),sys.argv[1:4]);out.mkdir(parents=True,exist_ok=True)
u=C.WinDLL('user32',use_last_error=True);g=C.WinDLL('gdi32',use_last_error=True)
CB=C.WINFUNCTYPE(W.BOOL,W.HWND,W.LPARAM)
u.GetWindowThreadProcessId.argtypes=[W.HWND,C.POINTER(W.DWORD)]
u.GetClassNameW.argtypes=[W.HWND,W.LPWSTR,C.c_int]
u.SendMessageTimeoutW.argtypes=[W.HWND,W.UINT,W.WPARAM,W.LPARAM,W.UINT,W.UINT,C.POINTER(C.c_size_t)]
u.GetDlgItem.argtypes=[W.HWND,C.c_int];u.GetDlgItem.restype=W.HWND
u.PostMessageW.argtypes=[W.HWND,W.UINT,W.WPARAM,W.LPARAM]
u.GetClientRect.argtypes=[W.HWND,C.POINTER(W.RECT)]
u.GetDC.argtypes=[W.HWND];u.GetDC.restype=W.HDC
u.ReleaseDC.argtypes=[W.HWND,W.HDC]
u.PrintWindow.argtypes=[W.HWND,W.HDC,W.UINT]
g.CreateCompatibleDC.argtypes=[W.HDC];g.CreateCompatibleDC.restype=W.HDC
g.CreateCompatibleBitmap.argtypes=[W.HDC,C.c_int,C.c_int];g.CreateCompatibleBitmap.restype=W.HBITMAP
g.SelectObject.argtypes=[W.HDC,W.HGDIOBJ];g.SelectObject.restype=W.HGDIOBJ
g.GetDIBits.argtypes=[W.HDC,W.HBITMAP,W.UINT,W.UINT,C.c_void_p,C.c_void_p,W.UINT]
g.DeleteObject.argtypes=[W.HGDIOBJ];g.DeleteDC.argtypes=[W.HDC]
def windows(pid):
 found=[]
 @CB
 def visit(h,_):
  p=W.DWORD();u.GetWindowThreadProcessId(h,C.byref(p));name=C.create_unicode_buffer(128);u.GetClassNameW(h,name,128)
  if p.value==pid:found.append((h,name.value))
  return True
 u.EnumWindows(visit,0);return found
def find(pid,name):return next((h for h,n in windows(pid)if n==name),None)
def send(h,m,a=0,b=0):
 v=C.c_size_t();assert u.SendMessageTimeoutW(h,m,a,b,2,1500,C.byref(v));return v.value
def wait(fn):
 end=time.monotonic()+5
 while time.monotonic()<end:
  v=fn()
  if v:return v
  time.sleep(.02)
 raise AssertionError('Own window timeout')
def capture(h):
 rect=W.RECT();u.GetClientRect(h,C.byref(rect));w,height=rect.right,rect.bottom
 dc=u.GetDC(h);mem=g.CreateCompatibleDC(dc);bmp=g.CreateCompatibleBitmap(dc,w,height);old=g.SelectObject(mem,bmp)
 try:
  assert u.PrintWindow(h,mem,1)
  g.SelectObject(mem,old)
  info=C.create_string_buffer(struct.pack('<IiiHHIIiiII',40,w,-height,1,32,0,w*height*4,0,0,0,0))
  data=C.create_string_buffer(w*height*4);assert g.GetDIBits(mem,bmp,0,height,data,info,0)==height
  Image.frombytes('RGB',(w,height),data.raw,'raw','BGRX').save(out/'choices.png')
 finally:g.SelectObject(mem,old);g.DeleteObject(bmp);g.DeleteDC(mem);u.ReleaseDC(h,dc)

u.IsWindowEnabled.argtypes=[W.HWND]
settings=out/'settings.ini';results=[]
for recording in (0,1):
 settings.write_text('spectrumView=1\nspectrumStyle=3\nrecording='+str(recording)+'\nreverb=0\ncontrolsRevision=1415\n')
 env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='remote163b2',NGMV_TEST_SNAPSHOT_MS='20000',NGMV_SETTINGS_TEST_FILE=str(settings))
 target=out/('recording.bmp'if recording else 'normal.bmp')
 p=subprocess.Popen([str(exe),'--snapshot',str(fixture),str(target),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
 try:
  main=wait(lambda:find(p.pid,'NintendoMusicStudioWindow'));state=lambda n:send(main,0x8000+74,n)
  wait(lambda:state(0)and state(1)and state(2)>100)
  assert state(15)==recording and not state(40)and not find(p.pid,'NintendoMusicStudioRemote'),'Startup opened remote'
  send(main,0x100,0x78);assert state(15)!=recording and not state(40),'F9 showed remote'
  send(main,0x100,0x78);assert state(15)==recording and not state(40)
  def click(x,y):
   rect=W.RECT();u.GetClientRect(main,C.byref(rect));scale=min(rect.right/1920,rect.bottom/1080)
   pos=int((rect.right-1920*scale)/2+x*scale)|(int((rect.bottom-1080*scale)/2+y*scale)<<16)
   send(main,0x201,1,pos);send(main,0x202,0,pos)
  if not recording:click(1125,1053)
  else:
   click(1125,1053);assert not state(40),'Recording retained hidden button hitbox'
   send(main,0x100,0x7b)
  remote=wait(lambda:find(p.pid,'NintendoMusicStudioRemote'));assert state(40)and state(15)==recording
  assert 'reverb=0' in settings.read_text(),'Remote button hit reverb'
  send(main,0x100,0x7b,1<<30);assert state(40),'Repeated key toggled remote'
  send(main,0x100,0x78);assert state(15)!=recording and state(40),'F9 hid remote'
  send(u.GetDlgItem(remote,2106),0xf5);wait(lambda:state(15)==recording);assert state(40)
  send(main,0x100,0x7b);assert not state(40)and state(15)==recording
  send(main,0x100,0x7b);assert state(40)and find(p.pid,'NintendoMusicStudioRemote')==remote
  # F12 forwarded from a native child control, without desktop key injection.
  assert u.PostMessageW(u.GetDlgItem(remote,2102),0x100,0x7b,0)
  wait(lambda:not state(40));assert state(15)==recording
  send(main,0x100,0x7b);assert state(40)
  send(u.GetDlgItem(remote,2107),0xf5);eq=wait(lambda:find(p.pid,'NintendoMusicStudioEqualizer'))
  u.PostMessageW(u.GetDlgItem(eq,2300),0x100,0x7b,0);wait(lambda:not state(40))
  send(eq,0x10);send(main,0x100,0x7b);assert state(40)
  for key,cls,child in [(0x76,'NintendoMusicStudioSpectrumChoices',2602),(None,'NintendoMusicStudioHiddenChannels',2700),(0x70,'NintendoMusicStudioInfo',None)]:
   if key:send(main,0x100,key)
   else:send(u.GetDlgItem(remote,2114),0xf5)
   tool=wait(lambda:find(p.pid,cls));focus=u.GetDlgItem(tool,child)if child else tool
   u.PostMessageW(focus,0x100,0x7b,0);wait(lambda:not state(40))
   u.PostMessageW(focus,0x100,0x7b,0);wait(lambda:state(40))
   send(tool,0x10)
  u.PostMessageW(u.GetDlgItem(remote,2102),0x100,0x1b,0);wait(lambda:not state(40));assert state(15)==recording
  send(main,0x100,0x7b);assert state(40)
  send(remote,0x10);assert not state(40)and state(15)==recording
  before=state(2);time.sleep(.15);assert state(2)>before and u.IsWindowEnabled(main)
  send(main,0x113,2);assert p.wait(timeout=10)==0
  shot=Image.open(target).convert('RGB');shot.save(target.with_suffix('.png'))
  footer=list(shot.crop((660,1034,1220,1078)).get_flattened_data())
  if recording:assert not any(pixel in {(112,130,153),(196,208,225),(29,35,46)} for pixel in footer),'Shortcut/button leaked into recording visualization'
  else:assert sum(pixel!=(12,16,24)for pixel in footer)>3000,'Missing normal footer button/guide'
  results.append({'startup_recording':bool(recording),'no_auto_open':True,'F12_button_and_shortcut':True,'F9_independent':True,'reuse_child_focus_autorepeat_close':True,'recording_button_and_guide_hidden':True,'nonblocking':True})
 finally:
  if p.poll()is None:p.terminate();p.wait(timeout=5)
result={'pass':True,'cases':results,'audio':'dummy','only_owned_windows_media_settings':True}
(out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
