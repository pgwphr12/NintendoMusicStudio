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


settings=out/'settings.ini';results=[]
for session in (0,1):
 if session==0:settings.write_text('showAllChannels=0\nspectrumView=0\nrepeat=0\nreverb=0\ncontrolsRevision=1415\n')
 env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='show-all-native',NGMV_SETTINGS_TEST_FILE=str(settings),NGMV_TEST_SNAPSHOT_MS='20000')
 p=subprocess.Popen([str(exe),'--snapshot',str(fixture),str(out/('view'+str(session)+'.bmp')),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
 try:
  main=wait(lambda:find(p.pid,'NintendoMusicStudioWindow'));state=lambda a,b=0:send(main,0x8000+74,a,b)
  wait(lambda:state(0)and state(1));total=state(17);automatic=int(os.environ.get("NGMV_EXPECT_VISIBLE","13"))
  assert total==int(os.environ.get("NGMV_EXPECT_TOTAL","44")),(total,"Unexpected raw channel count")
  assert state(20)==(automatic if session==0 else total),'Show-all preference not restored'
  before=[(state(18,i),state(19,i))for i in range(total)]
  send(main,0x100,0x7b);remote=wait(lambda:find(p.pid,'NintendoMusicStudioRemote'));check=u.GetDlgItem(remote,2115);assert check
  assert send(check,0xf0)==(session==1),'Checkbox state wrong'
  if session==0:
   rect=W.RECT();u.GetClientRect(main,C.byref(rect));scale=min(rect.right/1920,rect.bottom/1080);x=int((rect.right-1920*scale)/2+500*scale);y=int((rect.bottom-1080*scale)/2+300*scale)
   send(main,0x205,0,x|(y<<16));assert state(20)==automatic-1 and state(38)==1
   send(check,0xf5);assert state(20)==total and send(check,0xf0)==1,'Show-all did not temporarily override manual/automatic hiding'
   assert [(state(18,i),state(19,i))for i in range(total)]==before,'Visibility option changed sound controls'
   send(check,0xf5);assert state(20)==automatic-1,'Uncheck lost prior hidden state'
   send(u.GetDlgItem(remote,2114),0xf5);hidden=wait(lambda:find(p.pid,'NintendoMusicStudioHiddenChannels'));send(u.GetDlgItem(hidden,2702),0xf5);send(hidden,0x10);assert state(20)==automatic
   send(check,0xf5);assert state(20)==total
   send(main,0x100,0x78);assert state(15)and state(20)==total,'Recording changed show-all'
   capture(remote)
  else:
   send(check,0xf5);assert state(20)==automatic and not send(check,0xf0),'Restored option cannot return to automatic'
  send(main,0x113,2);assert p.wait(timeout=10)==0
  results.append({'session':session,'total':total,'automatic':automatic,'option_works':True,'settings_restored':bool(session),'mute_gain_unchanged':True})
 finally:
  if p.poll() is None:p.terminate();p.wait(timeout=5)
(out/'result.json').write_text(json.dumps({'pass':True,'cases':results,'audio':'dummy','only_owned_children':True},indent=2));print(json.dumps(results))
