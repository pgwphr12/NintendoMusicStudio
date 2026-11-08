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
settings=out/'settings.ini';settings.write_text('spectrumView=1\nspectrumStyle=2\nreverb=0\ncontrolsRevision=1415\n')
env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='choices162t12',NGMV_TEST_SNAPSHOT_MS='20000',NGMV_SETTINGS_TEST_FILE=str(settings))
p=subprocess.Popen([str(exe),'--snapshot',str(fixture),str(out/'main.bmp'),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
try:
 main=wait(lambda:find(p.pid,'NintendoMusicStudioWindow'));state=lambda n:send(main,0x8000+74,n)
 wait(lambda:state(0)and state(1));assert state(31)==3,'Legacy waterfall not migrated'
 send(main,0x100,0x76);choice=wait(lambda:find(p.pid,'NintendoMusicStudioSpectrumChoices'))
 assert state(37)and send(u.GetDlgItem(choice,2602),0xf0)==1
 capture(choice)
 # Native modeless dialog keyboard routes, with no global input injection.
 send(choice,0x28,u.GetDlgItem(choice,2602),1)
 u.PostMessageW(u.GetDlgItem(choice,2602),0x100,0x26,0)
 wait(lambda:state(31)==1)
 u.PostMessageW(u.GetDlgItem(choice,2601),0x100,0x1b,0);wait(lambda:not state(37))
 send(main,0x100,0x76);assert state(37)
 assert find(p.pid,'NintendoMusicStudioSpectrumChoices')==choice
 send(u.GetDlgItem(choice,2602),0xf5);assert state(31)==3
 send(choice,0x10);assert not state(37)
 send(main,0x113,2);assert p.wait(timeout=10)==0
 assert 'spectrumStyle=3' in settings.read_text()
 result={'pass':True,'legacy2_to_scrolling3':True,'checked_radio':True,'native_arrow_selection':True,'Escape_hides':True,'close_reopen_reuses_window':True,'saved3':True,'only_owned_offscreen_windows':True,'audio':'dummy'}
 (out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
finally:
 if p.poll()is None:p.terminate();p.wait(timeout=5)
