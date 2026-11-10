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
  Image.frombytes('RGB',(w,height),data.raw,'raw','BGRX').save(out/'hidden.png')
 finally:g.SelectObject(mem,old);g.DeleteObject(bmp);g.DeleteDC(mem);u.ReleaseDC(h,dc)

u.GetWindowTextW.argtypes=[W.HWND,W.LPWSTR,C.c_int]
u.IsWindowEnabled.argtypes=[W.HWND]
settings=out/'settings.ini';settings.write_text('spectrumView=0\nspectrumStyle=3\nautoplay=1\nreverb=0\ncontrolsRevision=1415\n')
# Owned16-channel MIDI exercises the two-column channel grid.
track=b''.join(b'\0'+bytes((0x90+c,60,80))for c in range(16))
track+=b'\xa5\x40'+bytes((0x80,60,0))+b''.join(b'\0'+bytes((0x80+c,60,0))for c in range(1,16))+b'\0\xff\x2f\0'
multi=out/'all16.mid';multi.write_bytes(b'MThd'+struct.pack('>IHHH',6,0,1,480)+b'MTrk'+struct.pack('>I',len(track))+track)
results=[]
for session in range(4):
 music=fixture/('channels.nsfe' if session<2 else 'channels.gbs')
 if session==3:
  music=multi;settings.write_text('spectrumView=0\nmidiDeviceOutput=0\nmidiOutputRevision=15122\nfollowVirtualMidi=0\nreverb=0\ncontrolsRevision=1415\n')
  (out/'midi-soundfont.txt').write_text(str(fixture/'midi/한글 음색.sf2'),encoding='utf-8')
 env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='hidden163b1',NGMV_TEST_SNAPSHOT_MS='20000',NGMV_SETTINGS_TEST_FILE=str(settings))
 p=subprocess.Popen([str(exe),'--snapshot',str(music),str(out/('main'+str(session)+'.bmp')),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
 try:
  main=wait(lambda:find(p.pid,'NintendoMusicStudioWindow'));state=lambda n,b=0:send(main,0x8000+74,n,b)
  wait(lambda:state(0)and state(1)and state(2)>100)
  def point(x,y):
   rect=W.RECT();u.GetClientRect(main,C.byref(rect));scale=min(rect.right/1920,rect.bottom/1080)
   return int((rect.right-1920*scale)/2+x*scale)|(int((rect.bottom-1080*scale)/2+y*scale)<<16)
  def right(x,y):send(main,0x205,0,point(x,y))
  def mute_gains():return [(state(18,i),state(19,i))for i in range(state(17))]
  if session==0:
   n=state(20);assert n>=2 and state(38)==0
   before=mute_gains();first=state(21,0);right(500,300)
   assert state(20)==n-1 and state(38)==1 and state(39,first)
   second=state(21,0);right(500,300);assert state(38)==2 and state(20)==n-2
   assert mute_gains()==before,'Hiding modified audio controls'
   right(500,160);assert state(38)==2,'Toolbar right-click hid channel'
   send(main,0x100,0x78);assert state(15)
   send(main,0x100,0x7b)
   remote=wait(lambda:find(p.pid,'NintendoMusicStudioRemote'));button=u.GetDlgItem(remote,2114);assert button
   text=C.create_unicode_buffer(128);u.GetWindowTextW(button,text,128);assert '(2)' in text.value
   send(button,0xf5);dialog=wait(lambda:find(p.pid,'NintendoMusicStudioHiddenChannels'));listing=u.GetDlgItem(dialog,2700)
   assert send(listing,0x18b)==2
   assert u.IsWindowEnabled(main),'Hidden list disabled main'
   start=state(2);time.sleep(.16);assert state(2)>start,'Hidden list blocked playback'
   capture(dialog)
   send(listing,0x186,0);send(u.GetDlgItem(dialog,2701),0xf5)
   assert state(38)==1 and not state(39,first) and state(39,second)
   send(u.GetDlgItem(dialog,2702),0xf5);assert state(38)==0 and state(20)==n
   assert send(listing,0x18b)==0 and not u.IsWindowEnabled(u.GetDlgItem(dialog,2702))
   send(dialog,0x10)
   # Recording-view hit testing must select the correct still-visible row.
   record_first=state(21,0);right(500,32);assert state(39,record_first)and state(38)==1
   send(button,0xf5);assert find(p.pid,'NintendoMusicStudioHiddenChannels')==dialog
   send(u.GetDlgItem(dialog,2702),0xf5);assert state(20)==n
   send(dialog,0x10);send(main,0x100,0x78)
   for i in range(n):right(500,300)
   assert state(20)==0 and state(38)==n,'Hiding all channels failed'
   assert mute_gains()==before,'Hide all muted audio'
   send(main,0xf)
   send(main,0x100,0x78);send(button,0xf5);send(u.GetDlgItem(dialog,2702),0xf5);assert state(20)==n
   send(dialog,0x10);send(main,0x100,0x78)
   right(500,300);assert state(38)==1
   send(main,0x100,0x20);send(main,0x100,0x20);assert state(38)==1
   # Native Stop/Play retains only the display preference; audio controls untouched.
   pos=point(735,165);send(main,0x201,1,pos);send(main,0x202,0,pos)
   assert state(38)==1 and state(20)==n-1
   pos=point(585,165);send(main,0x201,1,pos);send(main,0x202,0,pos)
   send(main,0x100,0x27);assert state(38)==1,'Subsong lost hidden preference'
   # Stereo spectrum has no individual row to hide.
   send(main,0x100,0x75);right(500,300);assert state(38)==1
   send(main,0x100,0x75)
   results.append({'normal_and_recording_right_click':True,'hidden_list_selected_and_all_restore':True,'hide_all_safe':True,'automatic_unused_filter_unchanged':True,'mute_gain_unchanged':True,'subsong_stop_play_persist':True,'spectrum_and_toolbar_no_hide':True})
  elif session==1:
   assert state(38)==1 and state(39,0),'Restart lost saved preference'
   results.append({'process_restart_persistence':True})
  elif session==2:
   assert state(38)==0 and state(20)==4,'Hidden preference leaked to other system'
   results.append({'other_system_isolation':True})
  if session==3:
   n=state(20);assert n==16,('Own MIDI grid not16',n)
   target=state(21,8);before=mute_gains();right(1050,300)
   assert state(39,target)and state(20)==15 and mute_gains()==before
   send(main,0x100,0x78);assert state(15)
   next_target=state(21,8);right(1050,32);assert state(39,next_target)and state(20)==14
   send(main,0x100,0x7b)
   remote=wait(lambda:find(p.pid,'NintendoMusicStudioRemote'));send(u.GetDlgItem(remote,2114),0xf5)
   dialog=wait(lambda:find(p.pid,'NintendoMusicStudioHiddenChannels'));assert send(u.GetDlgItem(dialog,2700),0x18b)==2
   send(u.GetDlgItem(dialog,2702),0xf5);assert state(20)==16 and state(38)==0
   results.append({'split16channel_grid_normal_and_recording':True,'right_column_identity':True,'restore16':True})
  send(main,0x113,2);assert p.wait(timeout=10)==0
 finally:
  if p.poll()is None:p.terminate();p.wait(timeout=5)
result={'pass':True,'cases':results,'audio':'dummy','own_children_settings_media_only':True}
(out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
