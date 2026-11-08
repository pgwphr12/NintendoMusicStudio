"""Own offscreen player: three two-row styles and a modeless choice window, persistence and no image feature."""
from pathlib import Path
import ctypes as C
from ctypes import wintypes as W
import json,math,os,struct,subprocess,sys,time,wave,csv
from PIL import Image
exe=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve();out.mkdir(parents=True,exist_ok=True)
u=C.WinDLL('user32',use_last_error=True)
u.SendMessageTimeoutW.argtypes=[W.HWND,W.UINT,W.WPARAM,W.LPARAM,W.UINT,W.UINT,C.POINTER(C.c_size_t)];u.SendMessageTimeoutW.restype=W.LPARAM
u.GetDlgItem.argtypes=[W.HWND,C.c_int];u.GetDlgItem.restype=W.HWND
u.GetWindowTextW.argtypes=[W.HWND,W.LPWSTR,C.c_int]
u.IsWindowEnabled.argtypes=[W.HWND]
u.GetClassNameW.argtypes=[W.HWND,W.LPWSTR,C.c_int]
u.GetWindowThreadProcessId.argtypes=[W.HWND,C.POINTER(W.DWORD)]
u.GetClientRect.argtypes=[W.HWND,C.POINTER(W.RECT)]
callback=C.WINFUNCTYPE(W.BOOL,W.HWND,W.LPARAM)
def windows(pid,name):
 found=[]
 @callback
 def visit(h,_):
  p=W.DWORD();u.GetWindowThreadProcessId(h,C.byref(p));v=C.create_unicode_buffer(256);u.GetClassNameW(h,v,256)
  if p.value==pid and v.value==name:found.append(h)
  return True
 u.EnumWindows(visit,0);return found
def wait(fn,message,seconds=8):
 deadline=time.monotonic()+seconds
 while time.monotonic()<deadline:
  value=fn()
  if value:return value
  time.sleep(.02)
 raise AssertionError(message)
def send(h,msg,a=0,b=0):
 result=C.c_size_t();assert u.SendMessageTimeoutW(h,msg,a,b,2,1500,C.byref(result)),('Own window unresponsive',msg,C.get_last_error());return result.value

music=out/'stereo.wav'
with wave.open(str(music),'wb')as f:
 f.setparams((2,2,48000,0,'NONE','not compressed'))
 chunk=bytearray()
 for i in range(48000*4):
  t=i/48000.;phase=2*math.pi*(110*t+12*t*t);other=2*math.pi*(180*t+17*t*t)
  l=int(8000*sum(math.sin(phase*h)/h for h in range(1,12)))
  r=int(8000*sum(math.sin(other*h)/h for h in range(1,12)))
  chunk+=struct.pack('<hh',l,r)
 f.writeframes(bytes(chunk)*5)
settings=out/'settings.ini';settings.write_text('autoplay=1\nrecording=0\nspectrumView=0\nrate=48000\nreverb=0\ncontrolsRevision=1415\n')
legacy=out/'spectrum-image.txt';legacy.write_text('C:/never-open-this-removed-image.png')
results=[]
for session,style in enumerate((0,1,3)):
 env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_RENDER_BENCHMARK='1',NGMV_TEST_SESSION='styles162t12',NGMV_TEST_SNAPSHOT_MS='25000',NGMV_SETTINGS_TEST_FILE=str(settings))
 destination=out/('bars.bmp' if style==0 else 'lines.bmp' if style==1 else 'scrolling.bmp')
 p=subprocess.Popen([str(exe),'--snapshot',str(music),str(destination),'--size=1920x1080'],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
 try:
  main=wait(lambda:windows(p.pid,'NintendoMusicStudioWindow'),'No owned main')[0]
  state=lambda n:send(main,0x8000+74,n)
  wait(lambda:state(0)and state(1)and state(2)>160,'Playback failed')
  assert state(31)==(3 if session==0 else (0,1)[session-1]),'Default or saved type failed'
  prior=state(31)
  if style==0:send(main,0x100,0x75)
  send(main,0x100,0x76)
  choice=wait(lambda:windows(p.pid,'NintendoMusicStudioSpectrumChoices'),'No choices')[0]
  assert state(37) and state(31)==prior,'Opening chooser changed type'
  send(main,0x100,0x76);assert windows(p.pid,'NintendoMusicStudioSpectrumChoices')==[choice],'Duplicate chooser'
  for i,label in enumerate(('막대형','선형','가로흐름')):
   h=u.GetDlgItem(choice,2600+i);assert h
   text=C.create_unicode_buffer(256);u.GetWindowTextW(h,text,256);assert label in text.value
  assert not u.GetDlgItem(choice,2603),'Removed option remains'
  selected=2602 if style==3 else 2600+style
  send(u.GetDlgItem(choice,selected),0xf5) # Actual native radio-button click.
  assert state(30)==1 and state(31)==style,'Direct selection failed'
  assert send(u.GetDlgItem(choice,selected),0xf0)==1,'Selected radio unchecked'
  before=state(2);time.sleep(.18);assert state(2)>before,'Playback froze with choices open'
  assert u.IsWindowEnabled(main),'Choice window disabled player'
  send(choice,0x111,2);assert not state(37)
  wait(lambda:state(32)>200,'No output spectrum')
  if style==1:
   # Main header hit test opens the same window without cycling.
   click_pos=1380|(44<<16);send(main,0x201,1,click_pos);send(main,0x202,0,click_pos)
   assert state(37) and state(31)==1
   send(choice,0x111,2)
   send(main,0x100,0x78);assert state(15)
   remote=wait(lambda:windows(p.pid,'NintendoMusicStudioRemote'),'No remote')[0]
   send(remote,0x111,2109);assert state(37) and state(31)==1
   warm=state(34)
   for i in range(60):
    send(u.GetDlgItem(choice,(2600,2601,2602)[i%3]),0xf5);send(main,0xf)
   assert state(31)==3
   send(u.GetDlgItem(choice,2601),0xf5);assert state(31)==1
   assert state(34)<=warm+1,'Selection resources accumulate'
   send(choice,0x111,2);send(main,0x100,0x78);assert not state(15)
  if style==3:wait(lambda:state(35)>=160,'No horizontal history',6)
  send(main,0x100,0x20);wait(lambda:not state(1),'Pause failed')
  paused_time=state(36);paused_columns=state(35)
  time.sleep(.8);assert state(36)==paused_time and state(35)==paused_columns,'Paused horizontal timeline changed'
  assert state(32)<100,'Front curve did not decay'
  send(main,0x100,0x20);wait(lambda:state(32)>200,'Resume failed')
  assert state(36)-paused_time<400,'Paused wall time was added on resume'
  if style==3:
   before=state(36);time.sleep(.25);assert state(36)>before+120,'Horizontal display did not advance'
   send(main,0x100,0x78);assert state(15)  # Capture requested16:9 recording layout.
  send(main,0x113,2);assert p.wait(timeout=10)==0
  shot=Image.open(destination).convert('RGB');shot.save(destination.with_suffix('.png'))
  for area in [(54,280,1866,610),(54,650,1866,990)]:
   pixels=list(shot.crop(area).get_flattened_data());assert sum(v[2]-v[0]>22 or max(v)-min(v)>100 for v in pixels)>300,'Missing stereo row'
  with Path(str(destination)+'.render.csv').open()as f:benchmark=next(csv.DictReader(f))
  assert int(benchmark['gdi_after'])<=int(benchmark['gdi_before']),'Rendering GDI grows'
  results.append({'style':style,'name':{0:'bars',1:'lines',3:'scrolling'}[style],'two_rows':True,'pause_resume':True,'persistence':True,'benchmark':benchmark})
 finally:
  if p.poll()is None:p.terminate();p.wait(timeout=5)
assert 'spectrumStyle=3' in settings.read_text()
assert legacy.read_text()=='C:/never-open-this-removed-image.png','Removed image setting was accessed/modified'
result={'pass':True,'cases':results,'audio':'dummy','only_owned_processes_settings_media':True,'image_feature_removed':True,'waterfall_removed':True,'default_type':'scrolling','choice_window_nonmodal':True,'entry_points':['F7','header','recording remote']}
(out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
