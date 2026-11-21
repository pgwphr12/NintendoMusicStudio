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

u.GetWindowRect.argtypes=[W.HWND,C.POINTER(W.RECT)]
u.GetWindowTextW.argtypes=[W.HWND,W.LPWSTR,C.c_int]
u.IsWindowVisible.argtypes=[W.HWND];u.GetParent.argtypes=[W.HWND];u.GetParent.restype=W.HWND
u.PostMessageW.argtypes=[W.HWND,W.UINT,W.WPARAM,W.LPARAM]
u.GetDlgCtrlID.argtypes=[W.HWND]
def text(h):
 b=C.create_unicode_buffer(32768);u.GetWindowTextW(h,b,len(b));return b.value
def children(h):
 values=[]
 @CB
 def visit(c,_):
  if u.GetParent(c)==h and u.IsWindowVisible(c):values.append(c)
  return True
 u.EnumChildWindows(h,visit,0);return values
def geometry(h):
 r=W.RECT();u.GetWindowRect(h,C.byref(r));return r.left,r.top,r.right,r.bottom
def layout(h):
 items=children(h);client=W.RECT();u.GetClientRect(h,C.byref(client));origin=W.POINT();u.ClientToScreen.argtypes=[W.HWND,C.POINTER(W.POINT)];u.ClientToScreen(h,C.byref(origin))
 for a in items:
  r=geometry(a);assert r[0]>=origin.x and r[1]>=origin.y and r[2]<=origin.x+client.right and r[3]<=origin.y+client.bottom,('clipped',text(a),r)
 for i,a in enumerate(items):
  ra=geometry(a)
  for b in items[i+1:]:
   rb=geometry(b);assert min(ra[2],rb[2])<=max(ra[0],rb[0]) or min(ra[3],rb[3])<=max(ra[1],rb[1]),('overlap',u.GetDlgCtrlID(a),text(a),u.GetDlgCtrlID(b),text(b))
def capture(h,name):
 g=C.WinDLL('gdi32');u.GetDC.argtypes=[W.HWND];u.GetDC.restype=W.HDC;u.ReleaseDC.argtypes=[W.HWND,W.HDC];u.PrintWindow.argtypes=[W.HWND,W.HDC,W.UINT]
 g.CreateCompatibleDC.argtypes=[W.HDC];g.CreateCompatibleDC.restype=W.HDC;g.CreateCompatibleBitmap.argtypes=[W.HDC,C.c_int,C.c_int];g.CreateCompatibleBitmap.restype=W.HBITMAP;g.SelectObject.argtypes=[W.HDC,W.HGDIOBJ];g.SelectObject.restype=W.HGDIOBJ;g.GetDIBits.argtypes=[W.HDC,W.HBITMAP,W.UINT,W.UINT,C.c_void_p,C.c_void_p,W.UINT];g.DeleteObject.argtypes=[W.HGDIOBJ];g.DeleteDC.argtypes=[W.HDC]
 r=W.RECT();u.GetClientRect(h,C.byref(r));w,z=r.right,r.bottom;dc=u.GetDC(h);mem=g.CreateCompatibleDC(dc);bmp=g.CreateCompatibleBitmap(dc,w,z);old=g.SelectObject(mem,bmp)
 try:
  u.PrintWindow(h,mem,1);g.SelectObject(mem,old)
  info=struct.pack('<IiiHHIIiiII',40,w,-z,1,32,0,w*z*4,0,0,0,0);buf=C.create_string_buffer(w*z*4);assert g.GetDIBits(mem,bmp,0,z,buf,C.create_string_buffer(info),0)==z
  (out/(name+'.bmp')).write_bytes(struct.pack('<2sIHHI',b'BM',54+w*z*4,0,0,54)+info+buf.raw)
 finally:g.SelectObject(mem,old);g.DeleteObject(bmp);g.DeleteDC(mem);u.ReleaseDC(h,dc)
env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='exporthotfix',NGMV_TEST_SNAPSHOT_MS='60000',NGMV_SETTINGS_TEST_FILE=str(settings));env.pop('NGMV_TEST_EXPORT_DIR',None)
p=subprocess.Popen([str(exe),'--snapshot',str(audio),str(out/'main.bmp'),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
try:
 main=wait(lambda:find(p.pid,'NintendoMusicStudioWindow'));wait(lambda:send(main,0x8000+74,0) and send(main,0x8000+74,1))
 send(main,0x100,0x7b);remote=wait(lambda:find(p.pid,'NintendoMusicStudioRemote'));layout(remote);capture(remote,'remote')
 for id,cls,name in [(2107,'NintendoMusicStudioEqualizer','eq'),(2109,'NintendoMusicStudioSpectrumChoices','spectrum'),(2114,'NintendoMusicStudioHiddenChannels','hidden')]:
  send(u.GetDlgItem(remote,id),0xf5);tool=wait(lambda:find(p.pid,cls));layout(tool);capture(tool,name);send(tool,0x10)
 r=W.RECT();u.GetClientRect(main,C.byref(r));scale=min(r.right/1920,r.bottom/1080);pos=int((r.right-1920*scale)/2+980*scale)|(int((r.bottom-1080*scale)/2+45*scale)<<16);send(main,0x201,1,pos);send(main,0x202,0,pos)
 send(main,0x100,0x70);about=wait(lambda:find(p.pid,'NintendoMusicStudioInfo'));layout(about);capture(about,'about');send(about,0x10)
 export=wait(lambda:find(p.pid,'NintendoMusicStudioExport'));layout(export);capture(export,'export');duration=C.create_unicode_buffer('0.05');send(u.GetDlgItem(export,2703),0xc,0,C.addressof(duration))
 for suffix in ['', '_1']:
  u.PostMessageW(u.GetDlgItem(export,2707),0xf5,0,0)
  picker=wait(lambda:next((h for h in [find(p.pid,'#32770')] if h and 'fixture' in text(h)),None))
  nested=[]
  @CB
  def visit(h,_):
   b=C.create_unicode_buffer(128);u.GetClassNameW(h,b,128);nested.append(b.value);return True
  def ready():
   nested.clear();u.EnumChildWindows(picker,visit,0);return 'DirectUIHWND' in nested
  wait(ready)
  u.PostMessageW(picker,0x111,1,0)
  result=wait(lambda:next((x for x in [out/('fixture'+suffix)] if (x/'master.wav').is_file()),None))
  wait(lambda:len(list(result.glob('*.wav')))>=2)
  with wave.open(str(result/'master.wav'),'rb') as wav:assert wav.getframerate()==48000 and wav.getnframes()==2400
  wait(lambda:'완료:' in text(u.GetDlgItem(export,2706)))
 # A third selection is cancelled and must not create a result directory.
 u.PostMessageW(u.GetDlgItem(export,2707),0xf5,0,0);picker=wait(lambda:find(p.pid,'#32770'));u.PostMessageW(picker,0x10,0,0);wait(lambda:not find(p.pid,'#32770'));assert not (out/'fixture_2').exists()
 send(main,0x113,2);assert p.wait(timeout=10)==0
 (out/'result.json').write_text(json.dumps({'pass':True,'modern_picker':True,'default_source_parent':True,'song_named_folder':True,'collision_preserves_existing':True,'cancel_no_folder':True,'master_alongside_stems':True,'six_tools_no_control_overlaps':True,'audio':'dummy'},indent=2));print('PASS modern picker accepts source parent, song folder, collision, cancellation, master and six tool layouts')
finally:
 if p.poll() is None:p.terminate();p.wait(timeout=5)
